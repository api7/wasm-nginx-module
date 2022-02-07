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
#ifndef NGX_HTTP_WASM_MODULE_H
#define NGX_HTTP_WASM_MODULE_H


#include <ngx_core.h>


typedef struct {
    ngx_str_t       vm;
    uint32_t        code;
    ngx_str_t       body;
} ngx_http_wasm_main_conf_t;


extern ngx_module_t ngx_http_wasm_module;


#endif // NGX_HTTP_WASM_MODULE_H
