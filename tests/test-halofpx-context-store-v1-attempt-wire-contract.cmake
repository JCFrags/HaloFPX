file(READ "${SOURCE_ROOT}/tools/server/CMakeLists.txt" server_cmake)
file(READ "${SOURCE_ROOT}/tools/server/halofpx-context-store-v1-attempt-wire.cpp" source)
file(READ "${SOURCE_ROOT}/tools/server/halofpx-context-store-v1-attempt-wire.h" header)

foreach(required
    "add_library(halofpx-context-store-v1-attempt-wire STATIC EXCLUDE_FROM_ALL"
    "halofpx.attempt-pending-key.v1"
    "halofpx.attempt-pending-auth.v1"
    "halofpx.attempt-terminal-key.v1"
    "halofpx.attempt-terminal-auth.v1"
    "context_store_v1_attempt_pending_verify"
    "context_store_v1_attempt_terminal_verify")
    string(FIND "${server_cmake}${source}${header}" "${required}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "missing L08h-a isolation token: ${required}")
    endif()
endforeach()

foreach(forbidden
    "server-context.cpp"
    "llama-server"
    "llama_context"
    "context_store_restore_transformer_state_v1"
    "openat("
    "renameat2("
    "unlinkat("
    "fsync(")
    string(FIND "${source}${header}" "${forbidden}" found)
    if(NOT found EQUAL -1)
        message(FATAL_ERROR "forbidden L08h-a product/filesystem token: ${forbidden}")
    endif()
endforeach()

