#ifndef NGX_HTTP_WASM_STATE_H
#define NGX_HTTP_WASM_STATE_H


#include <ngx_core.h>


typedef struct {
    ngx_str_t    conf;
} ngx_http_wasm_state_t;


void ngx_http_wasm_set_state(ngx_http_wasm_state_t *state);
const ngx_str_t *ngx_http_wasm_get_conf(void);


#endif // NGX_HTTP_WASM_STATE_H
