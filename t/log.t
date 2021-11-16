use t::WASM 'no_plan';

run_tests();

__DATA__

=== TEST 1: sanity
--- log_level: debug
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local p = wasm.load("plugin", "t/testdata/log/main.go.wasm")
        wasm.on_configure(p, "blah")
    }
}
--- grep_error_log eval
qr/\[\w+\] \d+#\d+: ouch, something is wrong/
--- grep_error_log_out eval
qr/\[emerg\] \d+#\d+: ouch, something is wrong
\[error\] \d+#\d+: ouch, something is wrong
\[warn\] \d+#\d+: ouch, something is wrong
\[info\] \d+#\d+: ouch, something is wrong
\[debug\] \d+#\d+: ouch, something is wrong
\[debug\] \d+#\d+: ouch, something is wrong
/
--- no_error_log
[alert]



=== TEST 2: log in http request
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/log/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, '{"body":512}'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- grep_error_log eval
qr/(create|run) http ctx \d+, client/
--- grep_error_log_out
create http ctx 2, client
run http ctx 2, client
--- no_error_log
[alert]
