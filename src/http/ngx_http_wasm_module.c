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
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "ngx_http_wasm_api.h"
#include "ngx_http_wasm_module.h"
#include "ngx_http_wasm_state.h"
#include "ngx_http_wasm_ctx.h"
#include "vm/vm.h"


static ngx_int_t ngx_http_wasm_init(ngx_conf_t *cf);
static void *ngx_http_wasm_create_main_conf(ngx_conf_t *cf);
static char *ngx_http_wasm_init_main_conf(ngx_conf_t *cf, void *conf);
static char *ngx_http_wasm_vm(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);


static bool     ngx_http_wasm_vm_inited = false;


static ngx_str_t plugin_start = ngx_string("_start");
static ngx_str_t plugin_initialize = ngx_string("_initialize");
static ngx_str_t abi_versions[] = {
    ngx_string("proxy_abi_version_0_1_0"),
    ngx_string("proxy_abi_version_0_2_0"),
    ngx_string("proxy_abi_version_0_2_1"),
    ngx_null_string,
};
static ngx_str_t proxy_on_context_create = ngx_string("proxy_on_context_create");
static ngx_str_t proxy_on_configure = ngx_string("proxy_on_configure");
static ngx_str_t proxy_on_done = ngx_string("proxy_on_done");
static ngx_str_t proxy_on_delete = ngx_string("proxy_on_delete");
static ngx_str_t proxy_on_request_headers =
    ngx_string("proxy_on_request_headers");
static ngx_str_t proxy_on_request_body =
    ngx_string("proxy_on_request_body");
static ngx_str_t proxy_on_response_headers =
    ngx_string("proxy_on_response_headers");
static ngx_str_t proxy_on_response_body =
    ngx_string("proxy_on_response_body");
static ngx_str_t proxy_on_http_call_response =
    ngx_string("proxy_on_http_call_response");


typedef enum {
    HTTP_REQUEST_HEADERS = 1,
    HTTP_REQUEST_BODY = 2,
    HTTP_RESPONSE_HEADERS = 4,
    HTTP_RESPONSE_BODY = 8,
} ngx_http_wasm_phase_t;


enum {
    RC_NEED_HTTP_CALL = 1,
};


static ngx_command_t ngx_http_wasm_cmds[] = {
    { ngx_string("wasm_vm"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
      ngx_http_wasm_vm,
      NGX_HTTP_MAIN_CONF_OFFSET,
      0,
      NULL },

    ngx_null_command
};


static ngx_http_module_t ngx_http_wasm_module_ctx = {
    NULL,                                    /* preconfiguration */
    ngx_http_wasm_init,                      /* postconfiguration */

    ngx_http_wasm_create_main_conf,          /* create main configuration */
    ngx_http_wasm_init_main_conf,            /* init main configuration */

    NULL,                                    /* create server configuration */
    NULL,                                    /* merge server configuration */

    NULL,                                    /* create location configuration */
    NULL                                     /* merge location configuration */
};


ngx_module_t ngx_http_wasm_module = {
    NGX_MODULE_V1,
    &ngx_http_wasm_module_ctx,           /* module context */
    ngx_http_wasm_cmds,                  /* module directives */
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


static void *
ngx_http_wasm_create_main_conf(ngx_conf_t *cf)
{
    ngx_http_wasm_main_conf_t    *wmcf;

    wmcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_wasm_main_conf_t));
    if (wmcf == NULL) {
        return NULL;
    }

    /* set by ngx_pcalloc:
     *      wmcf->vm = { 0, NULL };
     *      wmcf->code = 0;
     *      wmcf->body = { 0, NULL };
     */

    return wmcf;
}


static char *
ngx_http_wasm_init_main_conf(ngx_conf_t *cf, void *conf)
{
    return NGX_CONF_OK;
}


