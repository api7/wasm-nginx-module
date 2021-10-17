#ifndef NGX_HTTP_WASM_STATE_H
#define NGX_HTTP_WASM_STATE_H


#include <ngx_core.h>
#include <ngx_http.h>


typedef struct {
    ngx_str_t           conf;
    ngx_http_request_t *r;
} ngx_http_wasm_state_t;


void ngx_http_wasm_set_state(ngx_http_wasm_state_t *state);
const ngx_str_t *ngx_http_wasm_get_conf(void);
ngx_http_request_t *ngx_http_wasm_get_req(void);
ngx_log_t *ngx_http_wasm_get_log(void);


#endif // NGX_HTTP_WASM_STATE_H
