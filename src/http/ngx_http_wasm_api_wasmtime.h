
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
/* Code generated by `./gen_wasm_host_api.py src/http`. DO NOT EDIT. */
#ifndef NGX_HTTP_WASM_API_WASMTIME_H
#define NGX_HTTP_WASM_API_WASMTIME_H


#include <wasm.h>
#include <wasmtime.h>
#include <ngx_core.h>
#include "proxy_wasm/proxy_wasm_types.h"
#include "http/ngx_http_wasm_api_def.h"


#define MAX_WASM_API_ARG  12


#define DEFINE_WASM_API(NAME, ARG_CHECK) \
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
#define DEFINE_WASM_NAME(NAME, ARG) \
    {ngx_string(#NAME), wasmtime_##NAME, ARG},


typedef struct {
    ngx_str_t                name;
    wasmtime_func_callback_t cb;
    int8_t                   param_num;
    wasm_valkind_t           param_type[MAX_WASM_API_ARG];
} ngx_wasm_wasmtime_host_api_t;


#define DEFINE_WASM_NAME_ARG_VOID \
    0, {}
#define DEFINE_WASM_API_ARG_CHECK_VOID(NAME) \
    int32_t res = NAME();

#define DEFINE_WASM_NAME_ARG_I32_1 \
    1, { \
    WASM_I32, }
#define DEFINE_WASM_API_ARG_CHECK_I32_1(NAME) \
    int32_t p0 = args[0].of.i32; \
    int32_t res = NAME(p0);

#define DEFINE_WASM_NAME_ARG_I32_2 \
    2, { \
    WASM_I32, WASM_I32, }
#define DEFINE_WASM_API_ARG_CHECK_I32_2(NAME) \
    int32_t p0 = args[0].of.i32; \
    int32_t p1 = args[1].of.i32; \
    int32_t res = NAME(p0, p1);

#define DEFINE_WASM_NAME_ARG_I32_3 \
    3, { \
    WASM_I32, WASM_I32, WASM_I32, }
#define DEFINE_WASM_API_ARG_CHECK_I32_3(NAME) \
    int32_t p0 = args[0].of.i32; \
    int32_t p1 = args[1].of.i32; \
    int32_t p2 = args[2].of.i32; \
    int32_t res = NAME(p0, p1, p2);

#define DEFINE_WASM_NAME_ARG_I32_4 \
    4, { \
    WASM_I32, WASM_I32, WASM_I32, WASM_I32, }
#define DEFINE_WASM_API_ARG_CHECK_I32_4(NAME) \
    int32_t p0 = args[0].of.i32; \
    int32_t p1 = args[1].of.i32; \
    int32_t p2 = args[2].of.i32; \
    int32_t p3 = args[3].of.i32; \
    int32_t res = NAME(p0, p1, p2, p3);

#define DEFINE_WASM_NAME_ARG_I32_5 \
    5, { \
    WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32,  \
}
#define DEFINE_WASM_API_ARG_CHECK_I32_5(NAME) \
    int32_t p0 = args[0].of.i32; \
    int32_t p1 = args[1].of.i32; \
    int32_t p2 = args[2].of.i32; \
    int32_t p3 = args[3].of.i32; \
    int32_t p4 = args[4].of.i32; \
    int32_t res = NAME(p0, p1, p2, p3, p4);

#define DEFINE_WASM_NAME_ARG_I32_6 \
    6, { \
    WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32,  \
    WASM_I32, }
#define DEFINE_WASM_API_ARG_CHECK_I32_6(NAME) \
    int32_t p0 = args[0].of.i32; \
    int32_t p1 = args[1].of.i32; \
    int32_t p2 = args[2].of.i32; \
    int32_t p3 = args[3].of.i32; \
    int32_t p4 = args[4].of.i32; \
    int32_t p5 = args[5].of.i32; \
    int32_t res = NAME(p0, p1, p2, p3, p4, p5);

#define DEFINE_WASM_NAME_ARG_I32_7 \
    7, { \
    WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32,  \
    WASM_I32, WASM_I32, }
#define DEFINE_WASM_API_ARG_CHECK_I32_7(NAME) \
    int32_t p0 = args[0].of.i32; \
    int32_t p1 = args[1].of.i32; \
    int32_t p2 = args[2].of.i32; \
    int32_t p3 = args[3].of.i32; \
    int32_t p4 = args[4].of.i32; \
    int32_t p5 = args[5].of.i32; \
    int32_t p6 = args[6].of.i32; \
    int32_t res = NAME(p0, p1, p2, p3, p4, p5, p6);

#define DEFINE_WASM_NAME_ARG_I32_8 \
    8, { \
    WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32,  \
    WASM_I32, WASM_I32, WASM_I32, }
#define DEFINE_WASM_API_ARG_CHECK_I32_8(NAME) \
    int32_t p0 = args[0].of.i32; \
    int32_t p1 = args[1].of.i32; \
    int32_t p2 = args[2].of.i32; \
    int32_t p3 = args[3].of.i32; \
    int32_t p4 = args[4].of.i32; \
    int32_t p5 = args[5].of.i32; \
    int32_t p6 = args[6].of.i32; \
    int32_t p7 = args[7].of.i32; \
    int32_t res = NAME(p0, p1, p2, p3, p4, p5, p6, p7);

#define DEFINE_WASM_NAME_ARG_I32_9 \
    9, { \
    WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32,  \
    WASM_I32, WASM_I32, WASM_I32, WASM_I32, }
#define DEFINE_WASM_API_ARG_CHECK_I32_9(NAME) \
    int32_t p0 = args[0].of.i32; \
    int32_t p1 = args[1].of.i32; \
    int32_t p2 = args[2].of.i32; \
    int32_t p3 = args[3].of.i32; \
    int32_t p4 = args[4].of.i32; \
    int32_t p5 = args[5].of.i32; \
    int32_t p6 = args[6].of.i32; \
    int32_t p7 = args[7].of.i32; \
    int32_t p8 = args[8].of.i32; \
    int32_t res = NAME(p0, p1, p2, p3, p4, p5, p6, p7, p8);

#define DEFINE_WASM_NAME_ARG_I32_10 \
    10, { \
    WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32,  \
    WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32,  \
}
#define DEFINE_WASM_API_ARG_CHECK_I32_10(NAME) \
    int32_t p0 = args[0].of.i32; \
    int32_t p1 = args[1].of.i32; \
    int32_t p2 = args[2].of.i32; \
    int32_t p3 = args[3].of.i32; \
    int32_t p4 = args[4].of.i32; \
    int32_t p5 = args[5].of.i32; \
    int32_t p6 = args[6].of.i32; \
    int32_t p7 = args[7].of.i32; \
    int32_t p8 = args[8].of.i32; \
    int32_t p9 = args[9].of.i32; \
    int32_t res = NAME(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9);

#define DEFINE_WASM_NAME_ARG_I32_11 \
    11, { \
    WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32,  \
    WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32,  \
    WASM_I32, }
#define DEFINE_WASM_API_ARG_CHECK_I32_11(NAME) \
    int32_t p0 = args[0].of.i32; \
    int32_t p1 = args[1].of.i32; \
    int32_t p2 = args[2].of.i32; \
    int32_t p3 = args[3].of.i32; \
    int32_t p4 = args[4].of.i32; \
    int32_t p5 = args[5].of.i32; \
    int32_t p6 = args[6].of.i32; \
    int32_t p7 = args[7].of.i32; \
    int32_t p8 = args[8].of.i32; \
    int32_t p9 = args[9].of.i32; \
    int32_t p10 = args[10].of.i32; \
    int32_t res = NAME(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);

#define DEFINE_WASM_NAME_ARG_I32_12 \
    12, { \
    WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32,  \
    WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32,  \
    WASM_I32, WASM_I32, }
#define DEFINE_WASM_API_ARG_CHECK_I32_12(NAME) \
    int32_t p0 = args[0].of.i32; \
    int32_t p1 = args[1].of.i32; \
    int32_t p2 = args[2].of.i32; \
    int32_t p3 = args[3].of.i32; \
    int32_t p4 = args[4].of.i32; \
    int32_t p5 = args[5].of.i32; \
    int32_t p6 = args[6].of.i32; \
    int32_t p7 = args[7].of.i32; \
    int32_t p8 = args[8].of.i32; \
    int32_t p9 = args[9].of.i32; \
    int32_t p10 = args[10].of.i32; \
    int32_t p11 = args[11].of.i32; \
    int32_t res = NAME(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11);

#define DEFINE_WASM_NAME_ARG_I32_I64 \
    2, { \
    WASM_I32, WASM_I64, }
#define DEFINE_WASM_API_ARG_CHECK_I32_I64(NAME) \
    int32_t p0 = args[0].of.i32; \
    int64_t p1 = args[1].of.i64; \
    int32_t res = NAME(p0, p1);

DEFINE_WASM_API(proxy_set_effective_context,
                DEFINE_WASM_API_ARG_CHECK_I32_1(proxy_set_effective_context))
DEFINE_WASM_API(proxy_get_property,
                DEFINE_WASM_API_ARG_CHECK_I32_4(proxy_get_property))
DEFINE_WASM_API(proxy_set_property,
                DEFINE_WASM_API_ARG_CHECK_I32_4(proxy_set_property))
DEFINE_WASM_API(proxy_log,
                DEFINE_WASM_API_ARG_CHECK_I32_3(proxy_log))
DEFINE_WASM_API(proxy_get_buffer_bytes,
                DEFINE_WASM_API_ARG_CHECK_I32_5(proxy_get_buffer_bytes))
DEFINE_WASM_API(proxy_set_buffer_bytes,
                DEFINE_WASM_API_ARG_CHECK_I32_5(proxy_set_buffer_bytes))
DEFINE_WASM_API(proxy_send_local_response,
                DEFINE_WASM_API_ARG_CHECK_I32_8(proxy_send_local_response))
DEFINE_WASM_API(proxy_send_http_response,
                DEFINE_WASM_API_ARG_CHECK_I32_8(proxy_send_http_response))
DEFINE_WASM_API(proxy_get_current_time_nanoseconds,
                DEFINE_WASM_API_ARG_CHECK_I32_1(proxy_get_current_time_nanoseconds))
DEFINE_WASM_API(proxy_set_tick_period_milliseconds,
                DEFINE_WASM_API_ARG_CHECK_I32_1(proxy_set_tick_period_milliseconds))
DEFINE_WASM_API(proxy_get_configuration,
                DEFINE_WASM_API_ARG_CHECK_I32_2(proxy_get_configuration))
DEFINE_WASM_API(proxy_get_header_map_pairs,
                DEFINE_WASM_API_ARG_CHECK_I32_3(proxy_get_header_map_pairs))
DEFINE_WASM_API(proxy_set_header_map_pairs,
                DEFINE_WASM_API_ARG_CHECK_I32_3(proxy_set_header_map_pairs))
DEFINE_WASM_API(proxy_get_header_map_value,
                DEFINE_WASM_API_ARG_CHECK_I32_5(proxy_get_header_map_value))
DEFINE_WASM_API(proxy_remove_header_map_value,
                DEFINE_WASM_API_ARG_CHECK_I32_3(proxy_remove_header_map_value))
DEFINE_WASM_API(proxy_replace_header_map_value,
                DEFINE_WASM_API_ARG_CHECK_I32_5(proxy_replace_header_map_value))
DEFINE_WASM_API(proxy_add_header_map_value,
                DEFINE_WASM_API_ARG_CHECK_I32_5(proxy_add_header_map_value))
DEFINE_WASM_API(proxy_get_shared_data,
                DEFINE_WASM_API_ARG_CHECK_I32_5(proxy_get_shared_data))
DEFINE_WASM_API(proxy_set_shared_data,
                DEFINE_WASM_API_ARG_CHECK_I32_5(proxy_set_shared_data))
DEFINE_WASM_API(proxy_register_shared_queue,
                DEFINE_WASM_API_ARG_CHECK_I32_3(proxy_register_shared_queue))
DEFINE_WASM_API(proxy_resolve_shared_queue,
                DEFINE_WASM_API_ARG_CHECK_I32_5(proxy_resolve_shared_queue))
DEFINE_WASM_API(proxy_dequeue_shared_queue,
                DEFINE_WASM_API_ARG_CHECK_I32_3(proxy_dequeue_shared_queue))
DEFINE_WASM_API(proxy_enqueue_shared_queue,
                DEFINE_WASM_API_ARG_CHECK_I32_3(proxy_enqueue_shared_queue))
DEFINE_WASM_API(proxy_continue_request,
                DEFINE_WASM_API_ARG_CHECK_VOID(proxy_continue_request))
DEFINE_WASM_API(proxy_continue_response,
                DEFINE_WASM_API_ARG_CHECK_VOID(proxy_continue_response))
DEFINE_WASM_API(proxy_clear_route_cache,
                DEFINE_WASM_API_ARG_CHECK_VOID(proxy_clear_route_cache))
DEFINE_WASM_API(proxy_continue_stream,
                DEFINE_WASM_API_ARG_CHECK_I32_1(proxy_continue_stream))
DEFINE_WASM_API(proxy_close_stream,
                DEFINE_WASM_API_ARG_CHECK_I32_1(proxy_close_stream))
DEFINE_WASM_API(proxy_http_call,
                DEFINE_WASM_API_ARG_CHECK_I32_10(proxy_http_call))
DEFINE_WASM_API(proxy_grpc_call,
                DEFINE_WASM_API_ARG_CHECK_I32_12(proxy_grpc_call))
DEFINE_WASM_API(proxy_grpc_stream,
                DEFINE_WASM_API_ARG_CHECK_I32_9(proxy_grpc_stream))
DEFINE_WASM_API(proxy_grpc_send,
                DEFINE_WASM_API_ARG_CHECK_I32_4(proxy_grpc_send))
DEFINE_WASM_API(proxy_grpc_cancel,
                DEFINE_WASM_API_ARG_CHECK_I32_1(proxy_grpc_cancel))
DEFINE_WASM_API(proxy_grpc_close,
                DEFINE_WASM_API_ARG_CHECK_I32_1(proxy_grpc_close))
DEFINE_WASM_API(proxy_get_status,
                DEFINE_WASM_API_ARG_CHECK_I32_3(proxy_get_status))
DEFINE_WASM_API(proxy_done,
                DEFINE_WASM_API_ARG_CHECK_VOID(proxy_done))
DEFINE_WASM_API(proxy_call_foreign_function,
                DEFINE_WASM_API_ARG_CHECK_I32_6(proxy_call_foreign_function))
DEFINE_WASM_API(proxy_define_metric,
                DEFINE_WASM_API_ARG_CHECK_I32_4(proxy_define_metric))
DEFINE_WASM_API(proxy_increment_metric,
                DEFINE_WASM_API_ARG_CHECK_I32_I64(proxy_increment_metric))
DEFINE_WASM_API(proxy_record_metric,
                DEFINE_WASM_API_ARG_CHECK_I32_I64(proxy_record_metric))
DEFINE_WASM_API(proxy_get_metric,
                DEFINE_WASM_API_ARG_CHECK_I32_I64(proxy_get_metric))

static ngx_wasm_wasmtime_host_api_t host_apis[] = {
    DEFINE_WASM_NAME(proxy_set_effective_context, DEFINE_WASM_NAME_ARG_I32_1)
    DEFINE_WASM_NAME(proxy_get_property, DEFINE_WASM_NAME_ARG_I32_4)
    DEFINE_WASM_NAME(proxy_set_property, DEFINE_WASM_NAME_ARG_I32_4)
    DEFINE_WASM_NAME(proxy_log, DEFINE_WASM_NAME_ARG_I32_3)
    DEFINE_WASM_NAME(proxy_get_buffer_bytes, DEFINE_WASM_NAME_ARG_I32_5)
    DEFINE_WASM_NAME(proxy_set_buffer_bytes, DEFINE_WASM_NAME_ARG_I32_5)
    DEFINE_WASM_NAME(proxy_send_local_response, DEFINE_WASM_NAME_ARG_I32_8)
    DEFINE_WASM_NAME(proxy_send_http_response, DEFINE_WASM_NAME_ARG_I32_8)
    DEFINE_WASM_NAME(proxy_get_current_time_nanoseconds, DEFINE_WASM_NAME_ARG_I32_1)
    DEFINE_WASM_NAME(proxy_set_tick_period_milliseconds, DEFINE_WASM_NAME_ARG_I32_1)
    DEFINE_WASM_NAME(proxy_get_configuration, DEFINE_WASM_NAME_ARG_I32_2)
    DEFINE_WASM_NAME(proxy_get_header_map_pairs, DEFINE_WASM_NAME_ARG_I32_3)
    DEFINE_WASM_NAME(proxy_set_header_map_pairs, DEFINE_WASM_NAME_ARG_I32_3)
    DEFINE_WASM_NAME(proxy_get_header_map_value, DEFINE_WASM_NAME_ARG_I32_5)
    DEFINE_WASM_NAME(proxy_remove_header_map_value, DEFINE_WASM_NAME_ARG_I32_3)
    DEFINE_WASM_NAME(proxy_replace_header_map_value, DEFINE_WASM_NAME_ARG_I32_5)
    DEFINE_WASM_NAME(proxy_add_header_map_value, DEFINE_WASM_NAME_ARG_I32_5)
    DEFINE_WASM_NAME(proxy_get_shared_data, DEFINE_WASM_NAME_ARG_I32_5)
    DEFINE_WASM_NAME(proxy_set_shared_data, DEFINE_WASM_NAME_ARG_I32_5)
    DEFINE_WASM_NAME(proxy_register_shared_queue, DEFINE_WASM_NAME_ARG_I32_3)
    DEFINE_WASM_NAME(proxy_resolve_shared_queue, DEFINE_WASM_NAME_ARG_I32_5)
    DEFINE_WASM_NAME(proxy_dequeue_shared_queue, DEFINE_WASM_NAME_ARG_I32_3)
    DEFINE_WASM_NAME(proxy_enqueue_shared_queue, DEFINE_WASM_NAME_ARG_I32_3)
    DEFINE_WASM_NAME(proxy_continue_request, DEFINE_WASM_NAME_ARG_VOID)
    DEFINE_WASM_NAME(proxy_continue_response, DEFINE_WASM_NAME_ARG_VOID)
    DEFINE_WASM_NAME(proxy_clear_route_cache, DEFINE_WASM_NAME_ARG_VOID)
    DEFINE_WASM_NAME(proxy_continue_stream, DEFINE_WASM_NAME_ARG_I32_1)
    DEFINE_WASM_NAME(proxy_close_stream, DEFINE_WASM_NAME_ARG_I32_1)
    DEFINE_WASM_NAME(proxy_http_call, DEFINE_WASM_NAME_ARG_I32_10)
    DEFINE_WASM_NAME(proxy_grpc_call, DEFINE_WASM_NAME_ARG_I32_12)
    DEFINE_WASM_NAME(proxy_grpc_stream, DEFINE_WASM_NAME_ARG_I32_9)
    DEFINE_WASM_NAME(proxy_grpc_send, DEFINE_WASM_NAME_ARG_I32_4)
    DEFINE_WASM_NAME(proxy_grpc_cancel, DEFINE_WASM_NAME_ARG_I32_1)
    DEFINE_WASM_NAME(proxy_grpc_close, DEFINE_WASM_NAME_ARG_I32_1)
    DEFINE_WASM_NAME(proxy_get_status, DEFINE_WASM_NAME_ARG_I32_3)
    DEFINE_WASM_NAME(proxy_done, DEFINE_WASM_NAME_ARG_VOID)
    DEFINE_WASM_NAME(proxy_call_foreign_function, DEFINE_WASM_NAME_ARG_I32_6)
    DEFINE_WASM_NAME(proxy_define_metric, DEFINE_WASM_NAME_ARG_I32_4)
    DEFINE_WASM_NAME(proxy_increment_metric, DEFINE_WASM_NAME_ARG_I32_I64)
    DEFINE_WASM_NAME(proxy_record_metric, DEFINE_WASM_NAME_ARG_I32_I64)
    DEFINE_WASM_NAME(proxy_get_metric, DEFINE_WASM_NAME_ARG_I32_I64)
    { ngx_null_string, NULL, 0, {} }
};


#endif