static char *
ngx_http_wasm_vm(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_wasm_main_conf_t       *wmcf = conf;
    ngx_str_t                       *value;

    if (wmcf->vm.len != 0) {
        return "is duplicate";
    }

    value = cf->args->elts;
    wmcf->vm.len = value[1].len;
    wmcf->vm.data = value[1].data;

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_wasm_init(ngx_conf_t *cf)
{
    ngx_int_t                   rc;
    ngx_pool_cleanup_t         *cln;
    ngx_http_wasm_main_conf_t  *wmcf;

    if (ngx_process == NGX_PROCESS_SIGNALLER || ngx_test_config || ngx_http_wasm_vm_inited) {
        return NGX_OK;
    }

    rc = ngx_http_wasm_resolve_symbol();
    if (rc != NGX_OK) {
        return rc;
    }

    wmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_wasm_module);
    if (wmcf->vm.len == 0) {
        return NGX_OK;
    }

    cln = ngx_pool_cleanup_add(cf->pool, 0);
    if (cln == NULL) {
        return NGX_ERROR;
    }

    cln->handler = ngx_wasm_vm_cleanup;

    rc = ngx_wasm_vm_init(&wmcf->vm);
    if (rc != NGX_OK) {
        return rc;
    }

    cln->data = ngx_wasm_vm;

    /* don't reinit during reconfiguring */
    ngx_http_wasm_vm_inited = true;

    return NGX_OK;
}


/* To avoid complex error handling, we choose to allocate several objects together.
 * The downside is that the objects are not alignment, but it is fine under x86 & ARM64, which
 * are the only platforms supported by most of the Wasm VM. */
__attribute__((no_sanitize("undefined")))
void *
ngx_http_wasm_load_plugin(const char *name, size_t name_len,
                          const char *bytecode, size_t size)
{
    void                    *plugin;
    ngx_int_t                rc, i;
    bool                     found;
    ngx_http_wasm_plugin_t  *hw_plugin;

    if (!ngx_http_wasm_vm_inited) {
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "miss wasm_vm configuration");
        return NULL;
    }

    plugin = ngx_wasm_vm->load(bytecode, size);

    if (plugin == NULL) {
        return NULL;
    }

    if (ngx_wasm_vm->has(plugin, &plugin_start)) {
        rc = ngx_wasm_vm->call(plugin, &plugin_start, false, NGX_WASM_PARAM_VOID);
    } else if (ngx_wasm_vm->has(plugin, &plugin_initialize)) {
        rc = ngx_wasm_vm->call(plugin, &plugin_initialize, false, NGX_WASM_PARAM_VOID);
    } else {
        rc = NGX_OK;
    }

    if (rc != NGX_OK) {
        goto free_plugin;
    }

    hw_plugin = ngx_calloc(sizeof(ngx_http_wasm_plugin_t) + name_len +
                           sizeof(ngx_http_wasm_state_t), ngx_cycle->log);
    if (hw_plugin == NULL) {
        goto free_plugin;
    }

    hw_plugin->cur_ctx_id = 0;
    hw_plugin->plugin = plugin;

    /* assumed using latest ABI by default */
    hw_plugin->abi_version = PROXY_WASM_ABI_VER_MAX;
    for (i = 0; abi_versions[i].len != 0; i++) {
        found = ngx_wasm_vm->has(plugin, &abi_versions[i]);
        if (found) {
            hw_plugin->abi_version = i;
        }
    }

    hw_plugin->name.len = name_len;
    hw_plugin->name.data = (u_char *) (hw_plugin + 1);
    ngx_memcpy(hw_plugin->name.data, name, name_len);

    hw_plugin->state = (ngx_http_wasm_state_t *) (hw_plugin->name.data + name_len);
    hw_plugin->state->plugin_name = &hw_plugin->name;

    ngx_queue_init(&hw_plugin->occupied);
    ngx_queue_init(&hw_plugin->free);
    return hw_plugin;

