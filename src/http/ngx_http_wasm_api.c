/*
 * Copyright 2022 Shenzhen ZhiLiu Technology Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#define _GNU_SOURCE /* for RTLD_DEFAULT */
#include <dlfcn.h>
#include "vm/vm.h"
#include "ngx_http_wasm_api.h"
#include "ngx_http_wasm_api_def.h"
#include "ngx_http_wasm_module.h"
#include "ngx_http_wasm_state.h"
#include "ngx_http_wasm_call.h"
#include "ngx_http_wasm_ctx.h"
#include "proxy_wasm/proxy_wasm_map.h"
#include "proxy_wasm/proxy_wasm_memory.h"


typedef struct {
    ngx_str_t   name;
    ngx_uint_t  ty;
    ngx_str_t  *(*getter) (ngx_http_request_t *r);
} ngx_http_wasm_pseudo_header_t;


#define STR_BUF_SIZE    4096

#define FFI_NO_REQ_CTX  -100
#define FFI_BAD_CONTEXT -101

#define must_get_memory(key, log, key_data, key_len) \
    key = (char *) ngx_wasm_vm->get_memory((log), (key_data), (key_len)); \
    if (key == NULL) { \
        return PROXY_RESULT_INVALID_MEMORY_ACCESS; \
    }
#define must_get_req(r) \
    r = ngx_http_wasm_get_req(); \
    if (r == NULL) { \
        return PROXY_RESULT_BAD_ARGUMENT; \
    }
#define must_resolve_symbol(f, name) \
    f = dlsym(RTLD_DEFAULT, #name); \
    err = dlerror(); \
    if (err != NULL) { \
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, \
                      "failed to resolve symbol: %s", err); \
        return NGX_ERROR; \
    }

#define PROXY_WASM_REQ_HEADER_STATIC_TABLE_ENTRIES \
    (sizeof(wasm_pseudo_req_header_static_table) \
     / sizeof(ngx_http_wasm_pseudo_header_t))

#define PROXY_WASM_RESP_HEADER_STATIC_TABLE_ENTRIES \
    (sizeof(wasm_pseudo_resp_header_static_table) \
     / sizeof(ngx_http_wasm_pseudo_header_t))


static ngx_str_t *ngx_http_wasm_get_path(ngx_http_request_t *r);
static ngx_str_t *ngx_http_wasm_get_method(ngx_http_request_t *r);
static ngx_str_t *ngx_http_wasm_get_scheme(ngx_http_request_t *r);
static ngx_str_t *ngx_http_wasm_get_status(ngx_http_request_t *r);
static ngx_str_t *ngx_http_wasm_get_pseudo_header(ngx_http_request_t *r,
                                                  u_char *key_data, size_t key_size, int ty);
static ngx_str_t *ngx_http_wasm_get_pseudo_req_header(ngx_http_request_t *r,
                                                      u_char *key_data, size_t key_size);
static ngx_str_t *ngx_http_wasm_get_pseudo_resp_header(ngx_http_request_t *r,
                                                       u_char *key_data, size_t key_size);

static int (*get_phase) (ngx_http_request_t *r, char **err);
static int (*set_resp_header) (ngx_http_request_t *r,
    const char *key_data, size_t key_len, int is_nil,
    const char *sval, size_t sval_len, ngx_str_t *mvals,
    size_t mvals_len, int override, char **errmsg);
static int (*set_resp_status) (ngx_http_request_t *r, int status);
static int (*get_resp_header) (ngx_http_request_t *r,
    const unsigned char *key, size_t key_len,
    unsigned char *key_buf, ngx_str_t *values,
    int max_nvalues, char **errmsg);
static int (*get_req_headers_count) (ngx_http_request_t *r,
    int max, int *truncated);
static int (*get_req_headers) (ngx_http_request_t *r,
    proxy_wasm_table_elt_t *out, int count, int raw);
static int (*set_req_header) (ngx_http_request_t *r,
    const char *key, size_t key_len, const char *value,
    size_t value_len, ngx_str_t *mvals, size_t mvals_len,
    int override, char **errmsg);
static int (*set_variable)(ngx_http_request_t *r, u_char *name_data,
    size_t name_len, u_char *lowcase_buf, u_char *value, size_t value_len,
    u_char *errbuf, size_t *errlen);

static char *err_bad_ctx = "API disabled in the current context";
static char *err_no_req_ctx = "no request ctx found";

static char *str_buf[STR_BUF_SIZE] = {0};

static ngx_str_t scheme_https = ngx_string("https");
static ngx_str_t scheme_http = ngx_string("http");
static unsigned char status_data_buf[NGX_INT_T_LEN];
static ngx_str_t status_str = { 0, status_data_buf };

static ngx_http_wasm_pseudo_header_t wasm_pseudo_req_header_static_table[] = {
    {ngx_string(":scheme"), PROXY_WASM_REQUEST_HEADER_SCHEME, ngx_http_wasm_get_scheme},
    {ngx_string(":path"),   PROXY_WASM_REQUEST_HEADER_PATH, ngx_http_wasm_get_path},
    {ngx_string(":method"), PROXY_WASM_REQUEST_HEADER_METHOD, ngx_http_wasm_get_method},
};

static ngx_http_wasm_pseudo_header_t wasm_pseudo_resp_header_static_table[] = {
    {ngx_string(":status"), PROXY_WASM_RESPONSE_HEADER_STATUS, ngx_http_wasm_get_status},
};


ngx_int_t
ngx_http_wasm_resolve_symbol(void)
{
    char        *err;

    dlerror();    /* Clear any existing error */

    must_resolve_symbol(get_phase, ngx_http_lua_ffi_get_phase);
    must_resolve_symbol(set_resp_header, ngx_http_lua_ffi_set_resp_header);
    must_resolve_symbol(set_resp_status, ngx_http_lua_ffi_set_resp_status);
    must_resolve_symbol(get_resp_header, ngx_http_lua_ffi_get_resp_header);
    must_resolve_symbol(get_req_headers_count, ngx_http_lua_ffi_req_get_headers_count);
    must_resolve_symbol(get_req_headers, ngx_http_lua_ffi_req_get_headers);
    must_resolve_symbol(set_req_header, ngx_http_lua_ffi_req_set_header);
    must_resolve_symbol(set_variable, ngx_http_lua_ffi_var_set);

    return NGX_OK;
}


static bool
ngx_http_wasm_is_yieldable(ngx_http_request_t *r)
{
    char    *errmsg;
    int      phase;

    phase = get_phase(r, &errmsg);
    if (phase < 0) {
        return false;
    }

    return (phase & NGX_HTTP_WASM_YIELDABLE) != 0;
}


