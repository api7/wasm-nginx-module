#ifndef NGX_HTTP_WASM_API_H
#define NGX_HTTP_WASM_API_H


#include <wasm.h>
#include <wasmtime.h>
#include <ngx_core.h>
#include "proxy_wasm/proxy_wasm_types.h"


#define MAX_WASM_API_ARG  12


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

#define DEFINE_WASM_API_ARG_VOID \
    void

#define DEFINE_WASM_API_ARG_CHECK_VOID(NAME) \
    int32_t res = NAME();

#define DEFINE_WASM_API_ARG_I32 \
    int32_t

#define DEFINE_WASM_API_ARG_CHECK_I32(NAME) \
    int32_t p1 = args[0].of.i32; \
    int32_t res = NAME(p1);

#define DEFINE_WASM_API_ARG_I32_I32 \
    int32_t, int32_t

#define DEFINE_WASM_API_ARG_CHECK_I32_I32(NAME) \
    int32_t p1 = args[0].of.i32; \
    int32_t p2 = args[1].of.i32; \
    int32_t res = NAME(p1, p2);

#define DEFINE_WASM_API_ARG_I32_I32_I32 \
    int32_t, int32_t, int32_t

#define DEFINE_WASM_API_ARG_CHECK_I32_I32_I32(NAME) \
    int32_t p1 = args[0].of.i32; \
    int32_t p2 = args[1].of.i32; \
    int32_t p3 = args[2].of.i32; \
    int32_t res = NAME(p1, p2, p3);

#define DEFINE_WASM_API_ARG_I32_I32_I32_I32 \
    int32_t, int32_t, int32_t, int32_t

#define DEFINE_WASM_API_ARG_CHECK_I32_I32_I32_I32(NAME) \
    int32_t p1 = args[0].of.i32; \
    int32_t p2 = args[1].of.i32; \
    int32_t p3 = args[2].of.i32; \
    int32_t p4 = args[3].of.i32; \
    int32_t res = NAME(p1, p2, p3, p4);

#define DEFINE_WASM_API_ARG_I32_I32_I32_I32_I32 \
    int32_t, int32_t, int32_t, int32_t, int32_t

#define DEFINE_WASM_API_ARG_CHECK_I32_I32_I32_I32_I32(NAME) \
    int32_t p1 = args[0].of.i32; \
    int32_t p2 = args[1].of.i32; \
    int32_t p3 = args[2].of.i32; \
    int32_t p4 = args[3].of.i32; \
    int32_t p5 = args[4].of.i32; \
    int32_t res = NAME(p1, p2, p3, p4, p5);

#define DEFINE_WASM_API_ARG_I32_8 \
    int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t

#define DEFINE_WASM_API_ARG_CHECK_I32_8(NAME) \
    int32_t p1 = args[0].of.i32; \
    int32_t p2 = args[1].of.i32; \
    int32_t p3 = args[2].of.i32; \
    int32_t p4 = args[3].of.i32; \
    int32_t p5 = args[4].of.i32; \
    int32_t p6 = args[5].of.i32; \
    int32_t p7 = args[6].of.i32; \
    int32_t p8 = args[7].of.i32; \
    int32_t res = NAME(p1, p2, p3, p4, p5, p6, p7, p8);

#define DEFINE_WASM_API_ARG_I32_9 \
    int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, \
    int32_t, int32_t

#define DEFINE_WASM_API_ARG_CHECK_I32_9(NAME) \
    int32_t p1 = args[0].of.i32; \
    int32_t p2 = args[1].of.i32; \
    int32_t p3 = args[2].of.i32; \
    int32_t p4 = args[3].of.i32; \
    int32_t p5 = args[4].of.i32; \
    int32_t p6 = args[5].of.i32; \
    int32_t p7 = args[6].of.i32; \
    int32_t p8 = args[7].of.i32; \
    int32_t p9 = args[8].of.i32; \
    int32_t res = NAME(p1, p2, p3, p4, p5, p6, p7, p8, p9);

#define DEFINE_WASM_API_ARG_I32_10 \
    int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, \
    int32_t, int32_t, int32_t

