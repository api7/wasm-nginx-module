#ifndef NGX_HTTP_WASM_CALL_H
#define NGX_HTTP_WASM_CALL_H


#include <ngx_http.h>


/* this is NGX_HTTP_LUA_CONTEXT_$PHASE in the lua-nginx-module */
#define NGX_HTTP_WASM_PHASE_REWRITE   0x0002
#define NGX_HTTP_WASM_PHASE_ACCESS    0x0004
#define NGX_HTTP_WASM_PHASE_CONTENT   0x0008
#define NGX_HTTP_WASM_PHASE_TIMER     0x0080


#define NGX_HTTP_WASM_YIELDABLE (NGX_HTTP_WASM_PHASE_REWRITE    \
                                 | NGX_HTTP_WASM_PHASE_ACCESS   \
                                 | NGX_HTTP_WASM_PHASE_CONTENT  \
                                 | NGX_HTTP_WASM_PHASE_TIMER    \
                                 )


ngx_int_t ngx_http_wasm_call_register(ngx_http_request_t *r, ngx_str_t *up,
                                      u_char *map_data, ngx_str_t *body,
                                      int32_t timeout, uint32_t *callout_id);


#endif // NGX_HTTP_WASM_CALL_H
