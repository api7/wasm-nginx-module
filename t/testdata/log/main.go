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