static ngx_inline ngx_int_t
ngx_http_wasm_check_unsafe_uri_bytes(ngx_http_request_t *r, u_char *str,
    size_t len, u_char *byte)
{
    size_t           i;
    u_char           c;

                     /* %00-%08, %0A-%1F, %7F */

    static uint32_t  unsafe[] = {
        0xfffffdff, /* 1111 1111 1111 1111  1111 1101 1111 1111 */

                    /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */

                    /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */

                    /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0x80000000, /* 1000 0000 0000 0000  0000 0000 0000 0000 */

        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000  /* 0000 0000 0000 0000  0000 0000 0000 0000 */
    };

    for (i = 0; i < len; i++, str++) {
        c = *str;
        if (unsafe[c >> 5] & (1 << (c & 0x1f))) {
            *byte = c;
            return NGX_ERROR;
        }
    }

    return NGX_OK;
}


static ngx_inline ngx_int_t
ngx_http_wasm_check_unsafe_method(ngx_http_request_t *r, u_char *str,
    size_t len, u_char *byte)
{
    size_t           i;
    u_char           c;

    for (i = 0; i < len; i++) {
        c = str[i];
        if (!(('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'))) {
            *byte = c;
            return NGX_ERROR;
        }
    }

    return NGX_OK;
}


static int32_t
ngx_http_wasm_copy_to_wasm(ngx_log_t *log, const u_char *data, int32_t len,
                           int32_t addr, int32_t size_addr)
{
    int32_t         buf_addr;
    int32_t        *p_size;
    int32_t        *p;
    u_char         *buf;

    p_size = (int32_t *) ngx_wasm_vm->get_memory(log, size_addr, sizeof(int32_t));
    if (p_size == NULL) {
        return PROXY_RESULT_INVALID_MEMORY_ACCESS;
    }

    *p_size = len;

    if (len == 0) {
        return PROXY_RESULT_OK;
    }

    buf_addr = proxy_wasm_memory_alloc(log, len);
    if (buf_addr == 0) {
        ngx_log_error(NGX_LOG_ERR, log, 0, "no memory");
        return PROXY_RESULT_INTERNAL_FAILURE;
    }

    buf = (u_char *) ngx_wasm_vm->get_memory(log, buf_addr, len);
    if (buf == NULL) {
        return PROXY_RESULT_INVALID_MEMORY_ACCESS;
    }

    ngx_memcpy(buf, data, len);

    p = (int32_t *) ngx_wasm_vm->get_memory(log, addr, sizeof(int32_t));
    if (p == NULL) {
        return PROXY_RESULT_INVALID_MEMORY_ACCESS;
    }

    *p = buf_addr;
    return PROXY_RESULT_OK;
}


static u_char *
ngx_http_wasm_get_buf_to_write(ngx_log_t *log, int32_t len,
                               int32_t addr, int32_t size_addr)
{
    int32_t         buf_addr;
    int32_t        *p_size;
    int32_t        *p;
    u_char         *buf;

    buf_addr = proxy_wasm_memory_alloc(log, len);
    if (buf_addr == 0) {
        ngx_log_error(NGX_LOG_ERR, log, 0, "no memory");
        return NULL;
    }

    buf = (u_char *) ngx_wasm_vm->get_memory(log, buf_addr, len);
    if (buf == NULL) {
        return NULL;
    }

    p = (int32_t *) ngx_wasm_vm->get_memory(log, addr, sizeof(int32_t));
    if (p == NULL) {
        return NULL;
    }

    *p = buf_addr;

    p_size = (int32_t *) ngx_wasm_vm->get_memory(log, size_addr, sizeof(int32_t));
    if (p_size == NULL) {
        return NULL;
    }

    *p_size = len;

    return buf;
}


static ngx_int_t
ngx_http_wasm_set_req_header(ngx_http_request_t *r,
    const char *key, size_t key_len,
    const char *val, size_t val_len,
    int override)
{
    char                          *errmsg = NULL;
    ngx_int_t                      rc = NGX_OK;
    ngx_uint_t                     found = 0;
    ngx_uint_t                     i, entries;
    ngx_http_wasm_pseudo_header_t *wh, *whs;

    if (key_len > 0 && key[0] == ':') {
        entries = (ngx_uint_t) PROXY_WASM_REQ_HEADER_STATIC_TABLE_ENTRIES;
        whs = &wasm_pseudo_req_header_static_table[0];

        if (val_len == 0) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "failed to set request header %*s: can't remove pseudo header",
                          key_len, key);
            return NGX_ERROR;
        }

        /* skip :scheme which is readonly */
        for (i = 1; i < entries; i++) {
            wh = whs + i;

            if (key_len != wh->name.len) {
                continue;
            }

            if (ngx_strncasecmp((u_char *) key, wh->name.data, wh->name.len) == 0) {
                u_char *p;

                found = 1;

                if (wh->ty == PROXY_WASM_REQUEST_HEADER_PATH) {
                    u_char              byte;

                    if (ngx_http_wasm_check_unsafe_uri_bytes(r, (u_char *) val, val_len, &byte)
                        != NGX_OK)
                    {
                        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                                      "failed to set request header %*s: invalid char \"%d\"",
                                      key_len, key, byte);

                        return NGX_ERROR;
                    }

                    p = ngx_palloc(r->pool, val_len);
                    if (p == NULL) {
                        return NGX_ERROR;
                    }
                    r->uri.data = p;

                    ngx_memcpy(r->uri.data, val, val_len);

                    r->uri.len = val_len;

                    r->internal = 1;
                    r->valid_unparsed_uri = 0;

                    ngx_http_set_exten(r);

                    r->valid_location = 0;
                    r->uri_changed = 0;

                } else {
                    /* method */
                    u_char              byte;

                    if (ngx_http_wasm_check_unsafe_method(r, (u_char *) val, val_len, &byte)
                        != NGX_OK)
                    {
                        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                                      "failed to set request header %*s: invalid char \"%d\"",
                                      key_len, key, byte);

                        return NGX_ERROR;
                    }

                    p = ngx_palloc(r->pool, val_len);
                    if (p == NULL) {
                        return NGX_ERROR;
                    }
                    r->method_name.data = p;

                    /* according to RFC 2616, "The Method token indicates
                     * the method to be performed on the resource identified by
                     * the Request-URI. The method is case-sensitive.",
                     * the caller should take care of the case of the input */

                    /* we don't restrict the range of method so
                     * the caller can use her custom verb like "PURGE" */
                    ngx_memcpy(r->method_name.data, val, val_len);
                    r->method_name.len = val_len;
                }

                break;
            }
        }
    }

    if (!found) {
        rc = set_req_header(r, key, key_len, val, val_len, NULL, 0,
                            override, &errmsg);
    }

    if (rc != NGX_OK && rc != NGX_DECLINED) {
        if (rc == FFI_BAD_CONTEXT) {
            errmsg = err_bad_ctx;
        }

        if (errmsg != NULL) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "failed to set request header %*s to %*s: %s",
                          key_len, key, val_len, val, errmsg);
        }

        return NGX_ERROR;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_wasm_set_resp_header(ngx_http_request_t *r,
    const char *key, size_t key_len, int is_nil,
    const char *val, size_t val_len,
    int override)
{
    char               *errmsg = NULL;
    ngx_int_t           rc;

    if (key_len == 7 && ngx_strncasecmp((u_char *) key, (u_char *) ":status", 7) == 0) {
        if (is_nil) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "failed to set response header %*s: can't remove pseudo header",
                          key_len, key);
            return NGX_ERROR;
        }

        rc = ngx_atoi((u_char *) val, val_len);
        if (rc == NGX_ERROR) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "failed to set response header %*s: invalid value",
                          key_len, key);
            return NGX_ERROR;
        }

        rc = set_resp_status(r, rc);

    } else {
        rc = set_resp_header(r, key, key_len, is_nil, val, val_len, NULL, 0,
                             override, &errmsg);
    }

    if (rc != NGX_OK && rc != NGX_DECLINED) {
        if (rc == FFI_BAD_CONTEXT) {
            errmsg = err_bad_ctx;
        } else if (rc == FFI_NO_REQ_CTX) {
            errmsg = err_no_req_ctx;
        }

        if (errmsg != NULL) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "failed to set response header %*s to %*s: %s",
                          key_len, key, val_len, val, errmsg);
        }

        return NGX_ERROR;
    }

    return NGX_OK;
}