free_plugin:
    ngx_wasm_vm->unload(plugin);
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

    ngx_wasm_vm->unload(plugin);

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
ngx_http_wasm_free_plugin_ctx(ngx_http_wasm_plugin_ctx_t *hwp_ctx)
{
    ngx_int_t                        rc;
    uint32_t                         ctx_id = hwp_ctx->id;
    ngx_http_wasm_plugin_t          *hw_plugin = hwp_ctx->hw_plugin;
    void                            *plugin = hw_plugin->plugin;
    ngx_log_t                       *log;

    log = ngx_cycle->log;

    if (!ngx_queue_empty(&hwp_ctx->occupied)) {
        /* some http ctxs are using it. Do not free */
        return;
    }

    ngx_queue_remove(&hwp_ctx->queue);
    ngx_queue_insert_head(&hw_plugin->free, &hwp_ctx->queue);

    rc = ngx_wasm_vm->call(plugin, &proxy_on_done, true, NGX_WASM_PARAM_I32, ctx_id);
    if (rc <= 0) {
        ngx_log_error(NGX_LOG_ERR, log, 0, "failed to mark context %d as done, rc: %d",
                      ctx_id, rc);
    }

    rc = ngx_wasm_vm->call(plugin, &proxy_on_delete, false, NGX_WASM_PARAM_I32, ctx_id);
    if (rc != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, log, 0, "failed to delete context %d, rc: %d",
                      ctx_id, rc);
    }

    if (hwp_ctx->pool != NULL) {
        ngx_destroy_pool(hwp_ctx->pool);
        hwp_ctx->pool = NULL;

        /* http_ctx destroyed in pool should not be referred in the queue */
        ngx_queue_init(&hwp_ctx->occupied);
        ngx_queue_init(&hwp_ctx->free);
    }

    ngx_log_error(NGX_LOG_INFO, log, 0, "free plugin context %d", ctx_id);

    if (hw_plugin->done) {
        ngx_http_wasm_free_plugin(hw_plugin);
    }
}


void
ngx_http_wasm_delete_plugin_ctx(ngx_http_wasm_plugin_ctx_t *hwp_ctx)
{
    hwp_ctx->done = 1;
    ngx_http_wasm_free_plugin_ctx(hwp_ctx);
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

    if (!ngx_http_wasm_vm_inited) {
        ngx_log_error(NGX_LOG_ERR, log, 0, "miss wasm_vm configuration");
        return NULL;
    }

    if (!ngx_queue_empty(&hw_plugin->free)) {
        ngx_queue_t         *q;

        q = ngx_queue_last(&hw_plugin->free);
        ngx_queue_remove(q);
        hwp_ctx = ngx_queue_data(q, ngx_http_wasm_plugin_ctx_t, queue);
        hwp_ctx->done = 0;
        ctx_id = hwp_ctx->id;

    } else {
        hwp_ctx = ngx_calloc(sizeof(ngx_http_wasm_plugin_ctx_t), log);
        if (hwp_ctx == NULL) {
            return NULL;
        }

        hw_plugin->cur_ctx_id++;
        ctx_id = hw_plugin->cur_ctx_id;
        hwp_ctx->id = ctx_id;
        hwp_ctx->hw_plugin = hw_plugin;
        ngx_queue_init(&hwp_ctx->occupied);
        ngx_queue_init(&hwp_ctx->free);
    }

    ngx_http_wasm_set_state(hw_plugin->state);

    rc = ngx_wasm_vm->call(plugin, &proxy_on_context_create, false,
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
    hwp_ctx->state->r = NULL;
    hwp_ctx->state->plugin_name = &hwp_ctx->hw_plugin->name;

    state_conf = (u_char *) (hwp_ctx->state + 1);
    /* copy conf so we can access it anytime */
    ngx_memcpy(state_conf, conf, size);

    hwp_ctx->state->conf.data = state_conf;
    hwp_ctx->state->conf.len = size;

    ngx_http_wasm_set_state(hwp_ctx->state);

    rc = ngx_wasm_vm->call(plugin, &proxy_on_configure, true,
                           NGX_WASM_PARAM_I32_I32, ctx_id, size);

    ngx_http_wasm_set_state(NULL);

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


ngx_http_wasm_http_ctx_t *
ngx_http_wasm_create_http_ctx(ngx_http_wasm_plugin_ctx_t *hwp_ctx, ngx_http_request_t *r)
{
    ngx_int_t                        rc;
    uint32_t                         ctx_id;
    ngx_http_wasm_http_ctx_t        *http_ctx;
    ngx_http_wasm_plugin_t          *hw_plugin = hwp_ctx->hw_plugin;
    void                            *plugin = hw_plugin->plugin;
    ngx_log_t                       *log;

    log = r->connection->log;

    if (!ngx_queue_empty(&hwp_ctx->free)) {
        ngx_queue_t         *q;

        q = ngx_queue_last(&hwp_ctx->free);
        ngx_queue_remove(q);
        http_ctx = ngx_queue_data(q, ngx_http_wasm_http_ctx_t, queue);
        ctx_id = http_ctx->id;

    } else {
        http_ctx = ngx_pcalloc(hwp_ctx->pool, sizeof(ngx_http_wasm_http_ctx_t));
        if (http_ctx == NULL) {
            ngx_log_error(NGX_LOG_ERR, log, 0, "no memory");
            return NULL;
        }

        hw_plugin->cur_ctx_id++;
        ctx_id = hw_plugin->cur_ctx_id;
        http_ctx->id = ctx_id;
        http_ctx->hwp_ctx = hwp_ctx;
    }

    rc = ngx_wasm_vm->call(plugin, &proxy_on_context_create, false,
                          NGX_WASM_PARAM_I32_I32, ctx_id, hwp_ctx->id);
    if (rc != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, log, 0, "failed to create context %d, rc: %d",
                      ctx_id, rc);
        /* reuse the ctx_id */
        ngx_queue_insert_head(&hwp_ctx->free, &http_ctx->queue);
        return NULL;
    }

    ngx_queue_insert_head(&hwp_ctx->occupied, &http_ctx->queue);

    ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, 0, "create http context %d", ctx_id);

    return http_ctx;
}


