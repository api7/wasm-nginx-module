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
	return &httpContext{
		contextID: contextID,
		metrics:   NewMetrics(),
	}
}

type httpContext struct {
	types.DefaultHttpContext
	metrics   *TestMetrics
	contextID uint32
}

// Override types.DefaultHttpContext.
func (ctx *httpContext) OnHttpRequestHeaders(numHeaders int, endOfStream bool) types.Action {
	data, err := proxywasm.GetPluginConfiguration()
	if err != nil {
		proxywasm.LogErrorf("error reading plugin configuration: %v", err)
		return types.ActionContinue
	}

	proxywasm.LogWarnf("OnHttpRequestHeaders: %s", string(data))

	ctx.metrics.incrementCounter("test")
	ctx.metrics.recordMetricHistogram("test")

	return types.ActionContinue
}

type TestMetrics struct {
	c map[string]proxywasm.MetricCounter
	h map[string]proxywasm.MetricHistogram
}

func NewMetrics() *TestMetrics {
	return &TestMetrics{
		c: make(map[string]proxywasm.MetricCounter),
		h: make(map[string]proxywasm.MetricHistogram),
	}
}

// ProxyDefineMetric
// ProxyIncrementMetric
func (m *TestMetrics) incrementCounter(name string) {
	counter, ok := m.c[name]
	if !ok {
		counter = proxywasm.DefineCounterMetric(name)
		m.c[name] = counter
	}
	proxywasm.LogWarnf("DefineCounterMetric success: %s", name)

	counter.Increment(1)
	proxywasm.LogWarnf("Increment success: %s", name)

}

// ProxyGetMetric
func (m *TestMetrics) getMetric(name string) uint64 {
	return m.c[name].Value()
}

func (m *TestMetrics) recordMetricHistogram(name string) {
	h, ok := m.h[name]
	if !ok {
		h = proxywasm.DefineHistogramMetric(name)
		m.h[name] = h
	}
	proxywasm.LogWarnf("DefineHistogramMetric success: %s", name)

	h.Record(uint64(1))
	proxywasm.LogWarnf("Record success: %s", name)
}
