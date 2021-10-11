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
	return &pluginLifecycle{}
}

type pluginLifecycle struct {
	types.DefaultPluginContext
	contextID uint32
}

func (ctx *pluginLifecycle) OnPluginStart(pluginConfigurationSize int) types.OnPluginStartStatus {
	proxywasm.LogCriticalf("%s", "ouch, something is wrong")
	proxywasm.LogError("ouch, something is wrong")
	proxywasm.LogWarn("ouch, something is wrong")
	proxywasm.LogInfof("ouch,%s", " something is wrong")
	proxywasm.LogDebug("ouch, something is wrong")
	proxywasm.LogTrace("ouch, something is wrong")
	return types.OnPluginStartStatusOK
}

func (*pluginLifecycle) NewHttpContext(contextID uint32) types.HttpContext {
	proxywasm.LogWarnf("create http ctx %d", contextID)
	return &httpLifecycle{contextID: contextID}
}

type httpLifecycle struct {
	types.DefaultHttpContext
	contextID uint32
}

func (ctx *httpLifecycle) OnHttpRequestHeaders(numHeaders int, endOfStream bool) types.Action {
	proxywasm.LogWarnf("run http ctx %d", ctx.contextID)
	return types.ActionContinue
}
