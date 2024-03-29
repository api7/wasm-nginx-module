// Copyright 2022 Shenzhen ZhiLiu Technology Co., Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package main

import (
	"github.com/tetratelabs/proxy-wasm-go-sdk/proxywasm"
	"github.com/tetratelabs/proxy-wasm-go-sdk/proxywasm/types"
	"github.com/valyala/fastjson"
)

func main() {
	proxywasm.SetVMContext(&vmContext{})
}

type vmContext struct {
	types.DefaultVMContext
}

func (*vmContext) NewPluginContext(contextID uint32) types.PluginContext {
	return &pluginContext{}
}

type pluginContext struct {
	types.DefaultPluginContext
}

func (*pluginContext) NewHttpContext(contextID uint32) types.HttpContext {
	return &httpContext{contextID: contextID}
}

type httpContext struct {
	types.DefaultHttpContext
	contextID uint32
}

func (ctx *httpContext) OnHttpRequestHeaders(numHeaders int, endOfStream bool) types.Action {
	data, err := proxywasm.GetPluginConfiguration()
	if err != nil {
		proxywasm.LogErrorf("error reading plugin configuration: %v", err)
		return types.ActionContinue
	}

	var action string
	var conf *fastjson.Value
	if data[0] == '{' {
		var p fastjson.Parser
		conf, err = p.ParseBytes(data)
		if err != nil {
			proxywasm.LogErrorf("error decoding plugin configuration: %v", err)
			return types.ActionContinue
		}

		action = string(conf.GetStringBytes("action"))

	} else {
		action = string(data)
	}

	switch action {
	case "req_hdr_get_all":
		hdrs, err := proxywasm.GetHttpRequestHeaders()
		if err != nil {
			proxywasm.LogErrorf("error getting headers: %v", err)
			return types.ActionContinue
		}

		for _, kv := range hdrs {
			proxywasm.LogWarnf("get request header: %v %v", kv[0], kv[1])
		}

	case "req_hdr_get":
		res, err := proxywasm.GetHttpRequestHeader("X-API")
		if err != nil {
			proxywasm.LogErrorf("error get request header: %v", err)
			return types.ActionContinue
		}
		proxywasm.LogWarnf("get request header: %v", res)

	case "req_hdr_get_caseless":
		res, err := proxywasm.GetHttpRequestHeader("x-api")
		if err != nil {
			proxywasm.LogErrorf("error get request header: %v", err)
			return types.ActionContinue
		}
		proxywasm.LogWarnf("get request header: %v", res)

	case "req_hdr_get_abnormal":
		hdr := string(conf.GetStringBytes("header"))
		res, err := proxywasm.GetHttpRequestHeader(hdr)
		if err != nil {
			proxywasm.LogErrorf("error get request header: %v", err)
			return types.ActionContinue
		}
		proxywasm.LogWarnf("get request header: %v", res)

	case "req_hdr_set":
		proxywasm.ReplaceHttpRequestHeader("foo", "bar")

	case "req_hdr_add":
		proxywasm.AddHttpRequestHeader("foo", "bar")

	case "req_hdr_del":
		proxywasm.RemoveHttpRequestHeader("foo")

	case "req_hdr_set_abnormal":
		hdr := string(conf.GetStringBytes("header"))
		v := string(conf.GetStringBytes("value"))
		err := proxywasm.ReplaceHttpRequestHeader(hdr, v)
		if err != nil {
			proxywasm.LogErrorf("error set request header: %v", err)
			return types.ActionContinue
		}

	case "req_path_get":
		res, err := proxywasm.GetHttpRequestHeader(":path")
		if err != nil {
			proxywasm.LogErrorf("error get request path: %v", err)
			return types.ActionContinue
		}
		proxywasm.LogWarnf("get request path: %v", res)

	case "req_method_get":
		res, err := proxywasm.GetHttpRequestHeader(":method")
		if err != nil {
			proxywasm.LogErrorf("error get request method: %v", err)
			return types.ActionContinue
		}
		proxywasm.LogWarnf("get request method: %v", res)

	case "req_scheme_get":
		res, err := proxywasm.GetHttpRequestHeader(":scheme")
		if err != nil {
			proxywasm.LogErrorf("error get request scheme: %v", err)
			return types.ActionContinue
		}
		proxywasm.LogWarnf("get request scheme: %v", res)

	case "req_hdr_get_authority":
		authority, err := proxywasm.GetHttpRequestHeader(":authority")
		if err != nil {
			proxywasm.LogErrorf("error get request authority: %v", err)
			return types.ActionContinue
		}
		proxywasm.LogWarnf("get request authority: %v", authority)

	case "req_pseduo_del":
		proxywasm.RemoveHttpRequestHeader(":method")
		proxywasm.RemoveHttpRequestHeader(":path")

	case "req_path_set":
		val := string(conf.GetStringBytes("value"))
		proxywasm.ReplaceHttpRequestHeader(":path", val)

	case "req_method_set":
		val := string(conf.GetStringBytes("value"))
		proxywasm.ReplaceHttpRequestHeader(":method", val)

	}

	return types.ActionContinue
}

