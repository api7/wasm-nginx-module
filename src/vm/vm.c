#include "vm.h"


ngx_int_t
ngx_wasm_vm_init()
{
    return ngx_wasm_vm.init();
}


void
ngx_wasm_vm_cleanup(void *data)
{
    ngx_wasm_vm_t *vm = data;

    if (vm == NULL) {
        return;
    }

    vm->cleanup();
}
