#include <ngx_http.h>
#include "ngx_http_wasm_call.h"
#include "ngx_http_wasm_ctx.h"
#include "ngx_http_wasm_map.h"


typedef struct {
    int32_t        timeout_ms;
    ngx_str_t     *up;
    u_char        *map_data;
    ngx_str_t     *body;
} proxy_wasm_callout_t;


static uint32_t cur_callout_id = 0;


ngx_int_t
ngx_http_wasm_call_register(ngx_http_request_t *r, ngx_str_t *up, u_char *map_data,
                            ngx_str_t *body, int32_t timeout, uint32_t *callout_id)
{
    ngx_http_wasm_ctx_t          *ctx;
    proxy_wasm_callout_t         *callout;

    ctx = ngx_http_wasm_get_module_ctx(r);
    /* as we are running with http ctx, we can ensure the ctx is not NULL */

    if (ctx->callout != NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "multiple calls are not supported");
        return NGX_ERROR;
    }

    callout = ngx_palloc(r->pool, sizeof(proxy_wasm_callout_t));
    if (callout == NULL) {
        return NGX_DECLINED;
    }

    callout->timeout_ms = timeout;
    callout->up = up;
    callout->map_data = map_data;
    callout->body = body;

    ctx->callout = callout;
    ctx->callout_id = cur_callout_id;

    ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                  "register http call callout id: %d, host: %V",
                  cur_callout_id, up);

    *callout_id = cur_callout_id;
    cur_callout_id++;

    return NGX_OK;
}


ngx_int_t
ngx_http_wasm_call_max_headers_count(ngx_http_request_t *r)
{
    ngx_http_wasm_ctx_t          *ctx;
    proxy_wasm_callout_t         *callout;
    proxy_wasm_map_iter           it;

    ctx = ngx_http_wasm_get_module_ctx(r);
    callout = ctx->callout;
    ngx_http_wasm_map_init_iter(&it, callout->map_data);

    return it.len;
}


void
ngx_http_wasm_call_get(ngx_http_request_t *r, ngx_str_t *method, ngx_str_t *scheme,
                       ngx_str_t *host, ngx_str_t *path,
                       ngx_str_t *headers, ngx_str_t *body, int32_t *timeout)
{
    ngx_http_wasm_ctx_t          *ctx;
    proxy_wasm_callout_t         *callout;

    ctx = ngx_http_wasm_get_module_ctx(r);
    callout = ctx->callout;
    *host = *callout->up;
    *timeout = callout->timeout_ms;
}
