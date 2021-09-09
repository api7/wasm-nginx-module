#ifndef VM_H
#define VM_H


#include <ngx_core.h>


typedef struct {
    ngx_str_t            *name;

    ngx_int_t            (*init)(void);
    void                 (*cleanup)(void);

    void            *(*load)(const char *bytecode, size_t size);
    void             (*unload)(void *plugin);
} ngx_wasm_vm_t;


extern ngx_wasm_vm_t ngx_wasm_vm;


ngx_int_t ngx_wasm_vm_init();
void ngx_wasm_vm_cleanup(void *data);

#endif // VM_H