#define DEFINE_WASM_API_ARG_CHECK_I32_10(NAME) \
    int32_t p1 = args[0].of.i32; \
    int32_t p2 = args[1].of.i32; \
    int32_t p3 = args[2].of.i32; \
    int32_t p4 = args[3].of.i32; \
    int32_t p5 = args[4].of.i32; \
    int32_t p6 = args[5].of.i32; \
    int32_t p7 = args[6].of.i32; \
    int32_t p8 = args[7].of.i32; \
    int32_t p9 = args[8].of.i32; \
    int32_t p10 = args[9].of.i32; \
    int32_t res = NAME(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);

#define DEFINE_WASM_API_ARG_I32_12 \
    int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, \
    int32_t, int32_t, int32_t, int32_t, int32_t

#define DEFINE_WASM_API_ARG_CHECK_I32_12(NAME) \
    int32_t p1 = args[0].of.i32; \
    int32_t p2 = args[1].of.i32; \
    int32_t p3 = args[2].of.i32; \
    int32_t p4 = args[3].of.i32; \
    int32_t p5 = args[4].of.i32; \
    int32_t p6 = args[5].of.i32; \
    int32_t p7 = args[6].of.i32; \
    int32_t p8 = args[7].of.i32; \
    int32_t p9 = args[8].of.i32; \
    int32_t p10 = args[9].of.i32; \
    int32_t p11 = args[10].of.i32; \
    int32_t p12 = args[11].of.i32; \
    int32_t res = NAME(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12);


