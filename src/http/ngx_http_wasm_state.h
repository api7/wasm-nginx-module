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
#ifndef NGX_HTTP_WASM_STATE_H
#define NGX_HTTP_WASM_STATE_H


#include <ngx_core.h>
#include <ngx_http.h>


typedef struct {
    ngx_str_t                 conf;
    ngx_http_request_t       *r;
    ngx_str_t                *plugin_name;
} ngx_http_wasm_state_t;


void ngx_http_wasm_set_state(ngx_http_wasm_state_t *state);
const ngx_str_t *ngx_http_wasm_get_conf(void);
ngx_http_request_t *ngx_http_wasm_get_req(void);
ngx_log_t *ngx_http_wasm_get_log(void);
ngx_str_t *ngx_http_wasm_get_plugin_name(void);


#endif // NGX_HTTP_WASM_STATE_H
