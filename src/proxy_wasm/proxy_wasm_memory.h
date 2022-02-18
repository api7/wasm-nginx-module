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
#ifndef PROXY_WASM_MEMORY_H
#define PROXY_WASM_MEMORY_H
#include <ngx_core.h>


/*
 * malloc allocates memory in WASM, and then return the address of the allocated
 * memory.
 */
int32_t proxy_wasm_memory_alloc(ngx_log_t *log, int32_t size);


#endif // PROXY_WASM_MEMORY_H
