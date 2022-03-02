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
#include "vm.h"


ngx_wasm_vm_t *ngx_wasm_vm = NULL;


ngx_int_t
ngx_wasm_vm_init(ngx_str_t *name)
{
    if (ngx_strcmp(name->data, "wasmtime") == 0) {
        ngx_wasm_vm = &ngx_wasm_wasmtime_vm;
    }

    if (ngx_wasm_vm == NULL) {
        ngx_log_error(NGX_LOG_EMERG, ngx_cycle->log, 0, "unsupported wasm vm %V", name);
        return NGX_ERROR;
    }

    return ngx_wasm_vm->init();
}


void
ngx_wasm_vm_cleanup(void *data)
{
    ngx_wasm_vm_t *vm = data;

    if (vm == NULL) {
        return;
    }

    vm->cleanup();
}
