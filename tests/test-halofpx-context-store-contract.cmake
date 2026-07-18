if (NOT DEFINED HALOFPX_SOURCE_DIR OR NOT IS_DIRECTORY "${HALOFPX_SOURCE_DIR}")
    message(FATAL_ERROR "HALOFPX_SOURCE_DIR must name the source tree")
endif()

set(header_path "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store.h")
set(source_path "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store.cpp")
file(READ "${header_path}" header)
file(READ "${source_path}" source)
set(seam "${header}\n${source}")

foreach(forbidden
        "<filesystem>"
        "<fstream>"
        "llama_state"
        "server_prompt_cache"
        "cache_disk"
        "CachyLLama"
        "llama-ai"
        "fopen("
        "CreateFile"
        "LOG_")
    string(FIND "${seam}" "${forbidden}" forbidden_position)
    if (NOT forbidden_position EQUAL -1)
        message(FATAL_ERROR "L03a inert seam contains forbidden dependency: ${forbidden}")
    endif()
endforeach()

file(GLOB_RECURSE runtime_paths
    "${HALOFPX_SOURCE_DIR}/common/*.h"
    "${HALOFPX_SOURCE_DIR}/common/*.cpp"
    "${HALOFPX_SOURCE_DIR}/tools/server/*.h"
    "${HALOFPX_SOURCE_DIR}/tools/server/*.cpp")

foreach(runtime_path IN LISTS runtime_paths)
    if ("${runtime_path}" STREQUAL "${header_path}" OR "${runtime_path}" STREQUAL "${source_path}")
        continue()
    endif()
    file(READ "${runtime_path}" runtime_source)
    foreach(forbidden_hook
            "make_disabled_context_store_provider"
            "context_store_provider")
        string(FIND "${runtime_source}" "${forbidden_hook}" hook_position)
        if (NOT hook_position EQUAL -1)
            message(FATAL_ERROR "L03a seam unexpectedly entered production source ${runtime_path}: ${forbidden_hook}")
        endif()
    endforeach()
endforeach()

message(STATUS "HaloFPX L03a provider seam is compiled but has no I/O or runtime hook")
