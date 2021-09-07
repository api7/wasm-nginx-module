.PHONY: build.testdata
build.testdata:
	@find ./t/testdata -type f -name "main.go" | grep ${name} | xargs -Ip tinygo build -o p.wasm -scheduler=none -target=wasi p

.PHONY: build.all.testdata
build.all.testdata:
	@find ./t/testdata -type f -name "main.go" | xargs -Ip tinygo build -o p.wasm -scheduler=none -target=wasi p
