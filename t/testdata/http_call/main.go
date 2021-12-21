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
	return &pluginContext{contextID: contextID}
}

type pluginContext struct {
	types.DefaultPluginContext
	contextID uint32
}

func (pluginCtx *pluginContext) NewHttpContext(contextID uint32) types.HttpContext {
	ctx := &httpContext{contextID: contextID, pluginCtxID: pluginCtx.contextID}
	ctx.callback = func(numHeaders, bodySize, numTrailers int) {
		proxywasm.LogInfof("called for contextID = %d", ctx.contextID)
	}
	return ctx
}

type httpContext struct {
	types.DefaultHttpContext
	contextID   uint32
	pluginCtxID uint32
	callback    func(numHeaders, bodySize, numTrailers int)
}

func (ctx *httpContext) dispatchHttpCall(elem *fastjson.Value) {
	host := elem.GetStringBytes("host")
	path := elem.GetStringBytes("path")
	method := elem.GetStringBytes("method")
	scheme := elem.GetStringBytes("scheme")
	headers := elem.GetArray("headers")
	body := elem.GetStringBytes("body")

	timeout := uint32(elem.GetInt("timeout"))
	if timeout == 0 {
		timeout = 5000
	}

	action := string(elem.GetStringBytes("action"))
	switch action {
	case "panic":
		ctx.callback = func(numHeaders int, bodySize int, numTrailers int) {
			panic("OUCH")
		}
	case "call_and_send":
		if err := proxywasm.SendHttpResponse(503, nil, nil, -1); err != nil {
			proxywasm.LogErrorf("send http failed: %v", err)
		}
	}

	hs := [][2]string{}
	if len(path) > 0 {
		hs = append(hs, [2]string{":path", string(path)})
	}
	if len(method) > 0 {
		hs = append(hs, [2]string{":method", string(method)})
	}
	if len(scheme) > 0 {
		hs = append(hs, [2]string{":scheme", string(scheme)})
	}
	if len(headers) > 0 {
		for _, e := range headers {
			kv := e.GetArray()
			k := string(kv[0].GetStringBytes())
			v := string(kv[1].GetStringBytes())

			proxywasm.LogInfof("with header %s: %s", k, v)

			hs = append(hs, [2]string{k, v})
		}
	}

	calloutID, err := proxywasm.DispatchHttpCall(string(host), hs, body, nil,
		timeout, ctx.callback)
	if err != nil {
		proxywasm.LogErrorf("httpcall failed: %v", err)
		return
	}
	proxywasm.LogInfof("httpcall calloutID %d, pluginCtxID %d", calloutID, ctx.pluginCtxID)
}

func (ctx *httpContext) OnHttpRequestHeaders(numHeaders int, endOfStream bool) types.Action {
	data, err := proxywasm.GetPluginConfiguration()
	if err != nil {
		proxywasm.LogErrorf("error reading plugin configuration: %v", err)
		return types.ActionContinue
	}

	var p fastjson.Parser
	v, err := p.ParseBytes(data)
	if err != nil {
		proxywasm.LogErrorf("erorr decoding plugin configuration: %v", err)
		return types.ActionContinue
	}

	if v.Type() == fastjson.TypeArray {
		arr := v.GetArray()
		for _, e := range arr {
			ctx.dispatchHttpCall(e)
		}

	} else {
		ctx.dispatchHttpCall(v)
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
	if action == "call_in_resp" {
		hs := [][2]string{
			{":method", "GET"}, {"host", "some_authority"},
			{":path", "/path/to/service"}, {"accept", "*/*"},
		}
		if _, err := proxywasm.DispatchHttpCall("web_service", hs, nil, nil,
			5000, ctx.callback); err != nil {
			proxywasm.LogErrorf("httpcall failed: %v", err)
		}
	}

	return types.ActionContinue
}
