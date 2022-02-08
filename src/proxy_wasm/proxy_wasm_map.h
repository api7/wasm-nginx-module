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
#ifndef PROXY_WASM_MAP_H
#define PROXY_WASM_MAP_H
#include <stdbool.h>
#include <ngx_core.h>


typedef enum {
    PROXY_MAP_TYPE_HTTP_REQUEST_HEADERS = 0,
    PROXY_MAP_TYPE_HTTP_REQUEST_TRAILERS = 1,
    PROXY_MAP_TYPE_HTTP_RESPONSE_HEADERS = 2,
    PROXY_MAP_TYPE_HTTP_RESPONSE_TRAILERS = 3,
    PROXY_MAP_TYPE_HTTP_CALL_RESPONSE_HEADERS = 6,
    PROXY_MAP_TYPE_HTTP_CALL_RESPONSE_TRAILERS = 7,
} proxy_map_type_t;


typedef struct {
    int32_t  idx;
    int32_t  len;
    int32_t *size_ptr;
    char    *data_ptr;
} proxy_wasm_map_iter;


void proxy_wasm_map_init_map(const u_char *map_data, int32_t len);
void proxy_wasm_map_init_iter(proxy_wasm_map_iter *it, const u_char *map_data);
bool proxy_wasm_map_next(proxy_wasm_map_iter *it, char **key, int32_t *key_len,
                         char **val, int32_t *val_len);
bool proxy_wasm_map_reserve(proxy_wasm_map_iter *it, char **key, int32_t key_len,
                            char **val, int32_t val_len);
bool proxy_wasm_map_reserve_literal_with_len(proxy_wasm_map_iter *it,
                                             const char *key, size_t key_len,
                                             const char *val, size_t val_len);
#define proxy_wasm_map_reserve_literal(it, k, v)	\
    proxy_wasm_map_reserve_literal_with_len(it, k, sizeof((k)) - 1, v, sizeof((v)) - 1)


#endif // PROXY_WASM_MAP_H
