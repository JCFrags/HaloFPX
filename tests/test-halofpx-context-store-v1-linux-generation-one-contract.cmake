file(READ "${SOURCE_ROOT}/tools/server/CMakeLists.txt" cmake)
file(READ "${SOURCE_ROOT}/tools/server/halofpx-context-store-v1-linux-generation-one.cpp" source)
file(READ "${SOURCE_ROOT}/tools/server/halofpx-context-store-v1-linux-generation-one.h" header)

foreach(required
    "halofpx-context-store-v1-linux-generation-one STATIC EXCLUDE_FROM_ALL"
    "F_OFD_SETLK"
    "pending.v1"
    "terminal.v1"
    "anchor.v1"
    "RENAME_NOREPLACE"
    "context_store_v1_attempt_pending_verify"
    "context_store_v1_attempt_terminal_verify"
    "make_context_store_v1_linux_read_only_provider"
    "make_context_store_v1_linux_snapshot_materializer"
    "distinct_non_nested_roots")
    string(FIND "${cmake}${source}${header}" "${required}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "missing L08h-b contract token: ${required}")
    endif()
endforeach()
foreach(forbidden
    "server-context.cpp"
    "llama-server"
    "llama_context"
    "context_store_restore_transformer_state_v1")
    string(FIND "${source}${header}" "${forbidden}" found)
    if(NOT found EQUAL -1)
        message(FATAL_ERROR "forbidden L08h-b product/live-state edge: ${forbidden}")
    endif()
endforeach()
