#define _GNU_SOURCE /* for RTLD_DEFAULT */
#include <dlfcn.h>
#include "vm/vm.h"
#include "ngx_http_wasm_api.h"
#include "ngx_http_wasm_module.h"
#include "ngx_http_wasm_state.h"
#include "ngx_http_wasm_map.h"


static int (*set_resp_header) (ngx_http_request_t *r,
    const char *key_data, size_t key_len, int is_nil,
    const char *sval, size_t sval_len, ngx_str_t *mvals,
    size_t mvals_len, int override, char **errmsg);


wasm_functype_t *
ngx_http_wasm_host_api_func(const ngx_wasm_host_api_t *api)
{
    int                i;
    wasm_valtype_vec_t param_vec, result_vec;
    wasm_valtype_t    *param[MAX_WASM_API_ARG];
    wasm_valtype_t    *result[1];
    wasm_functype_t   *f;

    for (i = 0; i < api->param_num; i++) {
        param[i] = wasm_valtype_new(api->param_type[i]);
    }

    result[0] = wasm_valtype_new(WASM_I32);
    wasm_valtype_vec_new(&param_vec, api->param_num, param);
    wasm_valtype_vec_new(&result_vec, 1, result);

    f = wasm_functype_new(&param_vec, &result_vec);

    for (i = 0; i < api->param_num; i++) {
        wasm_valtype_delete(param[i]);
    }

    wasm_valtype_delete(result[0]);

    return f;
}


ngx_int_t
ngx_http_wasm_resolve_symbol(void)
{
    char        *err;

    dlerror();    /* Clear any existing error */

    set_resp_header = dlsym(RTLD_DEFAULT, "ngx_http_lua_ffi_set_resp_header");
    err = dlerror();
    if (err != NULL) {
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0,
                      "failed to resolve symbol: %s", err);
        return NGX_ERROR;
    }

    return NGX_OK;
}


int32_t
proxy_set_effective_context(int32_t id)
{
    /* dummy function to make plugin load */
    return PROXY_RESULT_OK;
}


static int32_t
ngx_http_wasm_copy_to_wasm(ngx_log_t *log, const u_char *data, int32_t len,
                           int32_t addr, int32_t size_addr)
{
    int32_t         buf_addr;
    int32_t        *p_size;
    int32_t        *p;
    u_char         *buf;

    buf_addr = ngx_wasm_vm.malloc(log, len);
    if (buf_addr == 0) {
        return PROXY_RESULT_INTERNAL_FAILURE;
    }

    buf = (u_char *) ngx_wasm_vm.get_memory(log, buf_addr, len);
    if (buf == NULL) {
        return PROXY_RESULT_INVALID_MEMORY_ACCESS;
    }

    ngx_memcpy(buf, data, len);

    p_size = (int32_t *) ngx_wasm_vm.get_memory(log, size_addr, sizeof(int32_t));
    if (p_size == NULL) {
        return PROXY_RESULT_INVALID_MEMORY_ACCESS;
    }

    *p_size = len;

    p = (int32_t *) ngx_wasm_vm.get_memory(log, addr, sizeof(int32_t));
    if (p == NULL) {
        return PROXY_RESULT_INVALID_MEMORY_ACCESS;
    }

    *p = buf_addr;
    return PROXY_RESULT_OK;
}


int32_t
proxy_get_property(int32_t path_data, int32_t path_size,
                   int32_t res_data, int32_t res_size)
{
    ngx_log_t          *log;
    const u_char       *p;

    log = ngx_http_wasm_get_log();

    p = ngx_wasm_vm.get_memory(log, path_data, path_size);
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

    return PROXY_RESULT_OK;
}


int32_t
proxy_set_property(int32_t path_data, int32_t path_size,
                   int32_t data, int32_t size)
{
    return PROXY_RESULT_OK;
}


