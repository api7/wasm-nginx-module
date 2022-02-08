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
#include "proxy_wasm_map.h"


/**
 * the format of proxy_map_t is:
 * number of items (4 bytes) +
 * len of key1 + len of val1 + len of key2 + ... (4 bytes for each len)
 * data of key1 + \0 + data of val1 + data of key2 + ...
 */


void
proxy_wasm_map_init_map(const u_char *map_data, int32_t len)
{
    *(int32_t *) map_data = len;
}


void
proxy_wasm_map_init_iter(proxy_wasm_map_iter *it, const u_char *map_data)
{
    it->idx = 0;
    it->len = *(int32_t *) map_data;
    it->size_ptr = (int32_t *) (map_data) + 1;
    it->data_ptr = (char *) (it->size_ptr + 2 * it->len);
}


bool
proxy_wasm_map_next(proxy_wasm_map_iter *it, char **key, int32_t *key_len,
                       char **val, int32_t *val_len)
{
    if (it->idx == it->len) {
        return false;
    }

    *key_len = *it->size_ptr;
    it->size_ptr++;
    *val_len = *it->size_ptr;
    it->size_ptr++;

    *key = it->data_ptr;
    it->data_ptr += *key_len + 1;
    *val = it->data_ptr;
    it->data_ptr += *val_len + 1;

    it->idx++;
    return true;
}


bool
proxy_wasm_map_reserve(proxy_wasm_map_iter *it, char **key, int32_t key_len,
                          char **val, int32_t val_len)
{
    if (it->idx == it->len) {
        return false;
    }

    *it->size_ptr = key_len;
    it->size_ptr++;
    *it->size_ptr = val_len;
    it->size_ptr++;

    *key = it->data_ptr;
    it->data_ptr += key_len;
    *it->data_ptr = '\0';
    it->data_ptr++;
    *val = it->data_ptr;
    it->data_ptr += val_len;
    *it->data_ptr = '\0';
    it->data_ptr++;

    it->idx++;
    return true;
}


bool
proxy_wasm_map_reserve_literal_with_len(proxy_wasm_map_iter *it,
                                           const char *key, size_t key_len,
                                           const char *val, size_t val_len)
{
    if (it->idx == it->len) {
        return false;
    }

    *it->size_ptr = key_len;
    it->size_ptr++;
    *it->size_ptr = val_len;
    it->size_ptr++;

    /* copy trailing '\0' */
    it->data_ptr = (char *) ngx_cpymem(it->data_ptr, key, key_len + 1);
    it->data_ptr = (char *) ngx_cpymem(it->data_ptr, val, val_len + 1);

    it->idx++;
    return true;
}
