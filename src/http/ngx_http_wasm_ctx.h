#ifndef NGX_HTTP_WASM_CTX_H
#define NGX_HTTP_WASM_CTX_H


#include <ngx_core.h>
#include "ngx_http_wasm_state.h"


typedef struct {
    void        *plugin;
    uint32_t     cur_ctx_id;
    ngx_queue_t  occupied;
    ngx_queue_t  free;
    unsigned     done:1;
} ngx_http_wasm_plugin_t;


typedef struct {
    ngx_queue_t                 queue;
    uint32_t                    id;
    ngx_http_wasm_state_t      *state;
    ngx_http_wasm_plugin_t     *hw_plugin;
    ngx_pool_t                 *pool;
    ngx_queue_t                 occupied;
    ngx_queue_t                 free;
    unsigned                    done:1;
} ngx_http_wasm_plugin_ctx_t;


typedef struct {
    ngx_queue_t                  queue;
    uint32_t                     id;
    ngx_http_wasm_plugin_ctx_t  *hwp_ctx;
} ngx_http_wasm_http_ctx_t;


typedef struct {
  ngx_http_wasm_http_ctx_t      *http_ctx;
} ngx_http_wasm_ctx_t;


#endif // NGX_HTTP_WASM_CTX_H
