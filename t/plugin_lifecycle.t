use t::WASM 'no_plan';

run_tests();

__DATA__

=== TEST 1: load
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.wasm")
        assert(wasm.load("t/testdata/plugin_lifecycle/main.go.wasm", '{"body":512}'))
    }
}
--- grep_error_log eval
qr/writeFile failed/
--- grep_error_log_out
writeFile failed
