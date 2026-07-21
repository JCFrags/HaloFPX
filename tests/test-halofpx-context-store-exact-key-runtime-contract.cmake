if (NOT DEFINED HALOFPX_SOURCE_DIR OR NOT IS_DIRECTORY "${HALOFPX_SOURCE_DIR}")
    message(FATAL_ERROR "HALOFPX_SOURCE_DIR must name the source tree")
endif()

file(READ "${HALOFPX_SOURCE_DIR}/tools/server/CMakeLists.txt" cmake_text)
foreach(required
        "option(HALOFPX_CONTEXT_STORE_EXACT_KEY_CANARY"
        "HALOFPX_CONTEXT_STORE_EXACT_KEY_CANARY requires all four prior full-v1 context-store gates"
        "HALOFPX_CONTEXT_STORE_EXACT_KEY_CANARY is supported only on Linux")
    string(FIND "${cmake_text}" "${required}" position)
    if (position EQUAL -1)
        message(FATAL_ERROR "missing exact-key build contract: ${required}")
    endif()
endforeach()

file(READ "${HALOFPX_SOURCE_DIR}/tools/server/server-context.cpp" runtime)
foreach(required
        "full-v1-exact-key-canary"
        "context_store_resolve_exact_session_v1"
        "restore_selected"
        "halofpx_exact_key_publish_at_prompt_boundary"
        "miss_not_found")
    string(FIND "${runtime}" "${required}" position)
    if (position EQUAL -1)
        message(FATAL_ERROR "missing exact-key runtime boundary: ${required}")
    endif()
endforeach()

file(READ "${HALOFPX_SOURCE_DIR}/tools/server/server-task.cpp" result_source)
foreach(forbidden "halofpx_exact_key" "session_id" "scope_namespace")
    string(FIND "${result_source}" "${forbidden}" position)
    if (NOT position EQUAL -1)
        message(FATAL_ERROR "opaque exact-key carrier entered response serialization: ${forbidden}")
    endif()
endforeach()

message(STATUS "HaloFPX exact-key runtime contract passed")
