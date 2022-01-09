#ifndef PROXY_WASM_TYPES_H
#define PROXY_WASM_TYPES_H


#include <ngx_core.h>


typedef struct {
    ngx_str_t       key;
    ngx_str_t       value;
} proxy_wasm_table_elt_t;


typedef enum {
    PROXY_RESULT_OK = 0,
    // The result could not be found, e.g. a provided key did not appear in a
    // table.
    PROXY_RESULT_NOT_FOUND = 1,
    // An argument was bad, e.g. did not not conform to the required range.
    PROXY_RESULT_BAD_ARGUMENT = 2,
    // A protobuf could not be serialized.
    PROXY_RESULT_SERIALIZATIONF_AILURE = 3,
    // A protobuf could not be parsed.
    PROXY_RESULT_PARSE_FAILURE = 4,
    // A provided expression (e.g. "foo.bar") was illegal or unrecognized.
    PROXY_RESULT_BAD_EXPRESSION = 5,
    // A provided memory range was not legal.
    PROXY_RESULT_INVALID_MEMORY_ACCESS = 6,
    // Data was requested from an empty container.
    PROXY_RESULT_EMPTY = 7,
    // The provided CAS did not match that of the stored data.
    PROXY_RESULT_CAS_MISMATCH = 8,
    // Returned result was unexpected, e.g. of the incorrect size.
    PROXY_RESULT_RESULT_MISMATCH = 9,
    // Internal failure: trying check logs of the surrounding system.
    PROXY_RESULT_INTERNAL_FAILURE = 10,
    // The connection/stream/pipe was broken/closed unexpectedly.
    PROXY_RESULT_BROKEN_CONNECTION = 11,
    // Feature not implemented.
    PROXY_RESULT_UNIMPLEMENTED = 12,
} proxy_result_t;


typedef enum {
    PROXY_LOG_TRACE,
    PROXY_LOG_DEBUG,
    PROXY_LOG_INFO,
    PROXY_LOG_WARN,
    PROXY_LOG_ERROR,
    PROXY_LOG_CRITICAL,
} proxy_log_level_t;


typedef enum {
    PROXY_BUFFER_TYPE_HTTP_REQUEST_BODY,
    PROXY_BUFFER_TYPE_HTTP_RESPONSE_BODY,
    PROXY_BUFFER_TYPE_DOWNSTREAM_DATA,
    PROXY_BUFFER_TYPE_UPSTREAM_DATA,
    PROXY_BUFFER_TYPE_HTTP_CALL_RESPONSE_BODY,
    PROXY_BUFFER_TYPE_GRPC_RECEIVE_BUFFER,
    PROXY_BUFFER_TYPE_VM_CONFIGURATION,
    PROXY_BUFFER_TYPE_PLUGIN_CONFIGURATION,
    PROXY_BUFFER_TYPE_CALL_DATA,
} proxy_buffer_type_t;


enum {
    PROXY_WASM_HEADER_PATH = 1,
    PROXY_WASM_HEADER_METHOD,
    PROXY_WASM_HEADER_SCHEME,
    PROXY_WASM_HEADER_AUTHORITY,
};


#endif // PROXY_WASM_TYPES_H
