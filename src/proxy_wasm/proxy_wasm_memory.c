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
#include "vm/vm.h"
#include "proxy_wasm_memory.h"


static ngx_str_t proxy_on_memory_allocate = ngx_string("proxy_on_memory_allocate");
static ngx_str_t exported_malloc = ngx_string("malloc");


int32_t proxy_wasm_memory_alloc(ngx_log_t *log, int32_t size)
{
    int32_t         addr;

    addr = ngx_wasm_vm->call(NULL, &proxy_on_memory_allocate, true, NGX_WASM_PARAM_I32, size);
    if (addr == 0) {
        addr = ngx_wasm_vm->call(NULL, &exported_malloc, true, NGX_WASM_PARAM_I32, size);
    }

    if (addr == 0) {
        ngx_log_error(NGX_LOG_ERR, log, 0, "failed to malloc");
    }

    return addr;
}
