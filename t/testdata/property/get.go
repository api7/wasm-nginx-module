package main

import (
	"github.com/tetratelabs/proxy-wasm-go-sdk/proxywasm"
	"github.com/tetratelabs/proxy-wasm-go-sdk/proxywasm/types"
)

func main() {
	proxywasm.SetVMContext(&vmContext{})
}

type vmContext struct {
	// Embed the default VM context here,
	// so that we don't need to reimplement all the methods.
	types.DefaultVMContext
}

// Override types.DefaultVMContext.
func (*vmContext) NewPluginContext(contextID uint32) types.PluginContext {
	return &pluginContext{}
}

type pluginContext struct {
	// Embed the default plugin context here,
	// so that we don't need to reimplement all the methods.
	types.DefaultPluginContext
}

// Override types.DefaultPluginContext.
func (*pluginContext) NewHttpContext(contextID uint32) types.HttpContext {
	return &httpLifecycle{contextID: contextID}
}

func (*pluginContext) OnPluginStart(pluginConfigurationSize int) types.OnPluginStartStatus {
	return true
}

type httpLifecycle struct {
	// Embed the default http context here,
	// so that we don't need to reimplement all the methods.
	types.DefaultHttpContext
	contextID uint32
}

func (ctx *httpLifecycle) OnHttpRequestHeaders(numHeaders int, endOfStream bool) types.Action {
	data, err := proxywasm.GetPluginConfiguration()
	if err != nil {
		proxywasm.LogErrorf("error reading plugin configuration: %v", err)
		return types.ActionContinue
	}

	name := string(data)
	value, err := proxywasm.GetProperty([]string{name})
	if err != nil {
		proxywasm.LogErrorf("error get property: %v", err)
		return types.ActionContinue
	}
	proxywasm.LogWarnf("get property: %v = %v", name, string(value))
	return types.ActionContinue
}
