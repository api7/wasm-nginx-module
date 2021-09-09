#include "ngx_http_wasm_api.h"


wasm_functype_t *
ngx_http_wasm_host_api_func(const ngx_wasm_host_api_t *api)
{
    int                i;
    wasm_valtype_vec_t param_vec, result_vec;
    wasm_valtype_t    *param[3];
    wasm_valtype_t    *result[1];

    for (i = 0; i < api->param_num; i++) {
        param[i] = wasm_valtype_new(api->param_type[i]);
    }

    if (api->ret_type == WASM_VOID) {
        wasm_valtype_vec_new(&param_vec, api->param_num, param);
        wasm_valtype_vec_new_empty(&result_vec);

    } else {
        result[0] = wasm_valtype_new(api->ret_type);
        wasm_valtype_vec_new(&param_vec, api->param_num, param);
        wasm_valtype_vec_new(&result_vec, 1, result);
    }

    return wasm_functype_new(&param_vec, &result_vec);
}
