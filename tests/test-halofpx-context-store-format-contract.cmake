if (NOT DEFINED HALOFPX_SOURCE_DIR OR NOT IS_DIRECTORY "${HALOFPX_SOURCE_DIR}")
    message(FATAL_ERROR "HALOFPX_SOURCE_DIR must name the source tree")
endif()

set(format_header "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-format.h")
set(format_source "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-format.cpp")
set(auth_header "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-auth.h")
set(auth_source "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-auth.cpp")
file(READ "${format_header}" header)
file(READ "${format_source}" source)
set(parser "${header}\n${source}")

file(READ "${HALOFPX_SOURCE_DIR}/tools/server/CMakeLists.txt" server_cmake)
string(FIND "${server_cmake}" "add_library(halofpx-context-store-format STATIC EXCLUDE_FROM_ALL" excluded_target_position)
if (excluded_target_position EQUAL -1)
    message(FATAL_ERROR "L04a parser target must remain EXCLUDE_FROM_ALL")
endif()

foreach(forbidden
        "<filesystem>" "<fstream>" "fopen(" "CreateFile" "llama_state"
        "#include \"halofpx-context-store.h\""
        "context_store_provider" "context_store_candidate" "server_prompt_cache"
        "CachyLLama" "llama-ai" "LOG_")
    string(FIND "${parser}" "${forbidden}" forbidden_position)
    if (NOT forbidden_position EQUAL -1)
        message(FATAL_ERROR "L04a offline parser contains forbidden dependency: ${forbidden}")
    endif()
endforeach()

string(REGEX MATCHALL "#[ \t]*include[ \t]*\"[^\"]+\"" header_local_includes "${header}")
if (header_local_includes)
    message(FATAL_ERROR "L04a format header contains local include: ${header_local_includes}")
endif()
string(REGEX MATCHALL "#[ \t]*include[ \t]*\"[^\"]+\"" source_local_includes "${source}")
if (NOT "${source_local_includes}" STREQUAL "#include \"halofpx-context-store-format.h\"")
    message(FATAL_ERROR "L04a parser source has non-allowlisted local includes: ${source_local_includes}")
endif()

file(GLOB_RECURSE runtime_paths
    "${HALOFPX_SOURCE_DIR}/common/*.h"
    "${HALOFPX_SOURCE_DIR}/common/*.cpp"
    "${HALOFPX_SOURCE_DIR}/tools/server/*.h"
    "${HALOFPX_SOURCE_DIR}/tools/server/*.cpp")
foreach(runtime_path IN LISTS runtime_paths)
    set(authority_header "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-authority.h")
    set(authority_source "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-authority.cpp")
    set(bootstrap_material_source "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-bootstrap-material.cpp")
    set(bootstrap_anchor_source "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-bootstrap-anchor.cpp")
    if ("${runtime_path}" STREQUAL "${format_header}" OR
        "${runtime_path}" STREQUAL "${format_source}" OR
        "${runtime_path}" STREQUAL "${auth_header}" OR
        "${runtime_path}" STREQUAL "${auth_source}" OR
        "${runtime_path}" STREQUAL "${authority_header}" OR
        "${runtime_path}" STREQUAL "${authority_source}" OR
        "${runtime_path}" STREQUAL "${bootstrap_material_source}" OR
        "${runtime_path}" STREQUAL "${bootstrap_anchor_source}")
        continue()
    endif()
    file(READ "${runtime_path}" runtime_source)
    string(FIND "${runtime_source}" "context_store_parse_manifest_v1" hook_position)
    if (NOT hook_position EQUAL -1)
        message(FATAL_ERROR "L04a parser unexpectedly entered production source: ${runtime_path}")
    endif()
endforeach()

message(STATUS "HaloFPX L04a parser is memory-only, offline, and not runtime-linked")
