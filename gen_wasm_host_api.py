#!/usr/bin/env python3
# Copyright 2022 Shenzhen ZhiLiu Technology Co., Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
import os
import re
import sys
from string import Template


max_wasm_api_arg = 12
header = """/*
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
/* Code generated by `./gen_wasm_host_api.py src/http`. DO NOT EDIT. */"""

api_def_tpl = """
$header
#ifndef NGX_HTTP_WASM_API_DEF_H
#define NGX_HTTP_WASM_API_DEF_H


#include <ngx_core.h>


$host_api_def


#endif
"""

tpl = """
$header
#ifndef $vm_api_header_name
#define $vm_api_header_name


#include <wasm.h>
$vm_header
#include <ngx_core.h>
#include "proxy_wasm/proxy_wasm_types.h"
#include "http/ngx_http_wasm_api_def.h"


#define MAX_WASM_API_ARG  $max_wasm_api_arg

$wasm_api_def


#endif
"""


def predefined_macro(vm):
    if vm == "wasmtime":
        vm_def = """
#define DEFINE_WASM_API(NAME, ARG_CHECK) \\
    static wasm_trap_t* wasmtime_##NAME( \\
        void *env, \\
        wasmtime_caller_t *caller, \\
        const wasmtime_val_t *args, \\
        size_t nargs, \\
        wasmtime_val_t *results, \\
        size_t nresults \\
    ) { \\
        ARG_CHECK \\
        results[0].kind = WASMTIME_I32; \\
        results[0].of.i32 = res; \\
        return NULL; \\
    }
#define DEFINE_WASM_NAME(NAME, ARG) \\
    {ngx_string(#NAME), wasmtime_##NAME, ARG},


typedef struct {
    ngx_str_t                name;
    wasmtime_func_callback_t cb;
    int8_t                   param_num;
    wasm_valkind_t           param_type[MAX_WASM_API_ARG];
} ngx_wasm_wasmtime_host_api_t;

"""
    elif vm == "wasmedge":
        vm_def = """
#define DEFINE_WASM_API(NAME, ARG_CHECK) \\
    static WasmEdge_Result wasmedge_##NAME( \\
        void *Data, \\
        WasmEdge_MemoryInstanceContext *MemCxt, \\
        const WasmEdge_Value *In, \\
        WasmEdge_Value *Out \\
    ) { \\
        ARG_CHECK \\
        Out[0] = WasmEdge_ValueGenI32(res); \\
        return WasmEdge_Result_Success; \\
    }
#define DEFINE_WASM_NAME(NAME, ARG) \\
    {ngx_string(#NAME), wasmedge_##NAME, ARG},


typedef struct {
    ngx_str_t                name;
    WasmEdge_HostFunc_t      cb;
    int8_t                   param_num;
    enum WasmEdge_ValType    param_type[MAX_WASM_API_ARG];
} ngx_wasm_wasmedge_host_api_t;

"""

    for i in range(max_wasm_api_arg + 1):
        if i == 0:
            void_def = """
#define DEFINE_WASM_NAME_ARG_VOID \\
    0, {}
#define DEFINE_WASM_API_ARG_CHECK_VOID(NAME) \\
    int32_t res = NAME();
"""
            vm_def += void_def
        else:
            param_s = ""
            if vm == "wasmtime":
                kind = "WASM_I32"
            else:
                kind = "WasmEdge_ValType_I32"
            for j in range(1, i + 1):
                if j % 5 == 1:
                    param_s += "    "
                param_s += kind + ", "
                if j % 5 == 0:
                    param_s += " \\\n"
            vm_def += """
#define DEFINE_WASM_NAME_ARG_I32_%d \\
    %d, { \\
%s}""" % (i, i, param_s)
            vm_def += """
#define DEFINE_WASM_API_ARG_CHECK_I32_%d(NAME) \\\n""" % (i)
            for j in range(i):
                if vm == "wasmtime":
                    vm_def += "    int32_t p%d = args[%d].of.i32; \\\n" % (j, j)
                elif vm == "wasmedge":
                    vm_def += "    int32_t p%d = WasmEdge_ValueGetI32(In[%d]); \\\n" % (j, j)
            param_s = ", ".join('p' + str(j) for j in range(i))
            vm_def += "    int32_t res = NAME(%s);\n" % (param_s)
    return vm_def