static void *
ngx_http_wasm_get_string_buf(ngx_pool_t *pool, size_t size)
{
    if (size > STR_BUF_SIZE) {
        return ngx_palloc(pool, size);
    }

    return str_buf;
}


int32_t
proxy_set_effective_context(int32_t id)
{
    /* dummy function to make plugin load */
    return PROXY_RESULT_OK;
}


int32_t
proxy_get_property(int32_t path_data, int32_t path_size,
                   int32_t res_data, int32_t res_size)
{
    u_char                      *lowcase_buf;
    ngx_log_t                   *log;
    ngx_str_t                    property_name;
    ngx_uint_t                   hash;
    const u_char                *p;
    ngx_http_request_t          *r;
    ngx_http_variable_value_t   *vv;

    log = ngx_http_wasm_get_log();

    p = ngx_wasm_vm->get_memory(log, path_data, path_size);
    if (p == NULL) {
        return PROXY_RESULT_INVALID_MEMORY_ACCESS;
    }

    if (path_size == 14 && ngx_strncmp(p, "plugin_root_id", 14) == 0) {
        /* assemblyscript SDK assumes plugin_root_id is always given */
        ngx_str_t       *name;

        name = ngx_http_wasm_get_plugin_name();
        return ngx_http_wasm_copy_to_wasm(log, name->data, name->len,
                                          res_data, res_size);
    }

    /*
     * assemblyscript need plugin_root_id to initialize, and the state
     * will set after plugin initialization completed
     */
    must_get_req(r);

    lowcase_buf = ngx_http_wasm_get_string_buf(r->pool, path_size);
    hash = ngx_hash_strlow(lowcase_buf, (u_char *) p, path_size);
    property_name.data = lowcase_buf;
    property_name.len = path_size;

    vv = ngx_http_get_variable(r, &property_name, hash);

    if (vv->not_found == 1) {
        return PROXY_RESULT_NOT_FOUND;
    }

    return ngx_http_wasm_copy_to_wasm(log, vv->data, vv->len,
                                      res_data, res_size);
}


int32_t
proxy_set_property(int32_t path_data, int32_t path_size,
                   int32_t data, int32_t size)
{
    int                          result;
    u_char                      *key_lowcase;
    u_char                      *errmsg;
    size_t                       errlen = 256;
    ngx_log_t                   *log;
    const u_char                *key;
    const u_char                *value;
    ngx_http_request_t          *r;

    log = ngx_http_wasm_get_log();
    must_get_req(r);

    /* fetch property key and value */
    key = ngx_wasm_vm->get_memory(log, path_data, path_size);
    if (key == NULL) {
        return PROXY_RESULT_INVALID_MEMORY_ACCESS;
    }

    value = ngx_wasm_vm->get_memory(log, data, size);
    if (key == NULL) {
        return PROXY_RESULT_INVALID_MEMORY_ACCESS;
    }

    /*
     * Request a piece of temporary memory to store the
     * lowercase characters of the property key and errmsg
     * of variable setting.
     */
    key_lowcase = ngx_http_wasm_get_string_buf(r->pool, path_size + errlen);
    errmsg = key_lowcase + path_size;

    /* Call the functions in lua-resty-core to set the variables. */
    result = set_variable(r, (u_char *) key, path_size, key_lowcase,
                          (u_char *) value, size, errmsg, &errlen);

    if (result != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, log, 0, (const char *)errmsg);

        if (ngx_strstrn(errmsg, "not found for writing", 20) != NULL) {
            return PROXY_RESULT_NOT_FOUND;
        }

        return PROXY_RESULT_INTERNAL_FAILURE;
    }

    return PROXY_RESULT_OK;
}


int32_t
proxy_log(int32_t log_level, int32_t addr, int32_t size)
{
    const u_char       *p;
    ngx_uint_t          host_log_level = NGX_LOG_ERR;
    ngx_log_t          *log;

    log = ngx_http_wasm_get_log();

    p = ngx_wasm_vm->get_memory(log, addr, size);
    if (p == NULL) {
        return PROXY_RESULT_INVALID_MEMORY_ACCESS;
    }

    switch (log_level) {
      case PROXY_LOG_CRITICAL:
        host_log_level = NGX_LOG_EMERG;
        break;

      case PROXY_LOG_ERROR:
        host_log_level = NGX_LOG_ERR;
        break;

      case PROXY_LOG_WARN:
        host_log_level = NGX_LOG_WARN;
        break;

      case PROXY_LOG_INFO:
        host_log_level = NGX_LOG_INFO;
        break;

      case PROXY_LOG_DEBUG:
        host_log_level = NGX_LOG_DEBUG;
        break;

      case PROXY_LOG_TRACE:
        host_log_level = NGX_LOG_DEBUG;
        break;
    }

    ngx_log_error(host_log_level, log, 0, "%*s", size, p);

    return PROXY_RESULT_OK;
}


