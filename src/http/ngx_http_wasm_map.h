#ifndef NGX_HTTP_WASM_MAP_H
#define NGX_HTTP_WASM_MAP_H
#include <stdbool.h>
#include <ngx_core.h>


typedef struct {
    int32_t  idx;
    int32_t  len;
    int32_t *size_ptr;
    char    *data_ptr;
} proxy_wasm_map_iter;


void ngx_http_wasm_map_init_iter(proxy_wasm_map_iter *it, const u_char *map_data);
bool ngx_http_wasm_map_next(proxy_wasm_map_iter *it, char **key, int32_t *key_len,
                            char **val, int32_t *val_len);


#endif // NGX_HTTP_WASM_MAP_H