def get_host_apis(src_dir):
    apis = []
    with open(os.path.join(src_dir, "ngx_http_wasm_api.c")) as f:
        matching = False
        api = ""
        name = ""
        for line in f:
            m = re.match(r"^(proxy_\w+)\(", line)
            if m is not None:
                matching = True
                name = m.group(1)
            elif line[0] == '{' and matching:
                matching = False
                n_param = len(api.split(','))
                if "(void)" in api:
                    n_param -= 1
                apis.append({
                    "name": name,
                    "n_param": n_param
                })
                api = ""
            if matching:
                api += line.rstrip()
    return apis


if __name__ == '__main__':
    host_api_def = ""
    api_def = ""
    name_def = ""
    for i in range(max_wasm_api_arg + 1):
        if i == 0:
            host_api_def += "#define DEFINE_WASM_API_ARG_VOID void\n"
        else:
            param_suffix = "I32_%d" % i
            param_s = ""
            for j in range(1, i + 1):
                if j % 8 == 1:
                    if j != 1:
                        param_s += ","
                    param_s += " \\\n    "
                else:
                    param_s += ", "
                param_s += "int32_t"
            host_api_def += "#define DEFINE_WASM_API_ARG_%s%s\n" % (param_suffix, param_s)
    host_api_def += "\n\n"

    src_dir = sys.argv[1]
    apis = get_host_apis(src_dir)

    for api in apis:
        name = api["name"]
        n_param = api["n_param"]
        if n_param == 0:
            param_suffix = "VOID"
        else:
            param_suffix = "I32_%d" % n_param

        host_api_def += """int32_t %s(DEFINE_WASM_API_ARG_%s);\n""" % (name, param_suffix)
        api_def += """
DEFINE_WASM_API(%s,
                DEFINE_WASM_API_ARG_CHECK_%s(%s))""" % (name, param_suffix, name)

        name_def += "    DEFINE_WASM_NAME(%s, DEFINE_WASM_NAME_ARG_%s)\n" % (name, param_suffix)

    name_def += "    { ngx_null_string, NULL, 0, {} }\n};"

    s = Template(api_def_tpl)
    with open(os.path.join(src_dir, "ngx_http_wasm_api_def.h"), 'w') as f:
        f.write(s.substitute(header=header, host_api_def=host_api_def))

    s = Template(tpl)

    wasmtime_def = predefined_macro("wasmtime")
    wasmtime_def += api_def + "\n\nstatic ngx_wasm_wasmtime_host_api_t host_apis[] = {\n"
    wasmtime_def += name_def
    with open(os.path.join(src_dir, "ngx_http_wasm_api_wasmtime.h"), 'w') as f:
        f.write(s.substitute(
            header=header,
            vm_header="#include <wasmtime.h>",
            vm_api_header_name="NGX_HTTP_WASM_API_WASMTIME_H",
            wasm_api_def=wasmtime_def,
            max_wasm_api_arg=max_wasm_api_arg,
        ))

    wasmedge_def = predefined_macro("wasmedge")
    wasmedge_def += api_def + "\n\nstatic ngx_wasm_wasmedge_host_api_t host_apis[] = {\n"
    wasmedge_def += name_def
    with open(os.path.join(src_dir, "ngx_http_wasm_api_wasmedge.h"), 'w') as f:
        f.write(s.substitute(
            header=header,
            vm_header="#include <wasmedge/wasmedge.h>",
            vm_api_header_name="NGX_HTTP_WASM_API_WASMEDGE_H",
            wasm_api_def=wasmedge_def,
            max_wasm_api_arg=max_wasm_api_arg,
        ))
