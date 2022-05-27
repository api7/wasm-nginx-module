/*
 * Copyright 2022 Shenzhen ZhiLiu Technology Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <wasi.h>
#include <wasm.h>
#include <wasmedge/wasmedge.h>
#include <http/ngx_http_wasm_api_wasmedge.h>
#include "vm.h"


typedef struct {
    WasmEdge_ModuleInstanceContext *module;
    WasmEdge_StoreContext          *store;
    WasmEdge_ExecutorContext       *exec;
    WasmEdge_MemoryInstanceContext *memory;
    /* need to ensure the import objects have the same lifecycle as plugin */
    WasmEdge_ModuleInstanceContext *import;
    WasmEdge_ModuleInstanceContext *import_wasi;
} ngx_wasm_wasmedge_plugin_t;


static ngx_str_t      vm_name = ngx_string("wasmedge");
static ngx_wasm_wasmedge_plugin_t *cur_plugin;


static WasmEdge_FunctionTypeContext *
ngx_http_wasmedge_host_api_func(const ngx_wasm_wasmedge_host_api_t *api)
{
    enum WasmEdge_ValType                result[1] = {WasmEdge_ValType_I32};

    return WasmEdge_FunctionTypeCreate(api->param_type, api->param_num, result, 1);
}


static ngx_int_t
ngx_wasm_wasmedge_init(void)
{
    ngx_log_error(NGX_LOG_NOTICE, ngx_cycle->log, 0, "init wasm vm: wasmedge");

    return NGX_OK;
}


static void
ngx_wasm_wasmedge_cleanup(void)
{
    ngx_log_error(NGX_LOG_NOTICE, ngx_cycle->log, 0, "cleanup wasm vm: wasmedge");
}


