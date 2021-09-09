#include <wasi.h>
#include <wasm.h>
#include <wasmtime.h>
#include <http/ngx_http_wasm_api.h>
#include "vm.h"


typedef struct {
    wasmtime_module_t       *module;
    wasmtime_store_t        *store;
    wasmtime_context_t      *context;
} ngx_wasm_wasmtime_plugin_t;


static ngx_str_t      vm_name = ngx_string("wasmtime");
static wasm_engine_t *vm_engine;
static wasmtime_linker_t *vm_linker;
static wasi_config_t    *vm_wasi_config;


static void
ngx_wasm_wasmtime_report_error(ngx_log_t *log, const char *message,
    wasmtime_error_t *error, wasm_trap_t *trap)
{
    wasm_byte_vec_t error_message;

    if (error != NULL) {
        wasmtime_error_message(error, &error_message);
        wasmtime_error_delete(error);
    } else {
        wasm_trap_message(trap, &error_message);
        wasm_trap_delete(trap);
    }

    ngx_log_error(NGX_LOG_ERR, log, 0, "%s%*s",
                  message, error_message.size, error_message.data);
    wasm_byte_vec_delete(&error_message);
}


static ngx_int_t
ngx_wasm_wasmtime_init(void)
{
    int               i;
    wasmtime_error_t *error;

    ngx_log_error(NGX_LOG_NOTICE, ngx_cycle->log, 0, "init wasm vm: wasmtime");

    vm_engine = wasm_engine_new();
    if (vm_engine == NULL) {
        return NGX_DECLINED;
    }

    vm_linker = wasmtime_linker_new(vm_engine);
    if (vm_linker == NULL) {
        goto free_engine;
    }

    error = wasmtime_linker_define_wasi(vm_linker);
    if (error != NULL) {
        ngx_wasm_wasmtime_report_error(ngx_cycle->log, "failed to define wasi",
                                       error, NULL);
        goto free_linker;
    }

    vm_wasi_config = wasi_config_new();
    if (vm_wasi_config == NULL) {
        goto free_linker;
    }

    wasi_config_inherit_argv(vm_wasi_config);
    wasi_config_inherit_env(vm_wasi_config);
    wasi_config_inherit_stdin(vm_wasi_config);
    wasi_config_inherit_stdout(vm_wasi_config);
    wasi_config_inherit_stderr(vm_wasi_config);

    for (i = 0; host_apis[i].name.len; i++) {
        ngx_wasm_host_api_t *api = &host_apis[i];

        ngx_log_debug1(NGX_LOG_DEBUG_CORE, ngx_cycle->log, 0,
                       "define wasm host API %V", &api->name);

        error = wasmtime_linker_define_func(vm_linker, "env", 3,
                                            (const char *) api->name.data, api->name.len,
                                            ngx_http_wasm_host_api_func(api),
                                            api->cb, NULL, NULL);
        if (error != NULL) {
            ngx_wasm_wasmtime_report_error(ngx_cycle->log, "failed to define API", error, NULL);
            goto free_linker;
        }
    }

    return NGX_OK;

free_linker:
    wasmtime_linker_delete(vm_linker);

free_engine:
    wasm_engine_delete(vm_engine);
    vm_engine = NULL;

    return NGX_DECLINED;
}


static void
ngx_wasm_wasmtime_cleanup(void)
{
    if (vm_engine == NULL) {
        return;
    }

    ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, 0, "cleanup wasm vm: wasmtime");
    wasmtime_linker_delete(vm_linker);
    wasm_engine_delete(vm_engine);
}


static void *
ngx_wasm_wasmtime_load(const char *bytecode, size_t size)
{
    wasm_trap_t                  *trap = NULL;
    wasmtime_module_t            *module;
    wasmtime_store_t             *store;
    wasmtime_context_t           *context;
    wasmtime_error_t             *error;
    ngx_wasm_wasmtime_plugin_t   *plugin;

    // TODO: separate WASM compiling from the store init
    error = wasmtime_module_new(vm_engine, (const uint8_t*) bytecode, size, &module);
    if (module == NULL) {
        return NULL;
    }

    store = wasmtime_store_new(vm_engine, NULL, NULL);
    if (store == NULL) {
        goto free_module;
    }

    context = wasmtime_store_context(store);
    error = wasmtime_context_set_wasi(context, vm_wasi_config);
    if (error != NULL) {
        ngx_wasm_wasmtime_report_error(ngx_cycle->log, "failed to init WASI: ", error, NULL);
        goto free_store;
    }

    plugin = ngx_alloc(sizeof(ngx_wasm_wasmtime_plugin_t), ngx_cycle->log);
    if (plugin == NULL) {
        goto free_store;
    }

    error = wasmtime_linker_module(vm_linker, context, "", 0, module);
    if (error != NULL || trap != NULL) {
        ngx_wasm_wasmtime_report_error(ngx_cycle->log, "failed to link module: ", error, trap);
        goto free_store;
    }

    plugin->module = module;
    plugin->store = store;
    plugin->context = context;

    ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, 0, "loaded wasm plugin");

    return plugin;

free_store:
    wasmtime_store_delete(store);

free_module:
    wasmtime_module_delete(module);

    return NULL;
}


static void
ngx_wasm_wasmtime_unload(void *data)
{
    ngx_wasm_wasmtime_plugin_t *plugin = data;

    wasmtime_module_delete(plugin->module);
    wasmtime_store_delete(plugin->store);

    ngx_free(plugin);

    ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, 0, "unloaded wasm plugin");
}


ngx_wasm_vm_t ngx_wasm_vm = {
    &vm_name,
    ngx_wasm_wasmtime_init,
    ngx_wasm_wasmtime_cleanup,
    ngx_wasm_wasmtime_load,
    ngx_wasm_wasmtime_unload,
};
