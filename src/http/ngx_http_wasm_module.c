#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "ngx_http_wasm_state.h"
#include "vm/vm.h"


static ngx_int_t ngx_http_wasm_init(ngx_conf_t *cf);


static bool     ngx_http_wasm_vm_inited = false;


static ngx_str_t plugin_start = ngx_string("_start");
static ngx_str_t proxy_on_context_create = ngx_string("proxy_on_context_create");
static ngx_str_t proxy_on_configure = ngx_string("proxy_on_configure");
static ngx_str_t proxy_on_done = ngx_string("proxy_on_done");
static ngx_str_t proxy_on_delete = ngx_string("proxy_on_delete");


typedef struct {
    void        *plugin;
    uint32_t     cur_ctx_id;
    ngx_queue_t  occupied;
    ngx_queue_t  free;
    unsigned     done:1;
} ngx_http_wasm_plugin_t;


typedef struct {
    uint32_t                    id;
    ngx_http_wasm_state_t      *state;
    ngx_http_wasm_plugin_t     *hw_plugin;
    ngx_pool_t                 *pool;
    ngx_queue_t                 queue;
} ngx_http_wasm_plugin_ctx_t;


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

    if (ngx_process == NGX_PROCESS_SIGNALLER || ngx_test_config || ngx_http_wasm_vm_inited) {
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

    /* don't reinit during reconfiguring */
    ngx_http_wasm_vm_inited = true;

    return NGX_OK;
}


void *
ngx_http_wasm_load_plugin(const char *bytecode, size_t size)
{
    void                    *plugin;
    ngx_int_t                rc;
    ngx_http_wasm_plugin_t  *hw_plugin;

    plugin = ngx_wasm_vm.load(bytecode, size);

    if (plugin == NULL) {
        return NULL;
    }

    rc = ngx_wasm_vm.call(plugin, &plugin_start, false,
                          NGX_WASM_PARAM_VOID);
    if (rc != NGX_OK) {
        goto free_plugin;
    }

    hw_plugin = ngx_alloc(sizeof(ngx_http_wasm_plugin_t), ngx_cycle->log);
    if (hw_plugin == NULL) {
        goto free_plugin;
    }

    hw_plugin->cur_ctx_id = 0;
    hw_plugin->plugin = plugin;
    ngx_queue_init(&hw_plugin->occupied);
    ngx_queue_init(&hw_plugin->free);
    return hw_plugin;

free_plugin:
    ngx_wasm_vm.unload(plugin);
    return NULL;
}


static void
ngx_http_wasm_free_plugin(ngx_http_wasm_plugin_t *hw_plugin)
{
    void                    *plugin = hw_plugin->plugin;

    if (!ngx_queue_empty(&hw_plugin->occupied)) {
        /* some plugin ctxs are using it. Do not free */
        return;
    }

    ngx_wasm_vm.unload(plugin);

    while (!ngx_queue_empty(&hw_plugin->free)) {
        ngx_queue_t                 *q;
        ngx_http_wasm_plugin_ctx_t  *hwp_ctx;

        q = ngx_queue_last(&hw_plugin->free);
        ngx_queue_remove(q);
        hwp_ctx = ngx_queue_data(q, ngx_http_wasm_plugin_ctx_t, queue);
        ngx_free(hwp_ctx);
    }

    ngx_free(hw_plugin);
}


void
ngx_http_wasm_unload_plugin(ngx_http_wasm_plugin_t *hw_plugin)
{
    hw_plugin->done = 1;
    ngx_http_wasm_free_plugin(hw_plugin);
}


