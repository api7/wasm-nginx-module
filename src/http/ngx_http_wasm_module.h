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
