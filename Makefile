OPENRESTY_PREFIX ?= /usr/local/openresty
INSTALL ?= install

.PHONY: install
install:
	$(INSTALL) -m 664 lib/resty/*.lua $(OPENRESTY_PREFIX)/lualib/resty/
	cp -r ./wasmtime-c-api $(OPENRESTY_PREFIX)/

.PHONY: build.testdata
build.testdata:
	@find ./t/testdata -type f -name "main.go" | grep ${name} | xargs -Ip tinygo build -o p.wasm -scheduler=none -target=wasi p

.PHONY: build.all.testdata
build.all.testdata:
	@find ./t/testdata -type f -name "main.go" | xargs -Ip tinygo build -o p.wasm -scheduler=none -target=wasi p
