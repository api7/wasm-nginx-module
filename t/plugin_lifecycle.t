use t::WASM 'no_plan';

run_tests();

__DATA__

=== TEST 1: load
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        assert(wasm.load("plugin", "t/testdata/plugin_lifecycle/main.go.wasm"))
    }
}



=== TEST 2: load repeatedly
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        for i = 1, 3 do
            assert(wasm.load("plugin", "t/testdata/plugin_lifecycle/main.go.wasm"))
        end
    }
}



=== TEST 3: on_configure
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = wasm.load("plugin", "t/testdata/plugin_lifecycle/main.go.wasm")
        assert(wasm.on_configure(plugin, '{"body":512}'))
    }
}
--- grep_error_log eval
qr/writeFile failed/
--- grep_error_log_out
writeFile failed
--- error_log
plugin config: {"body":512}



=== TEST 4: on_configure, repeatedly
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = wasm.load("plugin", "t/testdata/plugin_lifecycle/main.go.wasm")
        local manager = {
            plugin = plugin,
            ctxs = {}
        }
        for i = 1, 3 do
            local ctx = assert(wasm.on_configure(plugin, '{"body":512}'))
            table.insert(manager.ctxs, ctx)
        end
    }
}
--- grep_error_log eval
qr/proxy_on_configure \d+/
--- grep_error_log_out eval
qr/proxy_on_configure 1
proxy_on_configure 2
proxy_on_configure 3
/
--- shutdown_error_log eval
qr/proxy_on_done [123]/



=== TEST 5: reuse plugin ctx id
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = wasm.load("plugin", "t/testdata/plugin_lifecycle/main.go.wasm")
        local ref
        for i = 1, 2 do
            do
                for j = 1, 2 do
                    local ctx = assert(wasm.on_configure(plugin, '{"body":512}'))
                end
                if i == 1 then
                    ref = assert(wasm.on_configure(plugin, '{"body":512}'))
                end

                collectgarbage()
            end
        end
    }
}
--- grep_error_log eval
qr/proxy_on_(configure|done) \d+/
--- grep_error_log_out eval
qr/proxy_on_configure 1
proxy_on_configure 2
proxy_on_configure 3
proxy_on_done [12]
proxy_on_done [12]
proxy_on_configure [12]
proxy_on_configure [12]
/
--- shutdown_error_log
proxy_on_done 3



=== TEST 6: free plugin after all ctxs are gone
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local ref
        do
            local plugin = wasm.load("plugin", "t/testdata/plugin_lifecycle/main.go.wasm")
            for i = 1, 2 do
                local ctx = assert(wasm.on_configure(plugin, '{"body":512}'))
            end
            ref = assert(wasm.on_configure(plugin, '{"body":512}'))

            collectgarbage()
        end
    }
}
--- grep_error_log eval
qr/proxy_on_(configure|done) \d+/
--- grep_error_log_out eval
qr/proxy_on_configure 1
proxy_on_configure 2
proxy_on_configure 3
proxy_on_done [12]
proxy_on_done [12]
/
--- shutdown_error_log
proxy_on_done 3
unloaded wasm plugin



=== TEST 7: plugin ctx id counts separately
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        for i = 1, 2 do
            do
                local plugin = wasm.load("plugin", "t/testdata/plugin_lifecycle/main.go.wasm")
                for j = 1, 2 do
                    local ctx = assert(wasm.on_configure(plugin, '{"body":512}'))
                end
            end
            collectgarbage()
        end
    }
}
--- grep_error_log eval
qr/(proxy_on_(configure|done) \d+|unloaded wasm plugin)/
--- grep_error_log_out eval
qr/proxy_on_configure 1
proxy_on_configure 2
proxy_on_done [12]
proxy_on_done [12]
unloaded wasm plugin
proxy_on_configure 1
proxy_on_configure 2
proxy_on_done [12]
proxy_on_done [12]
unloaded wasm plugin
/



=== TEST 8: delete ctx even failed to done
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local ctx
        do
            local plugin = wasm.load("plugin", "t/testdata/plugin_lifecycle/main.go.wasm")
            ctx = assert(wasm.on_configure(plugin, 'failed in proxy_on_done'))
            collectgarbage()
        end
    }
}
--- shutdown_error_log
failed to mark context 1 as done



=== TEST 9: empty configure
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = wasm.load("plugin", "t/testdata/plugin_lifecycle/main.go.wasm")
        local _, err = wasm.on_configure(plugin, '')
        ngx.say(err)
    }
}
--- response_body
bad conf