int32_t
proxy_get_buffer_bytes(int32_t type, int32_t start, int32_t size,
                       int32_t addr, int32_t size_addr)
{
    ngx_log_t            *log;
    const u_char         *data = NULL;
    int32_t               len = 0;
    const ngx_str_t      *buffer;

    ngx_http_request_t      *r;
    ngx_http_wasm_ctx_t     *ctx;

    log = ngx_http_wasm_get_log();

    switch (type) {
    case PROXY_BUFFER_TYPE_PLUGIN_CONFIGURATION:
        buffer = ngx_http_wasm_get_conf();
        break;

    case PROXY_BUFFER_TYPE_HTTP_REQUEST_BODY:
    case PROXY_BUFFER_TYPE_HTTP_RESPONSE_BODY:
        buffer = ngx_http_wasm_get_body();
        break;

    case PROXY_BUFFER_TYPE_HTTP_CALL_RESPONSE_BODY:
        must_get_req(r);
        ctx = ngx_http_wasm_get_module_ctx(r);
        buffer = ctx->call_resp_body;
        break;

    default:
        return PROXY_RESULT_UNIMPLEMENTED;
    }

    if (buffer != NULL) {
        data = buffer->data;
        len = buffer->len;
    }

    if (len == 0) {
        return PROXY_RESULT_NOT_FOUND;
    }

    if (type != PROXY_BUFFER_TYPE_PLUGIN_CONFIGURATION) {
        /* configuration is fetched as a whole */
        if (start < 0 || size <= 0 || start >= len) {
            return PROXY_RESULT_BAD_ARGUMENT;
        }

        if (start + size > len) {
            size = len - start;
        }

        data = data + start;
        len = size;
    }

    return ngx_http_wasm_copy_to_wasm(log, data, len, addr, size_addr);
}


int32_t
proxy_set_buffer_bytes(int32_t type, int32_t start, int32_t length,
                       int32_t addr, int32_t size_addr)
{
    return PROXY_RESULT_UNIMPLEMENTED;
}


int32_t
proxy_send_local_response(int32_t res_code,
                          int32_t res_code_details_data, int32_t res_code_details_size,
                          int32_t body, int32_t body_size,
                          int32_t headers, int32_t headers_size,
                          int32_t grpc_status)
{
    ngx_http_wasm_main_conf_t  *wmcf;
    ngx_http_request_t         *r;
    ngx_log_t                  *log;
    const u_char               *p;

    must_get_req(r);
    log = r->connection->log;

    wmcf = ngx_http_get_module_main_conf(r, ngx_http_wasm_module);
    wmcf->code = res_code;
    wmcf->body.len = body_size;

    if (headers_size > 0) {
        char               *key, *val;
        int32_t             key_len, val_len;
        ngx_int_t           rc;
        proxy_wasm_map_iter it;

        p = ngx_wasm_vm->get_memory(log, headers, headers_size);
        if (p == NULL) {
            return PROXY_RESULT_INVALID_MEMORY_ACCESS;
        }

        proxy_wasm_map_init_iter(&it, p);
        while (proxy_wasm_map_next(&it, &key, &key_len, &val, &val_len)) {
            rc = ngx_http_wasm_set_resp_header(r, key, key_len, 0, val, val_len, 0);
            if (rc != NGX_OK) {
                return PROXY_RESULT_BAD_ARGUMENT;
            }
        }
    }

    if (body_size > 0) {
        p = ngx_wasm_vm->get_memory(log, body, body_size);
        if (p == NULL) {
            return PROXY_RESULT_INVALID_MEMORY_ACCESS;
        }

        wmcf->body.data = ngx_palloc(r->pool, body_size);
        if (wmcf->body.data == NULL) {
            ngx_log_error(NGX_LOG_ERR, log, 0, "no memory");
            return PROXY_RESULT_INTERNAL_FAILURE;
        }
        ngx_memcpy(wmcf->body.data, p, body_size);
    }

    return PROXY_RESULT_OK;
}


int32_t
proxy_send_http_response(int32_t res_code,
                         int32_t res_code_details_data, int32_t res_code_details_size,
                         int32_t body, int32_t body_size,
                         int32_t headers, int32_t headers_size,
                         int32_t grpc_status)
{
    return proxy_send_local_response(res_code,
                                     res_code_details_data, res_code_details_size,
                                     body, body_size,
                                     headers, headers_size,
                                     grpc_status);
}


int32_t
proxy_get_current_time_nanoseconds(int32_t time_addr)
{
    return PROXY_RESULT_UNIMPLEMENTED;
}


int32_t
proxy_set_tick_period_milliseconds(int32_t tick)
{
    return PROXY_RESULT_UNIMPLEMENTED;
}


int32_t
proxy_get_configuration(int32_t addr, int32_t size_addr)
{
    int32_t               len;
    ngx_log_t            *log;
    const u_char         *data;
    const ngx_str_t      *conf;

    log = ngx_http_wasm_get_log();
    conf = ngx_http_wasm_get_conf();
    if (conf == NULL) {
        return PROXY_RESULT_NOT_FOUND;
    }

    data = conf->data;
    len = conf->len;

    return ngx_http_wasm_copy_to_wasm(log, data, len, addr, size_addr);
}


static ngx_str_t *
ngx_http_wasm_get_path(ngx_http_request_t *r)
{
    return &r->unparsed_uri;
}


static ngx_str_t *
ngx_http_wasm_get_method(ngx_http_request_t *r)
{
    return &r->method_name;
}


static ngx_str_t *ngx_http_wasm_get_scheme(ngx_http_request_t *r)
{
    if (r->connection->ssl) {
        return &scheme_https;
    }

    return &scheme_http;
}


static ngx_str_t *
ngx_http_wasm_get_status(ngx_http_request_t *r)
{
    ngx_uint_t  status;

    if (r->err_status) {
        status = r->err_status;
    } else {
        status = r->headers_out.status;
    }

    status_str.len = ngx_sprintf(status_str.data, "%ui", status) - status_str.data;

    return &status_str;
}


static ngx_str_t *
ngx_http_wasm_get_pseudo_header(ngx_http_request_t *r, u_char *key_data, size_t key_size, int ty) {
    ngx_str_t                 *h = NULL;
    ngx_uint_t                 i, entries;
    ngx_http_wasm_pseudo_header_t *wh, *whs;

    if (ty == PROXY_WASM_REQUEST_HEADER) {
        entries = (ngx_uint_t) PROXY_WASM_REQ_HEADER_STATIC_TABLE_ENTRIES;
        whs = &wasm_pseudo_req_header_static_table[0];

    } else if (ty == PROXY_WASM_RESPONSE_HEADER) {
        entries = (ngx_uint_t) PROXY_WASM_RESP_HEADER_STATIC_TABLE_ENTRIES;
        whs = &wasm_pseudo_resp_header_static_table[0];

    } else {
        return NULL;
    }


    if (key_size <= 0 || key_data[0] != ':') {
        return NULL;
    }

    for (i = 0; i < entries; i++) {
        wh = whs + i;

        if ((size_t) key_size != wh->name.len) {
            continue;
        }

        if (ngx_strncasecmp(key_data, wh->name.data, wh->name.len) == 0) {
            h = wh->getter(r);
            break;
        }
    }

    return h;
}


static ngx_str_t *
ngx_http_wasm_get_pseudo_req_header(ngx_http_request_t *r, u_char *key_data, size_t key_size) {
    return ngx_http_wasm_get_pseudo_header(r, key_data, key_size, PROXY_WASM_REQUEST_HEADER);
}


