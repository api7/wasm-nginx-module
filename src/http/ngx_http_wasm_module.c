#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "vm/vm.h"


static ngx_int_t ngx_http_wasm_init(ngx_conf_t *cf);
static ngx_str_t plugin_start = ngx_string("_start");
static ngx_str_t proxy_on_context_create = ngx_string("proxy_on_context_create");
static ngx_str_t proxy_on_configure = ngx_string("proxy_on_configure");


static ngx_http_module_t ngx_http_wasm_module_ctx = {
    NULL,                                    /* preconfiguration */
    ngx_http_wasm_init,                      /* postconfiguration */

    NULL,                                    /* create main configuration */
    NULL,                                    /* init main configuration */

    NULL,                                    /* create server configuration */
    NULL,                                    /* merge server configuration */

    NULL,                                    /* create location configuration */
    NULL                                     /* merge location configuration */
};


ngx_module_t ngx_http_wasm_module = {
    NGX_MODULE_V1,
    &ngx_http_wasm_module_ctx,           /* module context */
    NULL,                                /* module directives */
    NGX_HTTP_MODULE,                     /* module type */
    NULL,                                /* init master */
    NULL,                                /* init module */
    NULL,                                /* init process */
    NULL,                                /* init thread */
    NULL,                                /* exit thread */
    NULL,                                /* exit process */
    NULL,                                /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_int_t
ngx_http_wasm_init(ngx_conf_t *cf)
{
    ngx_int_t                   rc;
    ngx_pool_cleanup_t         *cln;

    if (ngx_process == NGX_PROCESS_SIGNALLER || ngx_test_config) {
        return NGX_OK;
    }

    cln = ngx_pool_cleanup_add(cf->pool, 0);
    if (cln == NULL) {
        return NGX_ERROR;
    }

    cln->handler = ngx_wasm_vm_cleanup;

    rc = ngx_wasm_vm_init();
    if (rc != NGX_OK) {
        return rc;
    }

    cln->data = &ngx_wasm_vm;

    return NGX_OK;
}


void *
ngx_http_load_plugin(const char *bytecode, size_t size)
{
    return ngx_wasm_vm.load(bytecode, size);
}


void
ngx_http_unload_plugin(void *plugin)
{
    ngx_wasm_vm.unload(plugin);
}


ngx_int_t
ngx_http_on_configure(void *plugin, const char *conf, size_t size)
{
    ngx_int_t           rc;

    rc = ngx_wasm_vm.call(plugin, &plugin_start, false,
                          NGX_WASM_PARAM_VOID);
    if (rc != NGX_OK) {
        return rc;
    }
    uint32_t plugin_ctx_id = 1;
    rc = ngx_wasm_vm.call(plugin, &proxy_on_context_create, false,
                          NGX_WASM_PARAM_I32_I32, plugin_ctx_id, 0);
    if (rc != NGX_OK) {
        return rc;
    }

    rc = ngx_wasm_vm.call(plugin, &proxy_on_configure, true,
                          NGX_WASM_PARAM_I32_I32, plugin_ctx_id, size);
    return rc;
}
