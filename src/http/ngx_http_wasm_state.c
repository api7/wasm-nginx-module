#include "ngx_http_wasm_state.h"


static ngx_http_wasm_state_t    *cur_state = NULL;


void
ngx_http_wasm_set_state(ngx_http_wasm_state_t *state)
{
    cur_state = state;
}


const ngx_str_t *
ngx_http_wasm_get_conf(void)
{
    if (cur_state == NULL) {
        return NULL;
    }

    return &cur_state->conf;
}


ngx_http_request_t *
ngx_http_wasm_get_req(void)
{
    if (cur_state == NULL) {
        return NULL;
    }

    return cur_state->r;
}


ngx_log_t *
ngx_http_wasm_get_log(void)
{
    if (cur_state != NULL && cur_state->r != NULL) {
        return cur_state->r->connection->log;
    }

    return ngx_cycle->log;
}
