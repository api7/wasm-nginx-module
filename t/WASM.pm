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
    wasm_vm wasmtime;

    server {
        listen 1980;
        location / {
            content_by_lua_block {
                ngx.log(ngx.WARN, "hit")
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
