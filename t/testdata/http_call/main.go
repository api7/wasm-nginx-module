package main

import (
	"sort"

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
	case "headers":
		ctx.callback = func(numHeaders int, bodySize int, numTrailers int) {
			proxywasm.LogWarnf("get numHeaders %d", numHeaders)
			hs, err := proxywasm.GetHttpCallResponseHeaders()
			if err != nil {
				proxywasm.LogErrorf("callback err: %v", err)
				return
			}

			sort.Slice(hs, func(i, j int) bool {
				if hs[i][0] == hs[j][0] {
					return hs[i][1] < hs[j][1]
				}
				return hs[i][0] < hs[j][0]
			})
			for _, h := range hs {
				proxywasm.LogWarnf("get header %s: %s", h[0], h[1])
			}
		}
	case "body":
		ctx.callback = func(numHeaders int, bodySize int, numTrailers int) {
			proxywasm.LogWarnf("get bodySize %d", bodySize)
			start := elem.GetInt("start")
			size := elem.GetInt("size")
			body, err := proxywasm.GetHttpCallResponseBody(start, size)
			if err != nil {
				proxywasm.LogErrorf("callback err: %v", err)
				return
			}
			proxywasm.LogWarnf("get body [%s]", string(body))
		}
	case "then_send":
		ctx.callback = func(numHeaders int, bodySize int, numTrailers int) {
			if err := proxywasm.SendHttpResponse(503, nil, nil, -1); err != nil {
				proxywasm.LogErrorf("send http failed: %v", err)
			}
		}
	case "then_call":
		ctx.callback = func(numHeaders int, bodySize int, numTrailers int) {
			cb := func(numHeaders int, bodySize int, numTrailers int) {
				if err := proxywasm.SendHttpResponse(403, nil, nil, -1); err != nil {
					proxywasm.LogErrorf("send http failed: %v", err)
				}
			}
			calloutID, err := proxywasm.DispatchHttpCall(string(host), hs, body, nil,
				timeout, cb)
			if err != nil {
				proxywasm.LogErrorf("httpcall failed: %v", err)
				return
			}
			proxywasm.LogInfof("httpcall calloutID %d, pluginCtxID %d", calloutID, ctx.pluginCtxID)
		}
	case "then_call_again":
		cb_builder := func(next func(numHeaders int, bodySize int, numTrailers int)) func(numHeaders int, bodySize int, numTrailers int) {
			return func(numHeaders int, bodySize int, numTrailers int) {
				calloutID, err := proxywasm.DispatchHttpCall(string(host), hs, body, nil,
					timeout, next)
				if err != nil {
					proxywasm.LogErrorf("httpcall failed: %v", err)
					return
				}
				proxywasm.LogInfof("httpcall calloutID %d, pluginCtxID %d", calloutID, ctx.pluginCtxID)
			}
		}
		cb := func(numHeaders int, bodySize int, numTrailers int) {
			if err := proxywasm.SendHttpResponse(401, nil, nil, -1); err != nil {
				proxywasm.LogErrorf("send http failed: %v", err)
			}
		}
		ctx.callback = cb_builder(cb_builder(cb))
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
