package t::WASM;

use Test::Nginx::Socket::Lua;
use Test::Nginx::Socket::Lua::Stream -Base;
use Cwd qw(cwd);

log_level('info');
no_long_string();
no_shuffle();
master_on();
worker_connections(1024);


$ENV{TEST_NGINX_HTML_DIR} ||= html_dir();


add_block_preprocessor(sub {
    my ($block) = @_;

    if (!$block->request) {
        $block->set_value("request", "GET /t");
    }

    if (!$block->no_error_log && !$block->error_log) {
        $block->set_value("no_error_log", "[error]\n[alert]");
    }

    if (!$block->shutdown_error_log) {
        # ensure the Leak log is checked
        $block->set_value("shutdown_error_log", "");
    }
    my $pat = $block->no_shutdown_error_log // '';
    $block->set_value("no_shutdown_error_log", "LeakSanitizer: detected memory leaks\n" . $pat);

    my $http_config = $block->http_config // '';
    $http_config .= <<_EOC_;
    lua_package_path "lib/?.lua;;";
    lua_ssl_trusted_certificate ../../certs/test.crt;
    wasm_vm wasmtime;

    server {
        listen 1980;
        listen 1981 ssl;
        ssl_certificate ../../certs/test.crt;
        ssl_certificate_key ../../certs/test.key;

        location /sleep {
            content_by_lua_block {
                ngx.sleep(1)
            }
        }

        location /repeated_headers {
            content_by_lua_block {
                ngx.header["foo"] = {"bar", "baz"}
            }
        }

        location / {
            content_by_lua_block {
                local cjson = require("cjson")
                ngx.log(ngx.WARN, "hit with [", ngx.var.request_method, " ",
                        ngx.var.scheme, "://", ngx.var.host, ngx.var.request_uri, "]")
                local hdrs = ngx.req.get_headers()
                hdrs["user-agent"] = nil
                local res = {}
                for k, v in pairs(hdrs) do
                    table.insert(res, {k, v})
                end
                table.sort(res, function (a, b)
                    return a[1] < b[1]
                end)
                ngx.log(ngx.WARN, "hit with headers ", cjson.encode(res))

                if ngx.var.request_method == "POST" then
                    ngx.req.read_body()
                    local body = ngx.req.get_body_data()
                    ngx.log(ngx.WARN, "hit with body ", body)
                end

                if ngx.var.arg_body then
                    ngx.print(ngx.var.arg_body)
                end
            }
        }
    }
_EOC_

    $block->set_value("http_config", $http_config);

    my $main_config = $block->main_config // '';
    $main_config .= <<_EOC_;
env WASMTIME_BACKTRACE_DETAILS=1;
_EOC_
    $block->set_value("main_config", $main_config);
});

1;
