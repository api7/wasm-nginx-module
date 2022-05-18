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
use proxy_wasm::traits::*;
use proxy_wasm::types::*;
use std::time::Duration;

proxy_wasm::main! {{
    proxy_wasm::set_log_level(LogLevel::Trace);
    proxy_wasm::set_http_context(|_, _| -> Box<dyn HttpContext> { Box::new(HttpCall) });
}}

struct HttpCall;

impl HttpContext for HttpCall {
    fn on_http_request_headers(&mut self, _: usize, _: bool) -> Action {
        self.dispatch_http_call(
            "127.0.0.1:1980",
            vec![
                (":method", "GET"),
                (":path", "/bytes/1"),
                (":authority", "httpbin.org"),
            ],
            None,
            vec![],
            Duration::from_secs(5),
        )
        .unwrap();
        Action::Pause
    }
}

impl Context for HttpCall {
    fn on_http_call_response(&mut self, _: u32, _: usize, _: usize, _: usize) {
        let s = self.get_http_call_response_header("Content-Type").unwrap();
        let st = self.get_http_call_response_header(":status").unwrap();
        self.send_http_response(
            403,
            vec![("Resp-Content-Type", &s), ("Resp-Status", &st)],
            Some(b"Access forbidden.\n"),
        );
    }
}
