OPENRESTY_PREFIX ?= /usr/local/openresty
INSTALL ?= install

.PHONY: install
install:
	$(INSTALL) -m 664 lib/resty/*.lua $(OPENRESTY_PREFIX)/lualib/resty/
	cp -r ./wasmtime-c-api $(OPENRESTY_PREFIX)/

.PHONY: build.go.testdata
build.go.testdata:
	@find ./t/testdata -type f -name "main.go" | xargs -Ip tinygo build -o p.wasm -scheduler=none -target=wasi p

.PHONY: build.all.testdata
build.all.testdata: build.go.testdata
	@cd ./t/testdata/assemblyscript && npm install && npm run asbuild
	@cd ./t/testdata/rust &&  cargo build --target=wasm32-wasi
