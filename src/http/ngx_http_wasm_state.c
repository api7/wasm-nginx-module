#include "ngx_http_wasm_state.h"


static ngx_http_wasm_state_t    *cur_state;


void
ngx_http_wasm_set_state(ngx_http_wasm_state_t *state)
{
    cur_state = state;
}


const ngx_str_t *
ngx_http_wasm_get_conf(void)
{
    return &cur_state->conf;
}
