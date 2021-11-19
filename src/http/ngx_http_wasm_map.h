#ifndef NGX_HTTP_WASM_MAP_H
#define NGX_HTTP_WASM_MAP_H
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


void ngx_http_wasm_map_init_map(const u_char *map_data, int32_t len);
void ngx_http_wasm_map_init_iter(proxy_wasm_map_iter *it, const u_char *map_data);
bool ngx_http_wasm_map_next(proxy_wasm_map_iter *it, char **key, int32_t *key_len,
                            char **val, int32_t *val_len);
bool ngx_http_wasm_map_reserve(proxy_wasm_map_iter *it, char **key, int32_t key_len,
                               char **val, int32_t val_len);
bool ngx_http_wasm_map_reserve_literal_with_len(proxy_wasm_map_iter *it,
                                                const char *key, size_t key_len,
                                                const char *val, size_t val_len);
#define ngx_http_wasm_map_reserve_literal(it, k, v)	\
    ngx_http_wasm_map_reserve_literal_with_len(it, k, sizeof((k)) - 1, v, sizeof((v)) - 1)


#endif // NGX_HTTP_WASM_MAP_H
