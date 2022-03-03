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
//
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

func (ctx *httpContext) OnHttpRequestBody(bodySize int, endOfStream bool) types.Action {
	data, err := proxywasm.GetPluginConfiguration()
	if err != nil {
		proxywasm.LogErrorf("error reading plugin configuration: %v", err)
		return types.ActionContinue
	}

	proxywasm.LogWarnf("body size %d, end of stream %v", bodySize, endOfStream)

	var action string
	var conf *fastjson.Value
	var p fastjson.Parser
	conf, err = p.ParseBytes(data)
	if err != nil {
		proxywasm.LogErrorf("error decoding plugin configuration: %v", err)
		return types.ActionContinue
	}

	action = string(conf.GetStringBytes("action"))

	switch action {
	case "offset":
		start := conf.GetInt("start")
		size := conf.GetInt("size")
		body, err := proxywasm.GetHttpRequestBody(start, size)
		if err != nil {
			proxywasm.LogErrorf("get body err: %v", err)
			return types.ActionContinue
		}
		proxywasm.LogWarnf("get body [%s]", string(body))

	case "callback":
		host := conf.GetStringBytes("host")
		path := conf.GetStringBytes("path")
		callback := func(numHeaders int, bodySize int, numTrailers int) {
			if err := proxywasm.SendHttpResponse(503, nil, nil, -1); err != nil {
				proxywasm.LogErrorf("send http failed: %v", err)
			}
		}
		_, err := proxywasm.DispatchHttpCall(string(host), [][2]string{{":path", string(path)}},
			[]byte(""), nil, 5000, callback)
		if err != nil {
			proxywasm.LogErrorf("httpcall failed: %v", err)
		}
	}

	return types.ActionContinue
}

func (ctx *httpContext) OnHttpResponseBody(bodySize int, endOfStream bool) types.Action {
	data, err := proxywasm.GetPluginConfiguration()
	if err != nil {
		proxywasm.LogErrorf("error reading plugin configuration: %v", err)
		return types.ActionContinue
	}

	proxywasm.LogWarnf("body size %d, end of stream %v", bodySize, endOfStream)

	var action string
	var conf *fastjson.Value
	var p fastjson.Parser
	conf, err = p.ParseBytes(data)
	if err != nil {
		proxywasm.LogErrorf("error decoding plugin configuration: %v", err)
		return types.ActionContinue
	}

	action = string(conf.GetStringBytes("action"))

	switch action {
	case "offset":
		if !endOfStream {
			return types.ActionContinue
		}
		start := conf.GetInt("start")
		size := conf.GetInt("size")
		body, err := proxywasm.GetHttpRequestBody(start, size)
		if err != nil {
			proxywasm.LogErrorf("get body err: %v", err)
			return types.ActionContinue
		}
		proxywasm.LogWarnf("get body [%s]", string(body))
	}

	return types.ActionContinue
}
