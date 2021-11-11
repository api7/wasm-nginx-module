#include "ngx_http_wasm_map.h"


/**
 * the format of proxy_map_t is:
 * number of items (4 bytes) +
 * len of key1 + len of val1 + len of key2 + ... (4 bytes for each len)
 * data of key1 + \0 + data of val1 + data of key2 + ...
 */


void
ngx_http_wasm_map_init_iter(proxy_wasm_map_iter *it, const u_char *map_data)
{
    it->idx = 0;
    it->len = *(int32_t *) map_data;
    it->size_ptr = (int32_t *) (map_data) + 1;
    it->data_ptr = (char *) (it->size_ptr + 2 * it->len);
}


bool
ngx_http_wasm_map_next(proxy_wasm_map_iter *it, char **key, int32_t *key_len,
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