void
ngx_http_wasm_delete_plugin_ctx(ngx_http_wasm_plugin_ctx_t *hwp_ctx)
{
    ngx_int_t                        rc;
    uint32_t                         ctx_id = hwp_ctx->id;
    ngx_http_wasm_plugin_t          *hw_plugin = hwp_ctx->hw_plugin;
    void                            *plugin = hw_plugin->plugin;
    ngx_log_t                       *log;

    log = ngx_cycle->log;

    ngx_queue_remove(&hwp_ctx->queue);
    ngx_queue_insert_head(&hw_plugin->free, &hwp_ctx->queue);

    rc = ngx_wasm_vm.call(plugin, &proxy_on_done, true, NGX_WASM_PARAM_I32, ctx_id);
    if (rc <= 0) {
        ngx_log_error(NGX_LOG_ERR, log, 0, "failed to mark context %d as done, rc: %d",
                      ctx_id, rc);
    }

    rc = ngx_wasm_vm.call(plugin, &proxy_on_delete, false, NGX_WASM_PARAM_I32, ctx_id);
    if (rc != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, log, 0, "failed to delete context %d, rc: %d",
                      ctx_id, rc);
    }

    if (hwp_ctx->pool != NULL) {
        ngx_destroy_pool(hwp_ctx->pool);
        hwp_ctx->pool = NULL;
    }

    if (hw_plugin->done) {
        ngx_http_wasm_free_plugin(hw_plugin);
    }
}


void *
ngx_http_wasm_on_configure(ngx_http_wasm_plugin_t *hw_plugin, const char *conf, size_t size)
{
    ngx_int_t                        rc;
    void                            *plugin = hw_plugin->plugin;
    uint32_t                         ctx_id;
    u_char                          *state_conf;
    ngx_log_t                       *log;
    ngx_http_wasm_plugin_ctx_t      *hwp_ctx;

    log = ngx_cycle->log;

    if (!ngx_queue_empty(&hw_plugin->free)) {
        ngx_queue_t         *q;

        q = ngx_queue_last(&hw_plugin->free);
        ngx_queue_remove(q);
        hwp_ctx = ngx_queue_data(q, ngx_http_wasm_plugin_ctx_t, queue);
        ctx_id = hwp_ctx->id;

    } else {
        hwp_ctx = ngx_alloc(sizeof(ngx_http_wasm_plugin_ctx_t), log);
        if (hwp_ctx == NULL) {
            return NULL;
        }

        hw_plugin->cur_ctx_id++;
        ctx_id = hw_plugin->cur_ctx_id;
        hwp_ctx->id = ctx_id;
        hwp_ctx->hw_plugin = hw_plugin;
    }

    rc = ngx_wasm_vm.call(plugin, &proxy_on_context_create, false,
                          NGX_WASM_PARAM_I32_I32, ctx_id, 0);
    if (rc != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, log, 0, "failed to create context %d, rc: %d",
                      ctx_id, rc);
        /* reuse the ctx_id */
        ngx_queue_insert_head(&hw_plugin->free, &hwp_ctx->queue);
        return NULL;
    }

    ngx_queue_insert_head(&hw_plugin->occupied, &hwp_ctx->queue);

    hwp_ctx->pool = ngx_create_pool(512, log);
    if (hwp_ctx->pool == NULL) {
        goto free_hwp_ctx;
    }

    hwp_ctx->state = ngx_palloc(hwp_ctx->pool, sizeof(ngx_http_wasm_state_t) + size);
    if (hwp_ctx->state == NULL) {
        goto free_hwp_ctx;
    }

    state_conf = (u_char *) (hwp_ctx->state + 1);
    /* copy conf so we can access it anytime */
    ngx_memcpy(state_conf, conf, size);

    hwp_ctx->state->conf.data = state_conf;
    hwp_ctx->state->conf.len = size;

    ngx_http_wasm_set_state(hwp_ctx->state);

    rc = ngx_wasm_vm.call(plugin, &proxy_on_configure, true,
                          NGX_WASM_PARAM_I32_I32, ctx_id, size);
    if (rc <= 0) {
        ngx_log_error(NGX_LOG_ERR, log, 0, "failed to configure plugin context %d, rc: %d",
                      ctx_id, rc);
        goto free_hwp_ctx;
    }

    ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, 0, "create plugin context %d", ctx_id);

    return hwp_ctx;

free_hwp_ctx:
    ngx_http_wasm_delete_plugin_ctx(hwp_ctx);
    return NULL;
}
