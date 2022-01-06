#include <ngx_http.h>
#include "ngx_http_wasm_call.h"
#include "ngx_http_wasm_ctx.h"
#include "proxy_wasm/proxy_wasm_types.h"
#include "proxy_wasm/proxy_wasm_map.h"


typedef struct {
    ngx_str_t   name;
    ngx_uint_t  ty;
} proxy_wasm_h2_header_t;


typedef struct {
    int32_t        timeout_ms;
    ngx_str_t     *up;
    u_char        *map_data;
    ngx_str_t     *body;
} proxy_wasm_callout_t;


static proxy_wasm_h2_header_t proxy_wasm_h2_headers[] = {
    {ngx_string(":path"),   PROXY_WASM_HEADER_PATH},
    {ngx_string(":method"), PROXY_WASM_HEADER_METHOD},
    {ngx_string(":scheme"), PROXY_WASM_HEADER_SCHEME},
    {ngx_string(":authority"), PROXY_WASM_HEADER_AUTHORITY},
    {ngx_null_string, 0 }
};


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

    if (timeout < 0) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "invalid timeout: %d", timeout);
        return NGX_ERROR;
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
    proxy_wasm_map_init_iter(&it, callout->map_data);

    return it.len;
}


void
ngx_http_wasm_call_get(ngx_http_request_t *r, ngx_str_t *method, ngx_str_t *scheme,
                       ngx_str_t *host, ngx_str_t *path,
                       proxy_wasm_table_elt_t *headers, ngx_str_t *body,
                       int32_t *timeout)
{
    ngx_http_wasm_ctx_t          *ctx;
    proxy_wasm_callout_t         *callout;
    proxy_wasm_map_iter           it;
    ngx_uint_t                    i;
    char                         *key, *val;
    int32_t                       key_len, val_len;

    ctx = ngx_http_wasm_get_module_ctx(r);
    callout = ctx->callout;
    ctx->callout = NULL;

    *host = *callout->up;
    *timeout = callout->timeout_ms;

    proxy_wasm_map_init_iter(&it, callout->map_data);

    while (proxy_wasm_map_next(&it, &key, &key_len, &val, &val_len)) {
        if (key_len == 0) {
            continue;
        }

        if (key[0] == ':') {
            for (i = 0; proxy_wasm_h2_headers[i].ty > 0; i++) {
                proxy_wasm_h2_header_t      *wh;

                wh = &proxy_wasm_h2_headers[i];

                if ((size_t) key_len != wh->name.len) {
                    continue;
                }

                if (ngx_strncasecmp((u_char *) key, wh->name.data, wh->name.len) == 0) {

                    switch (wh->ty) {
                    case PROXY_WASM_HEADER_PATH:
                        path->data = (u_char *) val;
                        path->len = val_len;
                        goto next;

                    case PROXY_WASM_HEADER_METHOD:
                        method->data = (u_char *) val;
                        method->len = val_len;
                        goto next;

                    case PROXY_WASM_HEADER_SCHEME:
                        scheme->data = (u_char *) val;
                        scheme->len = val_len;
                        goto next;

                    case PROXY_WASM_HEADER_AUTHORITY:
                        headers->key.data = (u_char *) "host";
                        headers->key.len = 4;
                        headers->value.data = (u_char *) val;
                        headers->value.len = val_len;
                        headers++;
                        goto next;

                    default:
                        break;
                    }
                }
            }
        }

        headers->key.data = (u_char *) key;
        headers->key.len = key_len;
        headers->value.data = (u_char *) val;
        headers->value.len = val_len;
        headers++;

    next:
        continue;
    }

    /* mark the end */
    headers->key.len = 0;

    if (callout->body) {
        *body = *callout->body;
    } else {
        body->len = 0;
    }
}