#define DEFINE_WASM_NAME(NAME, ARG) \
    {ngx_string(#NAME), wasmtime_##NAME, ARG},
#define DEFINE_WASM_NAME_ALIAS(NAME, ARG, ALIAS) \
    {ngx_string(#NAME), wasmtime_##NAME, ARG}, \
    {ngx_string(#ALIAS), wasmtime_##NAME, ARG},
#define DEFINE_WASM_NAME_ARG_VOID \
    0, {}
#define DEFINE_WASM_NAME_ARG_I32 \
    1, {WASM_I32}
#define DEFINE_WASM_NAME_ARG_I32_I32 \
    2, {WASM_I32, WASM_I32}
#define DEFINE_WASM_NAME_ARG_I32_I32_I32 \
    3, {WASM_I32, WASM_I32, WASM_I32}
#define DEFINE_WASM_NAME_ARG_I32_I32_I32_I32 \
    4, {WASM_I32, WASM_I32, WASM_I32, WASM_I32}
#define DEFINE_WASM_NAME_ARG_I32_I32_I32_I32_I32 \
    5, {WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32}
#define DEFINE_WASM_NAME_ARG_I32_8 \
    8, {WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32, \
        WASM_I32, WASM_I32, WASM_I32, \
    }
#define DEFINE_WASM_NAME_ARG_I32_9 \
    9, {WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32, \
        WASM_I32, WASM_I32, WASM_I32, WASM_I32, \
    }
#define DEFINE_WASM_NAME_ARG_I32_10 \
    10, {WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32, \
         WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32, \
    }
#define DEFINE_WASM_NAME_ARG_I32_12 \
    12, {WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32, \
         WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32, \
         WASM_I32, WASM_I32, \
    }


typedef struct {
    ngx_str_t                name;
    wasmtime_func_callback_t cb;
    int8_t                   param_num;
    wasm_valkind_t           param_type[MAX_WASM_API_ARG];
} ngx_wasm_host_api_t;


DEFINE_WASM_API(proxy_set_effective_context,
                DEFINE_WASM_API_ARG_I32,
                DEFINE_WASM_API_ARG_CHECK_I32(proxy_set_effective_context))
DEFINE_WASM_API(proxy_log,
                DEFINE_WASM_API_ARG_I32_I32_I32,
                DEFINE_WASM_API_ARG_CHECK_I32_I32_I32(proxy_log))
DEFINE_WASM_API(proxy_get_property,
                DEFINE_WASM_API_ARG_I32_I32_I32_I32,
                DEFINE_WASM_API_ARG_CHECK_I32_I32_I32_I32(proxy_get_property))
DEFINE_WASM_API(proxy_get_buffer_bytes,
                DEFINE_WASM_API_ARG_I32_I32_I32_I32_I32,
                DEFINE_WASM_API_ARG_CHECK_I32_I32_I32_I32_I32(proxy_get_buffer_bytes))
DEFINE_WASM_API(proxy_set_buffer_bytes,
                DEFINE_WASM_API_ARG_I32_I32_I32_I32_I32,
                DEFINE_WASM_API_ARG_CHECK_I32_I32_I32_I32_I32(proxy_set_buffer_bytes))
DEFINE_WASM_API(proxy_send_http_response,
                DEFINE_WASM_API_ARG_I32_8,
                DEFINE_WASM_API_ARG_CHECK_I32_8(
                    proxy_send_http_response))
DEFINE_WASM_API(proxy_get_current_time_nanoseconds,
                DEFINE_WASM_API_ARG_I32,
                DEFINE_WASM_API_ARG_CHECK_I32(proxy_get_current_time_nanoseconds))
DEFINE_WASM_API(proxy_set_tick_period_milliseconds,
                DEFINE_WASM_API_ARG_I32,
                DEFINE_WASM_API_ARG_CHECK_I32(proxy_set_tick_period_milliseconds))
DEFINE_WASM_API(proxy_get_configuration,
                DEFINE_WASM_API_ARG_I32_I32,
                DEFINE_WASM_API_ARG_CHECK_I32_I32(proxy_get_configuration))
DEFINE_WASM_API(proxy_get_header_map_pairs,
                DEFINE_WASM_API_ARG_I32_I32_I32,
                DEFINE_WASM_API_ARG_CHECK_I32_I32_I32(proxy_get_header_map_pairs))
DEFINE_WASM_API(proxy_set_header_map_pairs,
                DEFINE_WASM_API_ARG_I32_I32_I32,
                DEFINE_WASM_API_ARG_CHECK_I32_I32_I32(proxy_set_header_map_pairs))
DEFINE_WASM_API(proxy_get_header_map_value,
                DEFINE_WASM_API_ARG_I32_I32_I32_I32_I32,
                DEFINE_WASM_API_ARG_CHECK_I32_I32_I32_I32_I32(proxy_get_header_map_value))
DEFINE_WASM_API(proxy_remove_header_map_value,
                DEFINE_WASM_API_ARG_I32_I32_I32,
                DEFINE_WASM_API_ARG_CHECK_I32_I32_I32(proxy_remove_header_map_value))
DEFINE_WASM_API(proxy_replace_header_map_value,
                DEFINE_WASM_API_ARG_I32_I32_I32_I32_I32,
                DEFINE_WASM_API_ARG_CHECK_I32_I32_I32_I32_I32(proxy_replace_header_map_value))
DEFINE_WASM_API(proxy_add_header_map_value,
                DEFINE_WASM_API_ARG_I32_I32_I32_I32_I32,
                DEFINE_WASM_API_ARG_CHECK_I32_I32_I32_I32_I32(proxy_add_header_map_value))
DEFINE_WASM_API(proxy_set_property,
                DEFINE_WASM_API_ARG_I32_I32_I32_I32,
                DEFINE_WASM_API_ARG_CHECK_I32_I32_I32_I32(proxy_set_property))
DEFINE_WASM_API(proxy_get_shared_data,
                DEFINE_WASM_API_ARG_I32_I32_I32_I32_I32,
                DEFINE_WASM_API_ARG_CHECK_I32_I32_I32_I32_I32(proxy_get_shared_data))
DEFINE_WASM_API(proxy_set_shared_data,
                DEFINE_WASM_API_ARG_I32_I32_I32_I32_I32,
                DEFINE_WASM_API_ARG_CHECK_I32_I32_I32_I32_I32(proxy_set_shared_data))
DEFINE_WASM_API(proxy_register_shared_queue,
                DEFINE_WASM_API_ARG_I32_I32_I32,
                DEFINE_WASM_API_ARG_CHECK_I32_I32_I32(proxy_register_shared_queue))
DEFINE_WASM_API(proxy_resolve_shared_queue,
                DEFINE_WASM_API_ARG_I32_I32_I32_I32_I32,
                DEFINE_WASM_API_ARG_CHECK_I32_I32_I32_I32_I32(proxy_resolve_shared_queue))
DEFINE_WASM_API(proxy_dequeue_shared_queue,
                DEFINE_WASM_API_ARG_I32_I32_I32,
                DEFINE_WASM_API_ARG_CHECK_I32_I32_I32(proxy_dequeue_shared_queue))
DEFINE_WASM_API(proxy_enqueue_shared_queue,
                DEFINE_WASM_API_ARG_I32_I32_I32,
                DEFINE_WASM_API_ARG_CHECK_I32_I32_I32(proxy_enqueue_shared_queue))
DEFINE_WASM_API(proxy_continue_request,
                DEFINE_WASM_API_ARG_VOID,
                DEFINE_WASM_API_ARG_CHECK_VOID(proxy_continue_request))
DEFINE_WASM_API(proxy_continue_response,
                DEFINE_WASM_API_ARG_VOID,
                DEFINE_WASM_API_ARG_CHECK_VOID(proxy_continue_response))
DEFINE_WASM_API(proxy_clear_route_cache,
                DEFINE_WASM_API_ARG_VOID,
                DEFINE_WASM_API_ARG_CHECK_VOID(proxy_clear_route_cache))
DEFINE_WASM_API(proxy_http_call,
                DEFINE_WASM_API_ARG_I32_10,
                DEFINE_WASM_API_ARG_CHECK_I32_10(proxy_http_call))
DEFINE_WASM_API(proxy_grpc_call,
                DEFINE_WASM_API_ARG_I32_12,
                DEFINE_WASM_API_ARG_CHECK_I32_12(proxy_grpc_call))
DEFINE_WASM_API(proxy_grpc_stream,
                DEFINE_WASM_API_ARG_I32_9,
                DEFINE_WASM_API_ARG_CHECK_I32_9(proxy_grpc_stream))
DEFINE_WASM_API(proxy_grpc_send,
                DEFINE_WASM_API_ARG_I32_I32_I32_I32,
                DEFINE_WASM_API_ARG_CHECK_I32_I32_I32_I32(proxy_grpc_send))
DEFINE_WASM_API(proxy_grpc_cancel,
                DEFINE_WASM_API_ARG_I32,
                DEFINE_WASM_API_ARG_CHECK_I32(proxy_grpc_cancel))
DEFINE_WASM_API(proxy_grpc_close,
                DEFINE_WASM_API_ARG_I32,
                DEFINE_WASM_API_ARG_CHECK_I32(proxy_grpc_close))
DEFINE_WASM_API(proxy_get_status,
                DEFINE_WASM_API_ARG_I32_I32_I32,
                DEFINE_WASM_API_ARG_CHECK_I32_I32_I32(proxy_get_status))
DEFINE_WASM_API(proxy_done,
                DEFINE_WASM_API_ARG_VOID,
                DEFINE_WASM_API_ARG_CHECK_VOID(proxy_done))


/* Here we put the definition of host_apis in the header so we don't need to modify two files when
 * a new host API is added. A side effect is that the compiler will think this variable 'unused'.
 * */
static ngx_wasm_host_api_t __attribute__((unused)) host_apis[] = {
    DEFINE_WASM_NAME(proxy_set_effective_context, DEFINE_WASM_NAME_ARG_I32)
    DEFINE_WASM_NAME(proxy_log, DEFINE_WASM_NAME_ARG_I32_I32_I32)
    DEFINE_WASM_NAME(proxy_get_property, DEFINE_WASM_NAME_ARG_I32_I32_I32_I32)
    DEFINE_WASM_NAME(proxy_get_buffer_bytes, DEFINE_WASM_NAME_ARG_I32_I32_I32_I32_I32)
    DEFINE_WASM_NAME(proxy_set_buffer_bytes, DEFINE_WASM_NAME_ARG_I32_I32_I32_I32_I32)
    DEFINE_WASM_NAME_ALIAS(proxy_send_http_response,
                           DEFINE_WASM_NAME_ARG_I32_8,
                           proxy_send_local_response)
    DEFINE_WASM_NAME(proxy_get_current_time_nanoseconds, DEFINE_WASM_NAME_ARG_I32)
    DEFINE_WASM_NAME(proxy_set_tick_period_milliseconds, DEFINE_WASM_NAME_ARG_I32)
    DEFINE_WASM_NAME(proxy_get_configuration, DEFINE_WASM_NAME_ARG_I32_I32)
    DEFINE_WASM_NAME(proxy_get_header_map_pairs, DEFINE_WASM_NAME_ARG_I32_I32_I32)
    DEFINE_WASM_NAME(proxy_set_header_map_pairs, DEFINE_WASM_NAME_ARG_I32_I32_I32)
    DEFINE_WASM_NAME(proxy_get_header_map_value,
                     DEFINE_WASM_NAME_ARG_I32_I32_I32_I32_I32)
    DEFINE_WASM_NAME(proxy_remove_header_map_value,
                     DEFINE_WASM_NAME_ARG_I32_I32_I32)
    DEFINE_WASM_NAME(proxy_replace_header_map_value,
                     DEFINE_WASM_NAME_ARG_I32_I32_I32_I32_I32)
    DEFINE_WASM_NAME(proxy_add_header_map_value,
                     DEFINE_WASM_NAME_ARG_I32_I32_I32_I32_I32)
    DEFINE_WASM_NAME(proxy_set_property, DEFINE_WASM_NAME_ARG_I32_I32_I32_I32)
    DEFINE_WASM_NAME(proxy_get_shared_data, DEFINE_WASM_NAME_ARG_I32_I32_I32_I32_I32)
    DEFINE_WASM_NAME(proxy_set_shared_data, DEFINE_WASM_NAME_ARG_I32_I32_I32_I32_I32)
    DEFINE_WASM_NAME(proxy_register_shared_queue, DEFINE_WASM_NAME_ARG_I32_I32_I32)
    DEFINE_WASM_NAME(proxy_resolve_shared_queue, DEFINE_WASM_NAME_ARG_I32_I32_I32_I32_I32)
    DEFINE_WASM_NAME(proxy_dequeue_shared_queue, DEFINE_WASM_NAME_ARG_I32_I32_I32)
    DEFINE_WASM_NAME(proxy_enqueue_shared_queue, DEFINE_WASM_NAME_ARG_I32_I32_I32)
    DEFINE_WASM_NAME(proxy_continue_request, DEFINE_WASM_NAME_ARG_VOID)
    DEFINE_WASM_NAME(proxy_continue_response, DEFINE_WASM_NAME_ARG_VOID)
    DEFINE_WASM_NAME(proxy_clear_route_cache, DEFINE_WASM_NAME_ARG_VOID)
    DEFINE_WASM_NAME(proxy_http_call, DEFINE_WASM_NAME_ARG_I32_10)
    DEFINE_WASM_NAME(proxy_grpc_call, DEFINE_WASM_NAME_ARG_I32_12)
    DEFINE_WASM_NAME(proxy_grpc_stream, DEFINE_WASM_NAME_ARG_I32_9)
    DEFINE_WASM_NAME(proxy_grpc_send, DEFINE_WASM_NAME_ARG_I32_I32_I32_I32)
    DEFINE_WASM_NAME(proxy_grpc_cancel, DEFINE_WASM_NAME_ARG_I32)
    DEFINE_WASM_NAME(proxy_grpc_close, DEFINE_WASM_NAME_ARG_I32)
    DEFINE_WASM_NAME(proxy_get_status, DEFINE_WASM_NAME_ARG_I32_I32_I32)
    DEFINE_WASM_NAME(proxy_done, DEFINE_WASM_NAME_ARG_VOID)
    { ngx_null_string, NULL, 0, {} }
};


wasm_functype_t *ngx_http_wasm_host_api_func(const ngx_wasm_host_api_t *api);
ngx_int_t ngx_http_wasm_resolve_symbol(void);


#endif // NGX_HTTP_WASM_API_H