func (ctx *httpContext) OnHttpResponseHeaders(numHeaders int, endOfStream bool) types.Action {
	data, err := proxywasm.GetPluginConfiguration()
	if err != nil {
		proxywasm.LogErrorf("error reading plugin configuration: %v", err)
		return types.ActionContinue
	}

	var action string
	var conf *fastjson.Value
	if data[0] == '{' {
		var p fastjson.Parser
		conf, err = p.ParseBytes(data)
		if err != nil {
			proxywasm.LogErrorf("error decoding plugin configuration: %v", err)
			return types.ActionContinue
		}

		action = string(conf.GetStringBytes("action"))

	} else {
		action = string(data)
	}

	if action == "resp_hdr_get_all" {
		hdrs, err := proxywasm.GetHttpResponseHeaders()
		if err != nil {
			proxywasm.LogErrorf("error getting headers: %v", err)
			return types.ActionContinue
		}

		for _, kv := range hdrs {
			proxywasm.LogWarnf("get response header %v %v", kv[0], kv[1])
		}
		return types.ActionContinue
	}

	proxywasm.AddHttpResponseHeader("add", "foo")

	switch action {
	case "resp_hdr_add":
		proxywasm.AddHttpResponseHeader("add", "bar")
	case "resp_hdr_set":
		proxywasm.ReplaceHttpResponseHeader("add", "bar")
	case "resp_hdr_set_empty":
		proxywasm.ReplaceHttpResponseHeader("add", "")
	case "resp_hdr_del":
		proxywasm.RemoveHttpResponseHeader("add")
	case "resp_hdr_del_all":
		proxywasm.AddHttpResponseHeader("add", "bar")
		proxywasm.RemoveHttpResponseHeader("add")
	case "resp_hdr_del_miss":
		proxywasm.RemoveHttpResponseHeader("aa")
	case "resp_hdr_set_abnormal":
		hdr := string(conf.GetStringBytes("header"))
		v := string(conf.GetStringBytes("value"))
		err := proxywasm.ReplaceHttpResponseHeader(hdr, v)
		if err != nil {
			proxywasm.LogErrorf("error set request header: %v", err)
			return types.ActionContinue
		}

	case "resp_hdr_get":
		res, err := proxywasm.GetHttpResponseHeader("add")
		if err != nil {
			proxywasm.LogErrorf("error get response header: %v", err)
			return types.ActionContinue
		}
		proxywasm.LogWarnf("get response header: %v", res)
	case "resp_hdr_get_status":
		res, err := proxywasm.GetHttpResponseHeader(":status")
		if err != nil {
			proxywasm.LogErrorf("error get response header: %v", err)
			return types.ActionContinue
		}
		proxywasm.LogWarnf("get response header: :status %v", res)
	case "resp_hdr_get_miss":
		res, err := proxywasm.GetHttpResponseHeader("")
		if err != nil {
			proxywasm.LogErrorf("error get response header: %v", err)
			return types.ActionContinue
		}
		proxywasm.LogWarnf("get response header: [%v]", res)
	case "resp_hdr_get_first":
		proxywasm.AddHttpResponseHeader("add", "bar")
		res, err := proxywasm.GetHttpResponseHeader("add")
		if err != nil {
			proxywasm.LogErrorf("error get response header: %v", err)
			return types.ActionContinue
		}
		proxywasm.LogWarnf("get response header: %v", res)
	case "resp_hdr_get_abnormal":
		hdr := string(conf.GetStringBytes("header"))
		res, err := proxywasm.GetHttpResponseHeader(hdr)
		if err != nil {
			proxywasm.LogErrorf("error get response header: %v", err)
			return types.ActionContinue
		}
		proxywasm.LogWarnf("get response header: %v", res)

	case "resp_hdr_set_status":
		proxywasm.ReplaceHttpResponseHeader(":status", "501")
	case "resp_hdr_set_status_bad_val":
		proxywasm.ReplaceHttpResponseHeader(":status", "")
	case "resp_hdr_del_status":
		proxywasm.RemoveHttpResponseHeader(":status")
	}

	return types.ActionContinue
}
