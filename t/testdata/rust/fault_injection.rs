use log::*;
use rand::Rng;
use serde::{Deserialize, Serialize};
use proxy_wasm::traits::*;
use proxy_wasm::types::*;

#[no_mangle]
pub fn _start() {
    proxy_wasm::set_log_level(LogLevel::Info);
    proxy_wasm::set_root_context(|_| -> Box<dyn RootContext> {
        Box::new(FaultInjectionRoot {
            conf: Conf{
                body: String::new(),
                http_status: 200,
                percentage: 100,
            },
        })
    });
}

#[derive(Serialize, Deserialize, Clone)]
struct Conf {
    #[serde(default)]
    body: String,
    http_status: u32,
    #[serde(default = "default_percentage")]
    percentage: u8,
}

fn default_percentage() -> u8 {
    100
}

struct FaultInjectionRoot {
    conf: Conf,
}

impl Context for FaultInjectionRoot {}

impl RootContext for FaultInjectionRoot {
    fn on_configure(&mut self, _: usize) -> bool {
        if let Some(config_bytes) = self.get_configuration() {
            let conf : Conf = serde_json::from_str(&*String::from_utf8(config_bytes).unwrap()
                                                   ).unwrap();
            self.conf = conf;
        }
        true
    }

    fn get_type(&self) -> Option<ContextType> {
        Some(ContextType::HttpContext)
    }

    fn create_http_context(&self, _context_id: u32) -> Option<Box<dyn HttpContext>> {
        Some(Box::new(FaultInjection {
            conf: self.conf.clone(),
        }))
    }
}

struct FaultInjection {
    conf: Conf,
}

impl Context for FaultInjection {}

impl HttpContext for FaultInjection {
    fn on_http_request_headers(&mut self, _: usize) -> Action {
        let mut rng = rand::thread_rng();

        let rd = rng.gen_range(0..100);
        info!("random {}, percentage {}", rd, self.conf.percentage);

        if self.conf.percentage <= rd {
            return Action::Continue
        }
        self.send_http_response(
            self.conf.http_status,
            vec![],
            Some(self.conf.body.as_bytes()),
        );
        Action::Pause
    }
}