static void *
ngx_wasm_wasmedge_load(const char *bytecode, size_t size)
{
    ngx_wasm_wasmedge_plugin_t      *plugin;
    size_t                           i;
    WasmEdge_Result                  res;
    WasmEdge_ASTModuleContext       *ast_ctx = NULL;
    WasmEdge_StoreContext           *store_ctx;
    WasmEdge_LoaderContext          *loader;
    WasmEdge_ValidatorContext       *validator;
    WasmEdge_ModuleInstanceContext  *import;
    WasmEdge_ModuleInstanceContext  *import_wasi = NULL;
    WasmEdge_ModuleInstanceContext  *module = NULL;
    WasmEdge_ExecutorContext        *executor;
    WasmEdge_MemoryInstanceContext  *memory;
    WasmEdge_String                  s;

    /* Create the configure context */
    store_ctx = WasmEdge_StoreCreate();
    if (store_ctx == NULL) {
        goto end;
    }

    loader = WasmEdge_LoaderCreate(NULL);
    if (loader == NULL) {
        goto free_store;
    }

    validator = WasmEdge_ValidatorCreate(NULL);
    if (validator == NULL) {
        goto free_loader;
    }

    executor = WasmEdge_ExecutorCreate(NULL, NULL);
    if (executor == NULL) {
        goto free_validator;
    }

    res = WasmEdge_LoaderParseFromBuffer(loader, &ast_ctx, (const uint8_t*) bytecode, size);
    if (!WasmEdge_ResultOK(res)) {
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "Loading phase failed: %s",
                      WasmEdge_ResultGetMessage(res));
        goto free_executor;
    }

    res = WasmEdge_ValidatorValidate(validator, ast_ctx);
    if (!WasmEdge_ResultOK(res)) {
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "Validation phase failed: %s",
                      WasmEdge_ResultGetMessage(res));
        goto free_ast;
    }

    s = WasmEdge_StringCreateByCString("env");
    import = WasmEdge_ModuleInstanceCreate(s);
    WasmEdge_StringDelete(s);

    if (import == NULL) {
        goto free_ast;
    }

    for (i = 0; host_apis[i].name.len; i++) {
        ngx_wasm_wasmedge_host_api_t        *api = &host_apis[i];
        WasmEdge_FunctionTypeContext        *ft;
        WasmEdge_FunctionInstanceContext    *f;

        ngx_log_debug1(NGX_LOG_DEBUG_CORE, ngx_cycle->log, 0,
                       "define wasm host API %V", &api->name);

        ft = ngx_http_wasmedge_host_api_func(api);
        if (ft == NULL) {
            goto free_import;
        }

        f = WasmEdge_FunctionInstanceCreate(ft, api->cb, NULL, 0);
        if (f == NULL) {
            goto free_import;
        }

        s = WasmEdge_StringCreateByBuffer((const char *)api->name.data, api->name.len);
        /* The caller should __NOT__ access or delete the function instance context
         * after calling this function.
         */
        WasmEdge_ModuleInstanceAddFunction(import, s, f);
        WasmEdge_StringDelete(s);
        WasmEdge_FunctionTypeDelete(ft);
    }

    res = WasmEdge_ExecutorRegisterImport(executor, store_ctx, import);
    if (!WasmEdge_ResultOK(res)) {
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "Register imports failed: %s",
                      WasmEdge_ResultGetMessage(res));
        goto free_import;
    }

    // TODO: add env
    import_wasi = WasmEdge_ModuleInstanceCreateWASI(NULL, 0, NULL, 0, NULL, 0);
    res = WasmEdge_ExecutorRegisterImport(executor, store_ctx, import_wasi);
    if (!WasmEdge_ResultOK(res)) {
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "Register wasi imports failed: %s",
                      WasmEdge_ResultGetMessage(res));
        goto free_import;
    }

    res = WasmEdge_ExecutorInstantiate(executor, &module, store_ctx, ast_ctx);
    if (!WasmEdge_ResultOK(res)) {
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "Instantiation phase failed: %s",
                      WasmEdge_ResultGetMessage(res));
        goto free_import;
    }

    s = WasmEdge_StringCreateByCString("memory");
    memory = WasmEdge_ModuleInstanceFindMemory(module, s);
    WasmEdge_StringDelete(s);
    if (memory == NULL) {
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "the wasm plugin doesn't export memory");
        goto free_module;
    }

    plugin = ngx_alloc(sizeof(ngx_wasm_wasmedge_plugin_t), ngx_cycle->log);
    if (plugin == NULL) {
        goto free_module;
    }

    plugin->exec = executor;
    plugin->module = module;
    plugin->store = store_ctx;
    plugin->memory = memory;
    plugin->import = import;
    plugin->import_wasi = import_wasi;

    WasmEdge_ASTModuleDelete(ast_ctx);
    WasmEdge_ValidatorDelete(validator);
    WasmEdge_LoaderDelete(loader);

    ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, 0, "loaded wasm plugin");

    return plugin;

free_module:
    WasmEdge_ModuleInstanceDelete(module);

free_import:
    WasmEdge_ModuleInstanceDelete(import);

    if (import_wasi != NULL) {
        WasmEdge_ModuleInstanceDelete(import_wasi);
    }

free_ast:
    WasmEdge_ASTModuleDelete(ast_ctx);

free_executor:
    WasmEdge_ExecutorDelete(executor);

free_validator:
    WasmEdge_ValidatorDelete(validator);

free_loader:
    WasmEdge_LoaderDelete(loader);

free_store:
    WasmEdge_StoreDelete(store_ctx);

end:
    return NULL;
}


static void
ngx_wasm_wasmedge_unload(void *data)
{
    ngx_wasm_wasmedge_plugin_t *plugin = data;

    WasmEdge_ModuleInstanceDelete(plugin->module);
    WasmEdge_ExecutorDelete(plugin->exec);
    WasmEdge_StoreDelete(plugin->store);
    WasmEdge_ModuleInstanceDelete(plugin->import);
    WasmEdge_ModuleInstanceDelete(plugin->import_wasi);
    ngx_free(plugin);

    ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, 0, "unloaded wasm plugin");
}


static bool
ngx_wasm_wasmedge_has(void *data, ngx_str_t *name)
{
    ngx_wasm_wasmedge_plugin_t          *plugin = data;
    WasmEdge_FunctionInstanceContext    *finst;
    WasmEdge_String                      s;

    s = WasmEdge_StringCreateByBuffer((const char *) name->data, name->len);
    finst = WasmEdge_ModuleInstanceFindFunction(plugin->module, s);
    WasmEdge_StringDelete(s);

    return finst != NULL;
}