static ngx_str_t *
ngx_http_wasm_get_pseudo_resp_header(ngx_http_request_t *r, u_char *key_data, size_t key_size) {
    return ngx_http_wasm_get_pseudo_header(r, key_data, key_size, PROXY_WASM_RESPONSE_HEADER);
}


static int32_t
ngx_http_wasm_req_get_headers(ngx_http_request_t *r, int32_t addr, int32_t size_addr)
{
    ngx_int_t                  count, rc, i;
    int                        truncated;
    ngx_log_t                 *log;
    proxy_wasm_table_elt_t    *headers = NULL;
    int                        size = 0;
    u_char                    *buf;
    proxy_wasm_map_iter        it;
    char                      *key;
    char                      *val;
    ngx_str_t                 *s;

    ngx_http_wasm_pseudo_header_t *wh;

    log = r->connection->log;

    count = get_req_headers_count(r, -1, &truncated);

    if (count == FFI_BAD_CONTEXT) {
        ngx_log_error(NGX_LOG_ERR, log, 0,
                      "failed to get request headers: %s", err_bad_ctx);
        return PROXY_RESULT_BAD_ARGUMENT;
    }

    if (count > 0) {
        headers = ngx_http_wasm_get_string_buf(r->pool, count * sizeof(proxy_wasm_table_elt_t));
        if (headers == NULL) {
            return PROXY_RESULT_INTERNAL_FAILURE;
        }

        rc = get_req_headers(r, headers, count, 0);
        if (rc != NGX_OK) {
            return PROXY_RESULT_INTERNAL_FAILURE;
        }

        /* count the size */
        for (i = 0; i < count; i++) {
            /* proxy_map_t appends '\0' after each entry */
            size += headers[i].key.len + headers[i].value.len + 2;
        }
    }

    /* count pseudo headers :path, :method, :scheme */
    for (i = 0; i < (ngx_int_t) PROXY_WASM_REQ_HEADER_STATIC_TABLE_ENTRIES; i++) {
        wh = &wasm_pseudo_req_header_static_table[i];
        s = wh->getter(r);
        size += wh->name.len + 1 + s->len + 1;
    }

    size += 4 + (count + PROXY_WASM_REQ_HEADER_STATIC_TABLE_ENTRIES) * 2 * 4;
    buf = ngx_http_wasm_get_buf_to_write(log, size, addr, size_addr);
    if (buf == NULL) {
        return PROXY_RESULT_INVALID_MEMORY_ACCESS;
    }

    /* get the data */
    proxy_wasm_map_init_map(buf, count + PROXY_WASM_REQ_HEADER_STATIC_TABLE_ENTRIES);
    proxy_wasm_map_init_iter(&it, buf);

    for (i = 0; i < count; i++) {
        proxy_wasm_map_reserve(&it, &key, headers[i].key.len,
                               &val, headers[i].value.len);
        /* the header key is already lowercase */
        ngx_memcpy(key, headers[i].key.data, headers[i].key.len);
        ngx_memcpy(val, headers[i].value.data, headers[i].value.len);

        ngx_log_debug4(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "wasm request header: \"%*s: %*s\"",
                       headers[i].key.len, headers[i].key.data,
                       headers[i].value.len, headers[i].value.data);
    }

    /* get pseudo headers :path, :method, :scheme */
    for (i = 0; i < (ngx_int_t) PROXY_WASM_REQ_HEADER_STATIC_TABLE_ENTRIES; i++) {
        wh = &wasm_pseudo_req_header_static_table[i];
        s = wh->getter(r);

        proxy_wasm_map_reserve(&it, &key, wh->name.len,
                               &val, s->len);
        ngx_memcpy(key, wh->name.data, wh->name.len);
        ngx_memcpy(val, s->data, s->len);
    }

    return PROXY_RESULT_OK;
}


static int32_t
ngx_http_wasm_resp_get_headers(ngx_http_request_t *r, int32_t addr, int32_t size_addr)
{
    int                  count = 0;
    int                  size = 0;
    ngx_uint_t           i;
    u_char              *content_length_hdr = NULL;
    u_char               content_length_hdr_len = 0;
    u_char              *status_length_hdr = NULL;
    u_char               status_length_hdr_len = 0;
    char                *lowcase_key;
    char                *val;
    ngx_list_part_t     *part;
    ngx_table_elt_t     *header;
    ngx_log_t           *log;
    u_char              *buf;
    proxy_wasm_map_iter  it;

    log = r->connection->log;

    part = &r->headers_out.headers.part;
    header = part->elts;

    /* count the size */
    for (i = 0; /* void */; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }

            part = part->next;
            header = part->elts;
            i = 0;
        }

        if (header[i].hash == 0) {
            continue;
        }

        count++;
        /* proxy_map_t appends '\0' after each entry */
        size += header[i].key.len + header[i].value.len + 2;
    }

    if (r->headers_out.content_type.len) {
        count++;
        size += sizeof("content-type") + r->headers_out.content_type.len + 1;
    }

    if (r->headers_out.content_length == NULL
        && r->headers_out.content_length_n >= 0)
    {
        count++;
        content_length_hdr = ngx_palloc(r->pool, NGX_OFF_T_LEN);
        if (content_length_hdr == NULL) {
            ngx_log_error(NGX_LOG_ERR, log, 0, "no memory");
            return PROXY_RESULT_INTERNAL_FAILURE;
        }

        content_length_hdr_len = ngx_snprintf(content_length_hdr, NGX_OFF_T_LEN, "%O",
                                              r->headers_out.content_length_n) - content_length_hdr;

        size += sizeof("content-length") + content_length_hdr_len + 1;
    }

    count++; /* for connection header */
    size += sizeof("content-length");
    if (r->headers_out.status == NGX_HTTP_SWITCHING_PROTOCOLS) {
        size += sizeof("upgrade");

    } else if (r->keepalive) {
        size += sizeof("keep-alive");

    } else {
        size += sizeof("close");
    }

    if (r->chunked) {
        count++;
        size += sizeof("transfer-encoding") + sizeof("chunked");
    }

    if (r->headers_out.status) {
        count++;
        status_length_hdr = ngx_pcalloc(r->pool, NGX_INT_T_LEN);
        if (status_length_hdr == NULL) {
            ngx_log_error(NGX_LOG_ERR, log, 0, "no memory");
            return PROXY_RESULT_INTERNAL_FAILURE;
        }

        status_length_hdr_len = ngx_snprintf(status_length_hdr, NGX_INT_T_LEN, "%ui",
                                             r->headers_out.status) - status_length_hdr;

        size += sizeof(":status") + status_length_hdr_len + 1;
    }

    size += 4 + count * 2 * 4;
    buf = ngx_http_wasm_get_buf_to_write(log, size, addr, size_addr);
    if (buf == NULL) {
        return PROXY_RESULT_INVALID_MEMORY_ACCESS;
    }

    /* get the data */
    proxy_wasm_map_init_map(buf, count);
    proxy_wasm_map_init_iter(&it, buf);

    for (i = 0; /* void */; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }

            part = part->next;
            header = part->elts;
            i = 0;
        }

        if (header[i].hash == 0) {
            continue;
        }

        count++;

        /* nginx does not even bother initializing output header entry's
         * "lowcase_key" field. so we cannot count on that at all. */
        proxy_wasm_map_reserve(&it, &lowcase_key, header[i].key.len,
                               &val, header[i].value.len);
        ngx_strlow((u_char *) lowcase_key, header[i].key.data, header[i].key.len);
        ngx_memcpy(val, header[i].value.data, header[i].value.len);

        ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "wasm response header: \"%V: %V\"",
                       &header[i].key, &header[i].value);
    }

    if (r->headers_out.content_type.len) {
        proxy_wasm_map_reserve(&it, &lowcase_key, sizeof("content-type") - 1,
                               &val, r->headers_out.content_type.len);
        ngx_memcpy(lowcase_key, "content-type", sizeof("content-type") - 1);
        ngx_memcpy(val, r->headers_out.content_type.data, r->headers_out.content_type.len);
    }

    if (content_length_hdr != NULL) {
        proxy_wasm_map_reserve(&it, &lowcase_key, sizeof("content-length") - 1,
                               &val, content_length_hdr_len);
        ngx_memcpy(lowcase_key, "content-length", sizeof("content-length") - 1);
        ngx_memcpy(val, content_length_hdr, content_length_hdr_len);
    }

    if (r->headers_out.status == NGX_HTTP_SWITCHING_PROTOCOLS) {
        proxy_wasm_map_reserve_literal(&it, "connection", "upgrade");

    } else if (r->keepalive) {
        proxy_wasm_map_reserve_literal(&it, "connection", "keep-alive");

    } else {
        proxy_wasm_map_reserve_literal(&it, "connection", "close");
    }

    if (r->chunked) {
        proxy_wasm_map_reserve_literal(&it, "transfer-encoding", "chunked");
    }

    if (status_length_hdr != NULL) {
        proxy_wasm_map_reserve(&it, &lowcase_key, sizeof(":status") -1,
                               &val, status_length_hdr_len);
        ngx_memcpy(lowcase_key, ":status", sizeof(":status") - 1);
        ngx_memcpy(val, status_length_hdr, status_length_hdr_len);
    }

    return PROXY_RESULT_OK;
}