static void
ngx_http_wasm_cleanup(void *data)
{
    ngx_int_t                     rc;
    ngx_uint_t                    i;
    ngx_http_wasm_ctx_t          *ctx = data;
    ngx_array_t                  *http_ctxs = ctx->http_ctxs;
    uint32_t                      ctx_id;
    ngx_http_wasm_http_ctx_t     *http_ctx;
    ngx_http_wasm_plugin_ctx_t   *hwp_ctx;
    void                         *plugin;
    ngx_log_t                    *log;

    log = ngx_cycle->log;

    if (http_ctxs == NULL) {
        return;
    }

    for (i = 0; i < http_ctxs->nelts; i++) {
        http_ctx = ((ngx_http_wasm_http_ctx_t **) http_ctxs->elts)[i];
        ctx_id = http_ctx->id;
        hwp_ctx = http_ctx->hwp_ctx;
        plugin = hwp_ctx->hw_plugin->plugin;

        ngx_queue_remove(&http_ctx->queue);
        ngx_queue_insert_head(&hwp_ctx->free, &http_ctx->queue);

        rc = ngx_wasm_vm->call(plugin, &proxy_on_done, true, NGX_WASM_PARAM_I32, ctx_id);
        if (rc <= 0) {
            ngx_log_error(NGX_LOG_ERR, log, 0, "failed to mark context %d as done, rc: %d",
                          ctx_id, rc);
        }

        rc = ngx_wasm_vm->call(plugin, &proxy_on_delete, false, NGX_WASM_PARAM_I32, ctx_id);
        if (rc != NGX_OK) {
            ngx_log_error(NGX_LOG_ERR, log, 0, "failed to delete context %d, rc: %d",
                          ctx_id, rc);
        }

        ngx_log_error(NGX_LOG_INFO, log, 0, "free http context %d", ctx_id);

        if (hwp_ctx->done) {
            ngx_http_wasm_free_plugin_ctx(hwp_ctx);
        }
    }
}


