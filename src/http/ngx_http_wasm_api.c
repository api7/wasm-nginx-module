#include "vm/vm.h"
#include "ngx_http_wasm_api.h"
#include "ngx_http_wasm_state.h"


/* convert Word to pointer */
#define WP(x)  (void *) (int64_t) (x)


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


int32_t
proxy_set_effective_context(int32_t id)
{
    /* dummy function to make plugin load */
    return PROXY_RESULT_OK;
}


int32_t
proxy_log(int32_t log_level, int32_t addr, int32_t size)
{
    const u_char       *p;
    ngx_uint_t          host_log_level = NGX_LOG_ERR;
    ngx_http_request_t *r;
    ngx_log_t          *log;

    r = ngx_http_wasm_get_req();
    if (r == NULL) {
        log = ngx_cycle->log;
    } else {
        log = r->connection->log;
    }

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
    int32_t         buf_addr;
    const u_char   *data;
    int32_t         len;
    int32_t        *p_size;
    u_char        **p;
    u_char         *buf;

    const ngx_str_t      *conf;
    ngx_http_request_t   *r;

    r = ngx_http_wasm_get_req();
    if (r == NULL) {
        log = ngx_cycle->log;
    } else {
        log = r->connection->log;
    }

    switch (type) {
    case PROXY_BUFFER_TYPE_PLUGIN_CONFIGURATION:
        conf = ngx_http_wasm_get_conf();
        data = conf->data;
        len = conf->len;
        break;

    default:
        return PROXY_RESULT_UNIMPLEMENTED;
    }

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

    p = (u_char **) ngx_wasm_vm.get_memory(log, addr, sizeof(int32_t));
    if (p == NULL) {
        return PROXY_RESULT_INVALID_MEMORY_ACCESS;
    }

    *p = WP(buf_addr);

    return PROXY_RESULT_OK;
}