static int32_t
ngx_http_wasm_http_call_resp_get_headers(ngx_http_request_t *r, int32_t addr, int32_t size_addr)
{
    ngx_log_t               *log;
    ngx_uint_t               i;
    ngx_http_wasm_ctx_t     *ctx;
    proxy_wasm_table_elt_t  *hdr;
    int                      size = 0;
    u_char                  *buf;
    proxy_wasm_map_iter      it;

    log = r->connection->log;

    ctx = ngx_http_wasm_get_module_ctx(r);
    if (ctx->call_resp_n_header == 0) {
        return PROXY_RESULT_NOT_FOUND;
    }

    hdr = ctx->call_resp_headers;

    /* count the size */
    for (i = 0; i < ctx->call_resp_n_header; i++) {
        size += hdr[i].key.len + hdr[i].value.len + 2;
    }

    size += 4 + ctx->call_resp_n_header * 2 * 4;
    buf = ngx_http_wasm_get_buf_to_write(log, size, addr, size_addr);
    if (buf == NULL) {
        return PROXY_RESULT_INVALID_MEMORY_ACCESS;
    }

    /* get the data */
    proxy_wasm_map_init_map(buf, ctx->call_resp_n_header);
    proxy_wasm_map_init_iter(&it, buf);

    for (i = 0; i < ctx->call_resp_n_header; i++) {
        char *key, *val;

        proxy_wasm_map_reserve(&it, &key, hdr[i].key.len, &val, hdr[i].value.len);
        ngx_memcpy(key, hdr[i].key.data, hdr[i].key.len);
        ngx_memcpy(val, hdr[i].value.data, hdr[i].value.len);
    }

    return PROXY_RESULT_OK;
}


int32_t
proxy_get_header_map_pairs(int32_t type, int32_t addr, int32_t size_addr)
{
    ngx_http_request_t         *r;

    must_get_req(r);

    switch (type) {
    case PROXY_MAP_TYPE_HTTP_REQUEST_HEADERS:
        return ngx_http_wasm_req_get_headers(r, addr, size_addr);

    case PROXY_MAP_TYPE_HTTP_RESPONSE_HEADERS:
        return ngx_http_wasm_resp_get_headers(r, addr, size_addr);

    case PROXY_MAP_TYPE_HTTP_CALL_RESPONSE_HEADERS:
        return ngx_http_wasm_http_call_resp_get_headers(r, addr, size_addr);

    default:
        return PROXY_RESULT_UNIMPLEMENTED;
    }

    return PROXY_RESULT_OK;
}


int32_t
proxy_set_header_map_pairs(int32_t type, int32_t data, int32_t size)
{
    return PROXY_RESULT_UNIMPLEMENTED;
}


static int32_t
ngx_http_wasm_req_get_header(ngx_http_request_t *r, char *key,  int32_t key_size,
                             int32_t addr, int32_t size)
{
    ngx_log_t                 *log;
    ngx_uint_t                 i;
    ngx_str_t                 *ph;
    ngx_list_part_t           *part;
    ngx_table_elt_t           *header;
    unsigned char             *key_buf = NULL;
    const u_char              *val = NULL;
    int32_t                    val_len = 0;

    log = r->connection->log;
    part = &r->headers_in.headers.part;
    header = part->elts;
    key_buf = (u_char *) key;

    ph = ngx_http_wasm_get_pseudo_req_header(r, key_buf, key_size);
    if (ph) {
        val = ph->data;
        val_len = ph->len;
        goto done;
    }

    for (i = 0; /* void */ ; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }

            part = part->next;
            header = part->elts;
            i = 0;
        }

        if (header[i].hash == 0) {
            continue;
        }

        if ((size_t) key_size != header[i].key.len) {
            continue;
        }

        if (ngx_strncasecmp(key_buf, header[i].key.data, header[i].key.len) == 0) {
            val = header[i].value.data;
            val_len = header[i].value.len;
            break;
        }
    }

done:

    if (val == NULL) {
        return PROXY_RESULT_NOT_FOUND;
    }

    return ngx_http_wasm_copy_to_wasm(log, val, val_len, addr, size);
}