static ngx_int_t
ngx_wasm_wasmedge_call(void *data, ngx_str_t *name, bool has_result, int param_type, ...)
{
    ngx_wasm_wasmedge_plugin_t          *plugin = data;
    WasmEdge_FunctionInstanceContext    *finst = NULL;
    WasmEdge_Result                      res;
    WasmEdge_String                      s;
    ngx_int_t                            rc;
    size_t                               i;
    va_list                              args;
    size_t                               param_num = 0;
    WasmEdge_Value                       param_list[MAX_WASM_API_ARG];
    WasmEdge_Value                       results[1];

    ngx_log_debug1(NGX_LOG_DEBUG_CORE, ngx_cycle->log, 0, "wasmedge call function %V", name);

    if (plugin == NULL) {
        plugin = cur_plugin;
    } else {
        cur_plugin = plugin;
    }

    if (!ngx_wasm_wasmedge_has(plugin, name)) {
        ngx_log_debug1(NGX_LOG_DEBUG_CORE, ngx_cycle->log, 0,
                       "wasmedge function %V not defined", name);
        return NGX_OK;
    }

    va_start(args, param_type);

    switch (param_type) {
    case NGX_WASM_PARAM_VOID:
        break;

    case NGX_WASM_PARAM_I32:
        param_num = 1;
        break;

    case NGX_WASM_PARAM_I32_I32:
        param_num = 2;
        break;

    case NGX_WASM_PARAM_I32_I32_I32:
        param_num = 3;
        break;

    case NGX_WASM_PARAM_I32_I32_I32_I32:
        param_num = 4;
        break;

    case NGX_WASM_PARAM_I32_I32_I32_I32_I32:
        param_num = 5;
        break;

    default:
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "unknown param type: %d", param_type);
        va_end(args);
        return NGX_ERROR;
    }

    for (i = 0; i < param_num; i++) {
        param_list[i] = WasmEdge_ValueGenI32(va_arg(args, int32_t));
    }

    va_end(args);

    s = WasmEdge_StringCreateByBuffer((const char *) name->data, name->len);
    finst = WasmEdge_ModuleInstanceFindFunction(plugin->module, s);
    res = WasmEdge_ExecutorInvoke(plugin->exec, finst,
                                  param_list, param_num, results, has_result ? 1 : 0);
    WasmEdge_StringDelete(s);

    if (!WasmEdge_ResultOK(res)) {
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "failed to call function: %s",
                      WasmEdge_ResultGetMessage(res));
        return NGX_ERROR;
    }

    if (!has_result) {
        ngx_log_debug0(NGX_LOG_DEBUG_CORE, ngx_cycle->log, 0,
                       "wasmedge call function done");
        return NGX_OK;
    }

    rc = WasmEdge_ValueGetI32(results[0]);

    ngx_log_debug1(NGX_LOG_DEBUG_CORE, ngx_cycle->log, 0,
                   "wasmedge call function result: %d", rc);

    return rc;
}


u_char *
ngx_wasm_wasmedge_get_memory(ngx_log_t *log, int32_t addr, int32_t size)
{
    u_char *data;

    /* If the `addr + size` is larger, than the data size in the memory instance,
     * this function will return NULL. */
    data = WasmEdge_MemoryInstanceGetPointer(cur_plugin->memory, addr, size);
    if (data == NULL) {
        ngx_log_error(NGX_LOG_ERR, log, 0,
                      "wasmedge failed to access memory addr %d with size %d", addr, size);
    }

    return data;
}


ngx_wasm_vm_t ngx_wasm_wasmedge_vm = {
    &vm_name,
    ngx_wasm_wasmedge_init,
    ngx_wasm_wasmedge_cleanup,
    ngx_wasm_wasmedge_load,
    ngx_wasm_wasmedge_unload,
    ngx_wasm_wasmedge_get_memory,
    ngx_wasm_wasmedge_call,
    ngx_wasm_wasmedge_has,
};