ngx_http_wasm_ctx_t *
ngx_http_wasm_get_module_ctx(ngx_http_request_t *r)
{
    ngx_http_wasm_ctx_t       *ctx;
    ngx_pool_cleanup_t        *cln;

    ctx = ngx_http_get_module_ctx(r, ngx_http_wasm_module);

    if (ctx == NULL) {
        ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_wasm_ctx_t));
        if (ctx == NULL) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "no memory");
            return NULL;
        }

        cln = ngx_pool_cleanup_add(r->pool, 0);
        if (cln == NULL) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "no memory");
            return NULL;
        }

        cln->data = ctx;
        cln->handler = ngx_http_wasm_cleanup;

        ngx_http_set_ctx(r, ctx, ngx_http_wasm_module);
    }

    return ctx;
}


ngx_http_wasm_http_ctx_t *
ngx_http_wasm_fetch_http_ctx(ngx_http_wasm_plugin_ctx_t *hwp_ctx, ngx_http_request_t *r)
{
    ngx_uint_t                   i;
    ngx_http_wasm_ctx_t         *ctx;
    ngx_http_wasm_http_ctx_t    *http_ctx;
    ngx_http_wasm_http_ctx_t   **p;


    ctx = ngx_http_wasm_get_module_ctx(r);
    if (ctx == NULL) {
        return NULL;
    }

    if (ctx->http_ctxs == NULL) {
        ctx->http_ctxs = ngx_array_create(r->pool, 1, sizeof(ngx_http_wasm_ctx_t *));
        if (ctx->http_ctxs == NULL) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "no memory");
            return NULL;
        }
    }

    p = ctx->http_ctxs->elts;

    for (i = 0; i < ctx->http_ctxs->nelts; i++) {
        if (p[i]->hwp_ctx == hwp_ctx) {
            return p[i];
        }
    }

    http_ctx = ngx_http_wasm_create_http_ctx(hwp_ctx, r);
    if (http_ctx == NULL) {
        return NULL;
    }

    p = ngx_array_push(ctx->http_ctxs);
    *p = http_ctx;
    return http_ctx;
}


static ngx_int_t
ngx_http_wasm_handle_rc_before_proxy(ngx_http_wasm_main_conf_t *wmcf,
                                     ngx_http_wasm_ctx_t *ctx, ngx_int_t rc)
{
    int32_t code = wmcf->code;

    /* reset code for next use */
    wmcf->code = 0;

    /* the http call is prior to other operation */
    if (ctx->callout) {
        return RC_NEED_HTTP_CALL;
    }

    if (rc < 0) {
        return rc;
    }

    if (code >= 100) {
        /* Return given http response instead of reaching the upstream.
         * The body will be fetched later by ngx_http_wasm_fetch_local_body
         * */
        return code;
    }

    return rc;
}