static int32_t
ngx_http_wasm_resp_get_header(ngx_http_request_t *r, char *key,  int32_t key_size,
                              int32_t addr, int32_t size)
{
    ngx_int_t            rc;
    char                *errmsg = NULL;
    ngx_log_t           *log;
    unsigned char       *key_buf;
    ngx_str_t           *values, *ph;


    log = r->connection->log;

    key_buf = ngx_http_wasm_get_string_buf(r->pool, key_size + sizeof(ngx_str_t));
    if (key_buf == NULL) {
        return PROXY_RESULT_INTERNAL_FAILURE;
    }

    values = (ngx_str_t *) (key_buf + key_size);

    ph = ngx_http_wasm_get_pseudo_resp_header(r, (u_char *) key, key_size);
    if (ph) {
        values = ph;
        goto done;
    }

    rc = get_resp_header(r, (unsigned char *) key, key_size, key_buf, values, 1, &errmsg);
    if (rc < 0) {
        if (rc == FFI_BAD_CONTEXT) {
            errmsg = err_bad_ctx;
        }

        if (errmsg != NULL) {
            ngx_log_error(NGX_LOG_ERR, log, 0, "failed to get response header %*s: %s",
                          key_size, key, errmsg);
        }

        return PROXY_RESULT_BAD_ARGUMENT;
    }

    if (rc == 0) {
        /* not found */
        return PROXY_RESULT_NOT_FOUND;
    }

done:

    return ngx_http_wasm_copy_to_wasm(log, values->data, values->len, addr, size);
}


static int32_t
ngx_http_wasm_http_call_resp_get_header(ngx_http_request_t *r, char *key,  int32_t key_size,
                                        int32_t addr, int32_t size)
{
    ngx_log_t                  *log;
    ngx_uint_t                  i;
    ngx_http_wasm_ctx_t        *ctx;
    proxy_wasm_table_elt_t     *hdr;
    const u_char               *val = NULL;
    int32_t                     val_len = 0;

    log = r->connection->log;

    ctx = ngx_http_wasm_get_module_ctx(r);
    if (ctx->call_resp_n_header == 0) {
        return PROXY_RESULT_NOT_FOUND;
    }

    hdr = ctx->call_resp_headers;

    for (i = 0; i < ctx->call_resp_n_header; i++) {
        if (key_size != hdr[i].key.len) {
            continue;
        }

        if (ngx_strncasecmp((u_char *) key, hdr[i].key.data, hdr[i].key.len) == 0) {
            val = hdr[i].value.data;
            val_len = hdr[i].value.len;
            break;
        }
    }

    if (val == NULL) {
        return PROXY_RESULT_NOT_FOUND;
    }

    return ngx_http_wasm_copy_to_wasm(log, val, val_len, addr, size);
}


int32_t
proxy_get_header_map_value(int32_t type, int32_t key_data, int32_t key_size,
                           int32_t addr, int32_t size)
{
    ngx_http_request_t         *r;
    char                       *key;

    must_get_req(r);
    must_get_memory(key, r->connection->log, key_data, key_size);

    switch (type) {
    case PROXY_MAP_TYPE_HTTP_REQUEST_HEADERS:
        return ngx_http_wasm_req_get_header(r, key, key_size, addr, size);

    case PROXY_MAP_TYPE_HTTP_RESPONSE_HEADERS:
        return ngx_http_wasm_resp_get_header(r, key, key_size, addr, size);

    case PROXY_MAP_TYPE_HTTP_CALL_RESPONSE_HEADERS:
        return ngx_http_wasm_http_call_resp_get_header(r, key, key_size, addr, size);

    default:
        return PROXY_RESULT_UNIMPLEMENTED;
    }

    return PROXY_RESULT_OK;
}


int32_t
proxy_remove_header_map_value(int32_t type, int32_t key_data, int32_t key_size)
{
    ngx_int_t                   rc;
    ngx_log_t                  *log;
    ngx_http_request_t         *r;
    char                       *key;

    log = ngx_http_wasm_get_log();
    must_get_req(r);
    must_get_memory(key, log, key_data, key_size);

    switch (type) {
    case PROXY_MAP_TYPE_HTTP_REQUEST_HEADERS:
        rc = ngx_http_wasm_set_req_header(r, key, key_size, NULL, 0, 1);
        break;

    case PROXY_MAP_TYPE_HTTP_RESPONSE_HEADERS:
        rc = ngx_http_wasm_set_resp_header(r, key, key_size, 1, NULL, 0, 1);
        break;

    default:
        return PROXY_RESULT_UNIMPLEMENTED;
    }

    if (rc != NGX_OK) {
        return PROXY_RESULT_BAD_ARGUMENT;
    }

    return PROXY_RESULT_OK;
}


int32_t
proxy_replace_header_map_value(int32_t type, int32_t key_data, int32_t key_size,
                               int32_t data, int32_t size)
{
    ngx_int_t                   rc;
    ngx_log_t                  *log;
    ngx_http_request_t         *r;
    char                       *key, *val;

    log = ngx_http_wasm_get_log();
    must_get_req(r);
    must_get_memory(key, log, key_data, key_size);
    must_get_memory(val, log, data, size);

    switch (type) {
    case PROXY_MAP_TYPE_HTTP_REQUEST_HEADERS:
        rc = ngx_http_wasm_set_req_header(r, key, key_size, val, size, 1);
        break;

    case PROXY_MAP_TYPE_HTTP_RESPONSE_HEADERS:
        rc = ngx_http_wasm_set_resp_header(r, key, key_size, 0, val, size, 1);
        break;

    default:
        return PROXY_RESULT_UNIMPLEMENTED;
    }

    if (rc != NGX_OK) {
        return PROXY_RESULT_BAD_ARGUMENT;
    }

    return PROXY_RESULT_OK;
}


int32_t
proxy_add_header_map_value(int32_t type, int32_t key_data, int32_t key_size,
                           int32_t data, int32_t size)
{
    ngx_int_t                   rc;
    ngx_log_t                  *log;
    ngx_http_request_t         *r;
    char                       *key, *val;

    log = ngx_http_wasm_get_log();
    must_get_req(r);
    must_get_memory(key, log, key_data, key_size);
    must_get_memory(val, log, data, size);

    switch (type) {
    case PROXY_MAP_TYPE_HTTP_REQUEST_HEADERS:
        rc = ngx_http_wasm_set_req_header(r, key, key_size, val, size, 0);
        break;

    case PROXY_MAP_TYPE_HTTP_RESPONSE_HEADERS:
        rc = ngx_http_wasm_set_resp_header(r, key, key_size, 0, val, size, 0);
        break;

    default:
        return PROXY_RESULT_UNIMPLEMENTED;
    }


    if (rc != NGX_OK) {
        return PROXY_RESULT_BAD_ARGUMENT;
    }

    return PROXY_RESULT_OK;
}


int32_t
proxy_get_shared_data(int32_t key_data, int32_t key_size,
                      int32_t addr, int32_t size,
                      int32_t cas)
{
    return PROXY_RESULT_UNIMPLEMENTED;
}


