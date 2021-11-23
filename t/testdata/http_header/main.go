package main

import (
	"github.com/tetratelabs/proxy-wasm-go-sdk/proxywasm"
	"github.com/tetratelabs/proxy-wasm-go-sdk/proxywasm/types"
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

	action := string(data)
	switch action {
	case "req_hdr_get":
		res, err := proxywasm.GetHttpRequestHeader("X-API")
		if err != nil {
			proxywasm.LogErrorf("error get request header: %v", err)
			return types.ActionContinue
		}
		proxywasm.LogWarnf("get request header: %v", res)
	case "req_hdr_get_caseless":
		res, err := proxywasm.GetHttpRequestHeader("x_api")
		if err != nil {
			proxywasm.LogErrorf("error get request header: %v", err)
			return types.ActionContinue
		}
		proxywasm.LogWarnf("get request header: %v", res)
	}

	return types.ActionContinue
}

func (ctx *httpContext) OnHttpResponseHeaders(numHeaders int, endOfStream bool) types.Action {
	data, err := proxywasm.GetPluginConfiguration()
	if err != nil {
		proxywasm.LogErrorf("error reading plugin configuration: %v", err)
		return types.ActionContinue
	}

	action := string(data)
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
	case "resp_hdr_get":
		res, err := proxywasm.GetHttpResponseHeader("add")
		if err != nil {
			proxywasm.LogErrorf("error get response header: %v", err)
			return types.ActionContinue
		}
		proxywasm.LogWarnf("get response header: %v", res)
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
	}

	return types.ActionContinue
}
