package main

import (
	"log"
	"math/rand"
	"os"
	"time"

	"github.com/tetratelabs/proxy-wasm-go-sdk/proxywasm"
	"github.com/tetratelabs/proxy-wasm-go-sdk/proxywasm/types"
)

const tickMilliseconds uint32 = 1000

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
	return &pluginLifecycle{contextID: contextID}
}

type pluginLifecycle struct {
	// Embed the default plugin context here,
	// so that we don't need to reimplement all the methods.
	types.DefaultPluginContext
	contextID uint32
}

func writeFile(name string, data []byte, perm os.FileMode) error {
	f, err := os.OpenFile(name, os.O_WRONLY|os.O_CREATE|os.O_TRUNC, perm)
	if err != nil {
		return err
	}
	_, err = f.Write(data)
	if err1 := f.Close(); err1 != nil && err == nil {
		err = err1
	}
	return err
}

// Override types.DefaultPluginContext.
func (ctx *pluginLifecycle) OnPluginStart(pluginConfigurationSize int) types.OnPluginStartStatus {
	rand.Seed(time.Now().UnixNano())

	data, err := proxywasm.GetPluginConfiguration()
	if err != nil {
		proxywasm.LogCriticalf("error reading plugin configuration: %v", err)
		return types.OnPluginStartStatusFailed
	}

	proxywasm.LogInfof("plugin config: %s", string(data))

	// the HTTP support is TODO yet in tinygo: https://github.com/tinygo-org/tinygo/issues/1961
	//_, err := http.Get("http://www.baidu.com/robots.txt")

	err = writeFile("/tmp/x", []byte("Hello, World"), 0666)
	if err != nil {
		// should be ENOTCAPABLE in wasm's sandbox environment. For details, see:
		// https://github.com/bytecodealliance/wasmtime/blob/main/docs/WASI-tutorial.md#executing-in-wasmtime-runtime
		log.Printf("writeFile failed\n")
	}

	proxywasm.LogWarnf("proxy_on_configure %d", ctx.contextID)
	return types.OnPluginStartStatusOK
}

func (ctx *pluginLifecycle) OnPluginDone() bool {
	proxywasm.LogWarnf("proxy_on_done %d", ctx.contextID)
	return true
}
