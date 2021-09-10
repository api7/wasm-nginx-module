#ifndef VM_H
#define VM_H


#include <stdbool.h>
#include <ngx_core.h>


#define NGX_WASM_PARAM_I32_I32  1
#define NGX_WASM_PARAM_VOID     2


typedef struct {
    ngx_str_t            *name;

    ngx_int_t            (*init)(void);
    void                 (*cleanup)(void);

    void            *(*load)(const char *bytecode, size_t size);
    void             (*unload)(void *plugin);

    ngx_int_t        (*call)(void *plugin, ngx_str_t *name, bool has_result,
                             int param_type, ...);
} ngx_wasm_vm_t;


extern ngx_wasm_vm_t ngx_wasm_vm;


ngx_int_t ngx_wasm_vm_init();
void ngx_wasm_vm_cleanup(void *data);


#endif // VM_H