int32_t
proxy_log(int32_t log_level, int32_t addr, int32_t size)
{
    const u_char       *p;
    ngx_uint_t          host_log_level = NGX_LOG_ERR;
    ngx_log_t          *log;

    log = ngx_http_wasm_get_log();

    p = ngx_wasm_vm.get_memory(log, addr, size);
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
proxy_get_buffer_bytes(int32_t type, int32_t start, int32_t length,
                       int32_t addr, int32_t size_addr)
{
    ngx_log_t      *log;
    const u_char   *data;
    int32_t         len;

    const ngx_str_t      *conf;
    ngx_http_request_t   *r;

    r = ngx_http_wasm_get_req();
    log = ngx_http_wasm_get_log();

    switch (type) {
    case PROXY_BUFFER_TYPE_PLUGIN_CONFIGURATION:
        conf = ngx_http_wasm_get_conf();
        data = conf->data;
        len = conf->len;
        break;

    default:
        return PROXY_RESULT_UNIMPLEMENTED;
    }

    return ngx_http_wasm_copy_to_wasm(log, data, len, addr, size_addr);
}


int32_t
proxy_set_buffer_bytes(int32_t type, int32_t start, int32_t length,
                       int32_t addr, int32_t size_addr)
{
    return PROXY_RESULT_OK;
}


int32_t
proxy_send_http_response(int32_t res_code,
                         int32_t res_code_details_data, int32_t res_code_details_size,
                         int32_t body, int32_t body_size,
                         int32_t headers, int32_t headers_size,
                         int32_t grpc_status)
{
    ngx_http_wasm_main_conf_t  *wmcf;
    ngx_http_request_t         *r;
    ngx_log_t                  *log;
    const u_char               *p;

    r = ngx_http_wasm_get_req();
    if (r == NULL) {
        return PROXY_RESULT_BAD_ARGUMENT;
    }

    log = r->connection->log;

    wmcf = ngx_http_get_module_main_conf(r, ngx_http_wasm_module);
    wmcf->code = res_code;
    wmcf->body.len = body_size;

    if (headers_size > 0) {
        char               *errmsg = NULL;
        char               *key, *val;
        int32_t             key_len, val_len;
        ngx_int_t           rc;
        proxy_wasm_map_iter it;

        p = ngx_wasm_vm.get_memory(log, headers, headers_size);
        if (p == NULL) {
            return PROXY_RESULT_INVALID_MEMORY_ACCESS;
        }

        ngx_http_wasm_map_init_iter(&it, p);
        while (ngx_http_wasm_map_next(&it, &key, &key_len, &val, &val_len)) {
            rc = set_resp_header(r, key, key_len, 0, val, val_len, NULL, 0,
                                 0, &errmsg);
            if (rc != NGX_OK && rc != NGX_DECLINED) {
                if (errmsg != NULL) {
                    ngx_log_error(NGX_LOG_ERR, log, 0,
                                  "faied to set header %*s to %*s: %s",
                                  key_len, key, val_len, val, errmsg);
                }

                return PROXY_RESULT_BAD_ARGUMENT;
            }
        }
    }

    if (body_size > 0) {
        p = ngx_wasm_vm.get_memory(log, body, body_size);
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
proxy_get_current_time_nanoseconds(int32_t time_addr)
{
    return PROXY_RESULT_OK;
}


int32_t
proxy_set_tick_period_milliseconds(int32_t tick)
{
    return PROXY_RESULT_OK;
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
    data = conf->data;
    len = conf->len;

    return ngx_http_wasm_copy_to_wasm(log, data, len, addr, size_addr);
}


int32_t
proxy_get_header_map_pairs(int32_t type, int32_t addr, int32_t size)
{
    return PROXY_RESULT_OK;
}


int32_t
proxy_set_header_map_pairs(int32_t type, int32_t data, int32_t size)
{
    return PROXY_RESULT_OK;
}


int32_t
proxy_get_header_map_value(int32_t type, int32_t key_data, int32_t key_size,
                           int32_t addr, int32_t size)
{
    return PROXY_RESULT_OK;
}


int32_t
proxy_remove_header_map_value(int32_t type, int32_t key_data, int32_t key_size)
{
    return PROXY_RESULT_OK;
}


int32_t
proxy_replace_header_map_value(int32_t type, int32_t key_data, int32_t key_size,
                               int32_t data, int32_t size)
{
    return PROXY_RESULT_OK;
}


int32_t
proxy_add_header_map_value(int32_t type, int32_t key_data, int32_t key_size,
                           int32_t data, int32_t size)
{
    return PROXY_RESULT_OK;
}


int32_t
proxy_get_shared_data(int32_t key_data, int32_t key_size,
                      int32_t addr, int32_t size,
                      int32_t cas)
{
    return PROXY_RESULT_OK;
}


int32_t
proxy_set_shared_data(int32_t key_data, int32_t key_size,
                      int32_t data, int32_t size,
                      int32_t cas)
{
    return PROXY_RESULT_OK;
}


int32_t
proxy_register_shared_queue(int32_t data, int32_t size, int32_t id)
{
    return PROXY_RESULT_OK;
}


int32_t
proxy_resolve_shared_queue(int32_t vm_id_data, int32_t vm_id_size,
                           int32_t name_data, int32_t name_size,
                           int32_t return_id)
{
    return PROXY_RESULT_OK;
}


int32_t
proxy_dequeue_shared_queue(int32_t id, int32_t addr, int32_t size)
{
    return PROXY_RESULT_OK;
}


int32_t
proxy_enqueue_shared_queue(int32_t id, int32_t addr, int32_t size)
{
    return PROXY_RESULT_OK;
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
    return PROXY_RESULT_OK;
}


int32_t
proxy_http_call(int32_t up_data, int32_t up_size,
                int32_t headers_data, int32_t headers_size,
                int32_t body_data, int32_t body_size,
                int32_t trailer_data, int32_t trailer_size,
                int32_t timeout, int32_t callout_addr)
{
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
    return PROXY_RESULT_OK;
}


int32_t
proxy_grpc_stream(int32_t service_data, int32_t service_size,
                  int32_t service_name_data, int32_t service_name_size,
                  int32_t method_data, int32_t method_size,
                  int32_t metadata_data, int32_t metadata_size,
                  int32_t callout_addr)
{
    return PROXY_RESULT_OK;
}


int32_t
proxy_grpc_send(int32_t id, int32_t message_data, int32_t message_size,
                int32_t end_of_stream)
{
    return PROXY_RESULT_OK;
}


int32_t
proxy_grpc_cancel(int32_t id)
{
    return PROXY_RESULT_OK;
}


int32_t
proxy_grpc_close(int32_t id)
{
    return PROXY_RESULT_OK;
}


int32_t
proxy_get_status(int32_t code, int32_t addr, int32_t size)
{
    return PROXY_RESULT_OK;
}


int32_t
proxy_done(void)
{
    return PROXY_RESULT_OK;
}
