# Proxy-Wasm ABI specification implemented in the wasm-nginx-module

As this module is still WIP, this list will be changed in the future version.
Due to the behavior of Nginx and the different Proxy-Wasm SDK, we doesn't
strictly implement the Proxy-Wasm ABI. In this case, we will add a `Note: `
in the corresponding section.

# Functions implemented in the Wasm module

All functions implemented in the Wasm module, other than the integration and memory management
functions, include context identifier (`context_id`) as the first parameter, which should be used to
distinguish between different contexts.


## Integration

### `_start`

* params:
  - none
* returns:
  - none

Start function which is called when the module is loaded and initialized. This can be used by SDKs
to setup and/or initialize state, but no proxy_ functions can be used at that point yet.


## Memory management

### `proxy_on_memory_allocate`

* params:
  - `i32 (size_t) memory_size`
* returns:
  - `i32 (void*) allocated_ptr`

Allocates memory using in-VM memory allocator and returns it to the host.


## Module lifecycle

### `proxy_on_context_create`

* params:
  - `i32 (uint32_t) context_id`
  - `i32 (uint32_t) parent_context_id`
* returns:
  - none

Called when the host environment creates a new root context (if `parent_context_id` is `0`) or a new
per-stream context.


### `proxy_on_done`

* params:
  - `i32 (uint32_t) context_id`
* returns:
  - `i32 (bool) is_done`

Called when the host environment is done processing the context (`context_id`). Return value
indicates when the Wasm VM is done with the processing as well.


### `proxy_on_delete`

* params:
  - `i32 (uint32_t) context_id`
* returns:
  - none

Called when the host environment removes the context (`context_id`). This is used to signal that VM
should stop tracking that `context_id` and remove all associated state.


## Configuration

### `proxy_on_configure`

* params:
  - `i32 (uint32_t) root_context_id`
  - `i32 (size_t) plugin_configuration_size`
* returns:
  - `i32 (bool) success`

Called when the host environment starts the plugin. Its configuration (`plugin_configuration_size`)
might be retrieved using `proxy_get_buffer`.


# Functions implemented in the host environment

All functions implemented in the host environment return `proxy_result_t`, which indicates the
status of the call (successful, invalid memory access, etc.), and the return values are written into
memory pointers passed in as arguments (indicated by the `return_` prefix in the specification).


## Logging

### `proxy_log`

* params:
  - `i32 (proxy_log_level_t) log_level`
  - `i32 (const char*) message_data`
  - `i32 (size_t) message_size`
* returns:
  - `i32 (proxy_result_t) call_result`

Log message (`message_data`, `message_size`) at the given `log_level`.


## Buffers, maps, and properties

### `proxy_get_buffer`

* params:
  - `i32 (proxy_buffer_type_t) buffer_type`
  - `i32 (offset_t) offset`
  - `i32 (size_t) max_size`
  - `i32 (const char**) return_buffer_data`
  - `i32 (size_t*) return_buffer_size`
  - `i32 (uint32_t*) return_flags`
* returns:
  - `i32 (proxy_result_t) call_result`

Get up to max_size bytes from the `buffer_type`, starting from `offset`. Bytes are written into
buffer slice (`return_buffer_data`, `return_buffer_size`), and buffer flags are written into
`return_flags`.

Note: we implement `proxy_get_buffer_bytes` (an older version of `proxy_get_buffer`) instead
because proxy-wasm-go-sdk uses it.

`proxy_get_buffer_bytes`

* params:
  - `i32 (proxy_buffer_type_t) buffer_type`
  - `i32 (offset_t) offset`
  - `i32 (size_t) max_size`
  - `i32 (const char**) return_buffer_data`
  - `i32 (size_t*) return_buffer_size`
* returns:
  - `i32 (proxy_result_t) call_result`
