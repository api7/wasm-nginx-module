#ifndef NGX_HTTP_WASM_CTX_H
#define NGX_HTTP_WASM_CTX_H


#include <ngx_core.h>
#include "ngx_http_wasm_state.h"
#include "proxy_wasm/proxy_wasm_types.h"


#define PROXY_WASM_ABI_VER_010      0
#define PROXY_WASM_ABI_VER_020      1
#define PROXY_WASM_ABI_VER_021      2
#define PROXY_WASM_ABI_VER_MAX      99


typedef struct {
    void                  *plugin;
    uint32_t               cur_ctx_id;
    uint32_t               abi_version;
    ngx_str_t              name;
    ngx_http_wasm_state_t *state;
    ngx_queue_t            occupied;
    ngx_queue_t            free;
    unsigned               done:1;
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
    ngx_array_t      *http_ctxs;
    void             *callout;
    uint32_t          callout_id;
    /* for http callback */
    proxy_wasm_table_elt_t  *call_resp_headers;
    ngx_uint_t               call_resp_n_header;
    ngx_str_t               *call_resp_body;
} ngx_http_wasm_ctx_t;


ngx_http_wasm_ctx_t *ngx_http_wasm_get_module_ctx(ngx_http_request_t *r);


#endif // NGX_HTTP_WASM_CTX_H
