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
#include "ngx_http_wasm_state.h"


static ngx_http_wasm_state_t    *cur_state = NULL;


void
ngx_http_wasm_set_state(ngx_http_wasm_state_t *state)
{
    if (state == NULL) {
        /* clear state data */
        cur_state->body.data = NULL;
        cur_state->body.len = 0;
    }

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


ngx_str_t *
ngx_http_wasm_get_plugin_name(void)
{
    if (cur_state == NULL) {
        return NULL;
    }

    return cur_state->plugin_name;
}


const ngx_str_t *
ngx_http_wasm_get_body(void)
{
    if (cur_state == NULL) {
        return NULL;
    }

    return &cur_state->body;
}
