#include <wasm.h>
#include <wasmtime.h>
#include "vm.h"


static ngx_str_t      vm_name = ngx_string("wasmtime");
static wasm_engine_t *vm_engine;


static ngx_int_t
ngx_wasm_wasmtime_init(void)
{
    vm_engine = wasm_engine_new();
    if (vm_engine == NULL) {
        return NGX_DECLINED;
    }

    ngx_log_error(NGX_LOG_NOTICE, ngx_cycle->log, 0, "init wasm vm: wasmtime");
    return NGX_OK;
}


static void
ngx_wasm_wasmtime_cleanup(void)
{
    if (vm_engine == NULL) {
        return;
    }
    wasm_engine_delete(vm_engine);
}


ngx_wasm_vm_t ngx_wasm_vm = {
    &vm_name,
    ngx_wasm_wasmtime_init,
    ngx_wasm_wasmtime_cleanup,
};
