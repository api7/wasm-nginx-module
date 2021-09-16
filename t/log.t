use t::WASM 'no_plan';

run_tests();

__DATA__

=== TEST 1: sanity
--- log_level: debug
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local p = wasm.load("t/testdata/log/main.go.wasm")
        wasm.on_configure(p)
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
