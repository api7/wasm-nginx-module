#ifndef NGX_HTTP_WASM_API_H
#define NGX_HTTP_WASM_API_H


#include <wasm.h>
#include <wasmtime.h>
#include <ngx_core.h>
#include "ngx_http_wasm_types.h"


#define DEFINE_WASM_API(NAME, ARG, ARG_CHECK) \
    int32_t NAME(ARG); \
    static wasm_trap_t* wasmtime_##NAME( \
        void *env, \
        wasmtime_caller_t *caller, \
        const wasmtime_val_t *args, \
        size_t nargs, \
        wasmtime_val_t *results, \
        size_t nresults \
    ) { \
        ARG_CHECK \
        results[0].kind = WASMTIME_I32; \
        results[0].of.i32 = res; \
        return NULL; \
    }

#define DEFINE_WASM_API_ARG_I32 \
    int32_t

#define DEFINE_WASM_API_ARG_CHECK_I32(NAME) \
    int32_t p1 = args[0].of.i32; \
    int32_t res = NAME(p1);

#define DEFINE_WASM_API_ARG_I32_I32_I32 \
    int32_t, int32_t, int32_t

#define DEFINE_WASM_API_ARG_CHECK_I32_I32_I32(NAME) \
    int32_t p1 = args[0].of.i32; \
    int32_t p2 = args[1].of.i32; \
    int32_t p3 = args[2].of.i32; \
    int32_t res = NAME(p1, p2, p3);

#define DEFINE_WASM_NAME(NAME, ARG) \
    {ngx_string(#NAME), wasmtime_##NAME, ARG},
#define DEFINE_WASM_NAME_ARG_I32 \
    1, {WASM_I32}
#define DEFINE_WASM_NAME_ARG_I32_I32_I32 \
    3, {WASM_I32, WASM_I32, WASM_I32}


typedef struct {
    ngx_str_t                name;
    wasmtime_func_callback_t cb;
    int8_t                   param_num;
    wasm_valkind_t           param_type[3];
} ngx_wasm_host_api_t;


DEFINE_WASM_API(proxy_set_effective_context,
                DEFINE_WASM_API_ARG_I32,
                DEFINE_WASM_API_ARG_CHECK_I32(proxy_set_effective_context))
DEFINE_WASM_API(proxy_log,
                DEFINE_WASM_API_ARG_I32_I32_I32,
                DEFINE_WASM_API_ARG_CHECK_I32_I32_I32(proxy_log))


static ngx_wasm_host_api_t host_apis[] = {
    DEFINE_WASM_NAME(proxy_set_effective_context, DEFINE_WASM_NAME_ARG_I32)
    DEFINE_WASM_NAME(proxy_log, DEFINE_WASM_NAME_ARG_I32_I32_I32)
    { ngx_null_string, NULL, 0, {} }
};


wasm_functype_t *ngx_http_wasm_host_api_func(const ngx_wasm_host_api_t *api);


#endif // NGX_HTTP_WASM_API_H
