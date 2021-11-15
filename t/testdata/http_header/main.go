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

func (ctx *httpContext) OnHttpResponseHeaders(numHeaders int, endOfStream bool) types.Action {
	data, err := proxywasm.GetPluginConfiguration()
	if err != nil {
		proxywasm.LogCriticalf("error reading plugin configuration: %v", err)
		return types.ActionContinue
	}

	action := string(data)
	proxywasm.AddHttpResponseHeader("add", "foo")

	if action == "resp_hdr_add" {
		proxywasm.AddHttpResponseHeader("add", "bar")
	} else if action == "resp_hdr_set" {
		proxywasm.ReplaceHttpResponseHeader("add", "bar")
	} else if action == "resp_hdr_set_empty" {
		proxywasm.ReplaceHttpResponseHeader("add", "")
	}

	return types.ActionContinue
}
