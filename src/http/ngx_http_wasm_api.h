#ifndef NGX_HTTP_WASM_API_H
#define NGX_HTTP_WASM_API_H


#include <wasm.h>
#include <wasmtime.h>
#include <ngx_core.h>
#include "vm/vm.h"
#include "ngx_http_wasm_types.h"


static wasm_trap_t* proxy_set_effective_context(
    void *env,
    wasmtime_caller_t *caller,
    const wasmtime_val_t *args,
    size_t nargs,
    wasmtime_val_t *results,
    size_t nresults
) {
    /* dummy function to make plugin load */
    return NULL;
}


static wasm_trap_t* proxy_log(
    void *env,
    wasmtime_caller_t *caller,
    const wasmtime_val_t *args,
    size_t nargs,
    wasmtime_val_t *results,
    size_t nresults
) {
    int32_t log_level = args[0].of.i32;
    int32_t addr = args[1].of.i32;
    int32_t size = args[2].of.i32;
    const u_char *p;
    ngx_uint_t host_log_level = NGX_LOG_ERR;

    p = ngx_wasm_vm.get_memory(ngx_cycle->log, addr, size);
    if (p == NULL) {
      results[0].kind = WASMTIME_I32;
      results[0].of.i32 = PROXY_RESULT_INVALID_MEMORY_ACCESS;
      return NULL;
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

    ngx_log_error(host_log_level, ngx_cycle->log, 0, "%*s", size, p);

    results[0].kind = WASMTIME_I32;
    results[0].of.i32 = PROXY_RESULT_OK;
    return NULL;
}


typedef struct {
    ngx_str_t                name;
    wasmtime_func_callback_t cb;
    int8_t                   param_num;
    wasm_valkind_t           param_type[3];
} ngx_wasm_host_api_t;


static ngx_wasm_host_api_t host_apis[] = {
    { ngx_string("proxy_set_effective_context"),
      proxy_set_effective_context,
      1, {WASM_I32}
    },
    { ngx_string("proxy_log"),
      proxy_log,
      3, {WASM_I32, WASM_I32, WASM_I32}
    },
    { ngx_null_string, NULL, 0, {} }
};


wasm_functype_t *ngx_http_wasm_host_api_func(const ngx_wasm_host_api_t *api);


#endif // NGX_HTTP_WASM_API_H