int32_t
proxy_set_shared_data(int32_t key_data, int32_t key_size,
                      int32_t data, int32_t size,
                      int32_t cas)
{
    return PROXY_RESULT_UNIMPLEMENTED;
}


int32_t
proxy_register_shared_queue(int32_t data, int32_t size, int32_t id)
{
    return PROXY_RESULT_UNIMPLEMENTED;
}


int32_t
proxy_resolve_shared_queue(int32_t vm_id_data, int32_t vm_id_size,
                           int32_t name_data, int32_t name_size,
                           int32_t return_id)
{
    return PROXY_RESULT_UNIMPLEMENTED;
}


int32_t
proxy_dequeue_shared_queue(int32_t id, int32_t addr, int32_t size)
{
    return PROXY_RESULT_UNIMPLEMENTED;
}


int32_t
proxy_enqueue_shared_queue(int32_t id, int32_t addr, int32_t size)
{
    return PROXY_RESULT_UNIMPLEMENTED;
}


int32_t
proxy_continue_request(void)
{
    return PROXY_RESULT_OK;
}


int32_t
proxy_continue_response(void)
{
    return PROXY_RESULT_OK;
}


int32_t
proxy_clear_route_cache(void)
{
    return PROXY_RESULT_UNIMPLEMENTED;
}


int32_t
proxy_continue_stream(int32_t stream_type)
{
    /* we don't need to continue the HTTP request as it always waits the HTTP call to finish */
    return PROXY_RESULT_OK;
}


int32_t
proxy_close_stream(int32_t stream_type)
{
    /* do nothing */
    return PROXY_RESULT_OK;
}


static ngx_str_t *
ngx_http_wasm_copy_as_str(ngx_http_request_t *r, char *src, size_t size)
{
    ngx_str_t       *s;

    s = ngx_palloc(r->pool, sizeof(ngx_str_t) + size);
    if (s == NULL) {
        return NULL;
    }

    s->data = (u_char *) (s + 1);
    ngx_memcpy(s->data, src, size);
    s->len = size;

    return s;
}


int32_t
proxy_http_call(int32_t up_data, int32_t up_size,
                int32_t headers_data, int32_t headers_size,
                int32_t body_data, int32_t body_size,
                int32_t trailer_data, int32_t trailer_size,
                int32_t timeout, int32_t callout_addr)
{
    ngx_int_t        rc;
    char            *p;
    ngx_str_t       *up;
    u_char          *headers = NULL;
    ngx_str_t       *body = NULL;
    uint32_t         callout_id;
    uint32_t        *p_callout;

    ngx_http_request_t *r;
    ngx_log_t          *log;
    ngx_url_t           url;

    must_get_req(r);
    log = r->connection->log;

    if (!ngx_http_wasm_is_yieldable(r)) {
        ngx_log_error(NGX_LOG_ERR, log, 0,
                      "http call is only supported during processing HTTP request");
        return PROXY_RESULT_BAD_ARGUMENT;
    }

    must_get_memory(p, log, up_data, up_size);

    up = ngx_http_wasm_copy_as_str(r, p, up_size);
    if (up == NULL) {
        return PROXY_RESULT_INTERNAL_FAILURE;
    }

    ngx_memzero(&url, sizeof(ngx_url_t));
    url.url = *up;
    url.no_resolve = 1;

    if (ngx_parse_url(r->pool, &url) != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, log, 0, "http call gets invalid host: %V", up);
        return PROXY_RESULT_BAD_ARGUMENT;
    }

    if (headers_size > 0) {
        must_get_memory(p, log, headers_data, headers_size);

        headers = ngx_palloc(r->pool, headers_size);
        if (headers == NULL) {
            return PROXY_RESULT_INTERNAL_FAILURE;
        }

        ngx_memcpy(headers, p, headers_size);
    }

    if (body_size > 0) {
        must_get_memory(p, log, body_data, body_size);

        body = ngx_http_wasm_copy_as_str(r, p, body_size);
        if (body == NULL) {
            return PROXY_RESULT_INTERNAL_FAILURE;
        }
    }

    /* TODO: handle trailer */

    rc = ngx_http_wasm_call_register(r, up, headers, body, timeout, &callout_id);
    if (rc != NGX_OK) {
        return PROXY_RESULT_INTERNAL_FAILURE;
    }

    p_callout = (uint32_t *) ngx_wasm_vm->get_memory(log, callout_addr, sizeof(callout_id));
    if (p_callout == NULL) {
        return PROXY_RESULT_INVALID_MEMORY_ACCESS;
    }

    *p_callout = callout_id;
    return PROXY_RESULT_OK;
}


int32_t
proxy_grpc_call(int32_t service_data, int32_t service_size,
                int32_t service_name_data, int32_t service_name_size,
                int32_t method_data, int32_t method_size,
                int32_t metadata_data, int32_t metadata_size,
                int32_t request_data, int32_t request_size,
                int32_t timeout, int32_t callout_addr)
{
    return PROXY_RESULT_UNIMPLEMENTED;
}


int32_t
proxy_grpc_stream(int32_t service_data, int32_t service_size,
                  int32_t service_name_data, int32_t service_name_size,
                  int32_t method_data, int32_t method_size,
                  int32_t metadata_data, int32_t metadata_size,
                  int32_t callout_addr)
{
    return PROXY_RESULT_UNIMPLEMENTED;
}


int32_t
proxy_grpc_send(int32_t id, int32_t message_data, int32_t message_size,
                int32_t end_of_stream)
{
    return PROXY_RESULT_UNIMPLEMENTED;
}


int32_t
proxy_grpc_cancel(int32_t id)
{
    return PROXY_RESULT_UNIMPLEMENTED;
}


int32_t
proxy_grpc_close(int32_t id)
{
    return PROXY_RESULT_UNIMPLEMENTED;
}


int32_t
proxy_get_status(int32_t code, int32_t addr, int32_t size)
{
    return PROXY_RESULT_UNIMPLEMENTED;
}


int32_t
proxy_done(void)
{
    return PROXY_RESULT_UNIMPLEMENTED;
}


int32_t
proxy_call_foreign_function(int32_t fn_data, int32_t fn_size,
                            int32_t param_data, int32_t param_size,
                            int32_t res_data, int32_t res_size)
{
    return PROXY_RESULT_UNIMPLEMENTED;
}

int32_t
proxy_define_metric(int32_t type, int32_t name_data, int32_t name_size,
                    int32_t result)
{
    return PROXY_RESULT_UNIMPLEMENTED;
}

int32_t
proxy_increment_metric(int32_t metric_id, int64_t offset)
{
    return PROXY_RESULT_UNIMPLEMENTED;
}

int32_t
proxy_record_metric(int32_t metric_id, int64_t value)
{
    return PROXY_RESULT_UNIMPLEMENTED;
}

int32_t
proxy_get_metric(int32_t metric_id, int64_t result)
{
    return PROXY_RESULT_UNIMPLEMENTED;
}
