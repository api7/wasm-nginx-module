#ifndef NGX_HTTP_WASM_API_H
#define NGX_HTTP_WASM_API_H


#include <wasm.h>
#include <wasmtime.h>
#include <ngx_core.h>


//typedef own wasm_trap_t* (*wasm_func_callback_with_env_t)(
  //void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results);
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


/* a zero wasm_valkind_t is valid value, so we have to use -1 to represent void */
#define WASM_VOID  -1


typedef struct {
  ngx_str_t                name;
  wasmtime_func_callback_t cb;
  int8_t                   ret_type;
  int8_t                   param_num;
  wasm_valkind_t           param_type[3];
} ngx_wasm_host_api_t;


static ngx_wasm_host_api_t host_apis[] = {
  { ngx_string("proxy_set_effective_context"),
    proxy_set_effective_context,
    WASM_I32,
    1, {WASM_I32}
  },
  { ngx_null_string, NULL, WASM_VOID, 0, {} }
};


wasm_functype_t *ngx_http_wasm_host_api_func(const ngx_wasm_host_api_t *api);


#endif // NGX_HTTP_WASM_API_H