ngx_int_t
ngx_http_wasm_on_http(ngx_http_wasm_plugin_ctx_t *hwp_ctx, ngx_http_request_t *r,
                      ngx_http_wasm_phase_t type, const u_char *body, size_t size,
                      int end_of_body)
{
    ngx_int_t                        rc;
    ngx_log_t                       *log;
    ngx_str_t                       *cb_name;
    ngx_http_wasm_ctx_t             *ctx;
    ngx_http_wasm_http_ctx_t        *http_ctx;
    ngx_http_wasm_main_conf_t       *wmcf;

    log = r->connection->log;

    if (!ngx_http_wasm_vm_inited) {
        ngx_log_error(NGX_LOG_ERR, log, 0, "miss wasm_vm configuration");
        return NGX_DECLINED;
    }

    wmcf = ngx_http_get_module_main_conf(r, ngx_http_wasm_module);
    hwp_ctx->state->r = r;

    if (body != NULL) {
        hwp_ctx->state->body.data = (u_char *) body;
        hwp_ctx->state->body.len = size;
    }

    ngx_http_wasm_set_state(hwp_ctx->state);

    http_ctx = ngx_http_wasm_fetch_http_ctx(hwp_ctx, r);
    if (http_ctx == NULL) {
        ngx_http_wasm_set_state(NULL);
        return NGX_DECLINED;
    }

    ctx = ngx_http_wasm_get_module_ctx(r);

    if (type == HTTP_REQUEST_HEADERS) {
        cb_name = &proxy_on_request_headers;
    } else if (type == HTTP_REQUEST_BODY) {
        cb_name = &proxy_on_request_body;
    } else if (type == HTTP_RESPONSE_HEADERS) {
        cb_name = &proxy_on_response_headers;
    } else {
        cb_name = &proxy_on_response_body;
    }

    if (type == HTTP_REQUEST_HEADERS || type == HTTP_RESPONSE_HEADERS) {
        if (hwp_ctx->hw_plugin->abi_version == PROXY_WASM_ABI_VER_010) {
            rc = ngx_wasm_vm->call(hwp_ctx->hw_plugin->plugin,
                                   cb_name,
                                   true, NGX_WASM_PARAM_I32_I32, http_ctx->id, 0);
        } else {
            rc = ngx_wasm_vm->call(hwp_ctx->hw_plugin->plugin,
                                   cb_name,
                                   true, NGX_WASM_PARAM_I32_I32_I32, http_ctx->id,
                                   0, 1);
        }

    } else {
        rc = ngx_wasm_vm->call(hwp_ctx->hw_plugin->plugin,
                              cb_name,
                              true, NGX_WASM_PARAM_I32_I32_I32, http_ctx->id,
                              size, end_of_body);
    }

    ngx_http_wasm_set_state(NULL);

    return ngx_http_wasm_handle_rc_before_proxy(wmcf, ctx, rc);
}


ngx_str_t *
ngx_http_wasm_fetch_local_body(ngx_http_request_t *r)
{
    ngx_http_wasm_main_conf_t       *wmcf;

    /* call after ngx_http_wasm_on_http */

    wmcf = ngx_http_get_module_main_conf(r, ngx_http_wasm_module);
    if (wmcf->body.len) {
        return &wmcf->body;
    }
    return NULL;
}


ngx_int_t
ngx_http_wasm_on_http_call_resp(ngx_http_wasm_plugin_ctx_t *hwp_ctx, ngx_http_request_t *r,
                                proxy_wasm_table_elt_t *headers, ngx_uint_t n_header,
                                ngx_str_t *body)
{
    ngx_int_t                        rc;
    ngx_log_t                       *log;
    ngx_http_wasm_ctx_t             *ctx;
    ngx_http_wasm_http_ctx_t        *http_ctx;
    ngx_http_wasm_main_conf_t       *wmcf;

    log = r->connection->log;
    wmcf = ngx_http_get_module_main_conf(r, ngx_http_wasm_module);

    if (!ngx_http_wasm_vm_inited) {
        ngx_log_error(NGX_LOG_ERR, log, 0, "miss wasm_vm configuration");
        return NGX_DECLINED;
    }

    hwp_ctx->state->r = r;
    ngx_http_wasm_set_state(hwp_ctx->state);

    http_ctx = ngx_http_wasm_fetch_http_ctx(hwp_ctx, r);
    if (http_ctx == NULL) {
        ngx_http_wasm_set_state(NULL);
        return NGX_DECLINED;
    }

    ctx = ngx_http_wasm_get_module_ctx(r);
    ctx->call_resp_headers = headers;
    ctx->call_resp_n_header = n_header;
    ctx->call_resp_body = body;

    ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                  "run http callback callout id: %d, plugin ctx id: %d",
                  ctx->callout_id, hwp_ctx->id);

    rc = ngx_wasm_vm->call(hwp_ctx->hw_plugin->plugin,
                          &proxy_on_http_call_response,
                          false, NGX_WASM_PARAM_I32_I32_I32_I32_I32,
                          hwp_ctx->id, ctx->callout_id,
                          n_header, body == NULL ? 0 : body->len, 0);

    ngx_http_wasm_set_state(NULL);

    return ngx_http_wasm_handle_rc_before_proxy(wmcf, ctx, rc);
}
