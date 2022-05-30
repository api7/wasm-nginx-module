# Copyright 2022 Shenzhen ZhiLiu Technology Co., Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
OPENRESTY_PREFIX ?= /usr/local/openresty
INSTALL ?= install
RUST_DIR = $(wildcard t/testdata/rust/*)

.PHONY: install
install:
	$(INSTALL) -m 664 lib/resty/*.lua $(OPENRESTY_PREFIX)/lualib/resty/
	cp -r ./wasmtime-c-api $(OPENRESTY_PREFIX)/
	if [ -d ./wasmedge ]; then cp -r ./wasmedge $(OPENRESTY_PREFIX)/; fi

.PHONY: build.go.testdata
build.go.testdata:
	@find ./t/testdata -type f -name "*.go" | xargs -Ip tinygo build -o p.wasm -scheduler=none -target=wasi p

.PHONY: build.testdata
build.testdata:
	@find ./t/testdata -type f -name "main.go" | grep ${name} | xargs -Ip tinygo build -o p.wasm -scheduler=none -target=wasi p

.PHONY: build.assemblyscript.testdata
build.assemblyscript.testdata:
	@cd ./t/testdata/assemblyscript && npm install && npm run asbuild

.PHONY: build.rust.testdata
build.rust.testdata:
	$(foreach DIR, $(RUST_DIR), \
		cd $(DIR) && \
		cargo build --target=wasm32-wasi && \
		cd ../../../..; \
	)

.PHONY: build.all.testdata
build.all.testdata: build.go.testdata build.assemblyscript.testdata build.rust.testdata

.PHONY: utils
utils:
ifeq ("$(wildcard utils/reindex)", "")
	wget -P utils https://raw.githubusercontent.com/iresty/openresty-devel-utils/master/reindex
	chmod a+x utils/reindex
endif


.PHONY: lint
lint: utils
	luacheck .
	./utils/check-test-code-style.sh
	git ls-files | xargs eclint check
