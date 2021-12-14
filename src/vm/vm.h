#ifndef VM_H
#define VM_H


#include <stdbool.h>
#include <ngx_core.h>


#define NGX_WASM_PARAM_VOID                 1
#define NGX_WASM_PARAM_I32                  2
#define NGX_WASM_PARAM_I32_I32              3
#define NGX_WASM_PARAM_I32_I32_I32          4
#define NGX_WASM_PARAM_I32_I32_I32_I32      5
#define NGX_WASM_PARAM_I32_I32_I32_I32_I32  6


typedef struct {
    ngx_str_t            *name;

    ngx_int_t            (*init)(void);
    void                 (*cleanup)(void);

    void            *(*load)(const char *bytecode, size_t size);
    void             (*unload)(void *plugin);

    /*
     * get_memory returns a pointer to the given address in WASM.
     * It returns NULL if addr + size is out of bound.
     */
    u_char          *(*get_memory)(ngx_log_t *log, int32_t addr, int32_t size);
    /*
     * malloc allocates memory in WASM, and then return the address of the allocated
     * memory.
     */
    int32_t          (*malloc)(ngx_log_t *log, int32_t size);

    /*
     * call run a function exported from the plugin.
     */
    ngx_int_t        (*call)(void *plugin, ngx_str_t *name, bool has_result,
                             int param_type, ...);
    /*
     * has check if a function is exported from the plugin.
     */
    bool             (*has)(void *plugin, ngx_str_t *name);
} ngx_wasm_vm_t;


extern ngx_wasm_vm_t ngx_wasm_vm;


ngx_int_t ngx_wasm_vm_init();
void ngx_wasm_vm_cleanup(void *data);


#endif // VM_H
