if (NOT DEFINED HALOFPX_SOURCE_DIR OR NOT IS_DIRECTORY "${HALOFPX_SOURCE_DIR}")
    message(FATAL_ERROR "HALOFPX_SOURCE_DIR must name the source tree")
endif()

set(auth_header "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-auth.h")
set(auth_source "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-auth.cpp")
file(READ "${auth_header}" header)
file(READ "${auth_source}" source)
set(verifier "${header}\n${source}")

file(READ "${HALOFPX_SOURCE_DIR}/tools/server/CMakeLists.txt" server_cmake)
string(FIND "${server_cmake}" "add_library(halofpx-context-store-auth STATIC EXCLUDE_FROM_ALL" excluded_target)
if (excluded_target EQUAL -1)
    message(FATAL_ERROR "L04b verifier target must remain EXCLUDE_FROM_ALL")
endif()
string(FIND "${server_cmake}" "# server-context containing the core server logic" runtime_marker)
if (runtime_marker EQUAL -1)
    message(FATAL_ERROR "Cannot locate production server-context CMake section")
endif()
string(SUBSTRING "${server_cmake}" ${runtime_marker} -1 runtime_cmake)
string(FIND "${runtime_cmake}" "halofpx-context-store-auth" runtime_link)
if (NOT runtime_link EQUAL -1)
    message(FATAL_ERROR "L04b verifier entered the production CMake graph")
endif()

foreach(forbidden
        "<filesystem>" "<fstream>" "fopen(" "CreateFile" "BCrypt" "OpenSSL"
        "context_store_provider" "context_store_candidate" "llama_state"
        "server_prompt_cache" "memcmp(" "CachyLLama" "llama-ai" "LOG_")
    string(FIND "${verifier}" "${forbidden}" position)
    if (NOT position EQUAL -1)
        message(FATAL_ERROR "L04b offline verifier contains forbidden dependency: ${forbidden}")
    endif()
endforeach()

string(REGEX MATCHALL "#[ \t]*include[ \t]*\"[^\"]+\"" header_local_includes "${header}")
if (NOT "${header_local_includes}" STREQUAL "#include \"halofpx-context-store-format.h\"")
    message(FATAL_ERROR "L04b header has non-allowlisted local includes: ${header_local_includes}")
endif()
string(REGEX MATCHALL "#[ \t]*include[ \t]*\"[^\"]+\"" source_local_includes "${source}")
set(expected_source_includes
    "#include \"halofpx-context-store-auth.h\";#include \"sha256/sha256.h\"")
if (NOT "${source_local_includes}" STREQUAL "${expected_source_includes}")
    message(FATAL_ERROR "L04b source has non-allowlisted local includes: ${source_local_includes}")
endif()

foreach(source_record
        "examples/gguf-hash/deps/sha256/sha256.c|EA70A42189B4C798657CB2FB334AD6AFB456245B13D3D36D0FF9FD4F9D9E8F62"
        "examples/gguf-hash/deps/sha256/sha256.h|697B3138AA7590D4C86F332B80E241990674C8344D47171A259B16353309B056"
        "examples/gguf-hash/deps/rotate-bits/rotate-bits.h|633D6F97ABABF28A562B820FC49C6A8788EA56349568CBE908C051AC032F0685")
    string(REPLACE "|" ";" fields "${source_record}")
    list(GET fields 0 path)
    list(GET fields 1 expected_sha256)
    file(SHA256 "${HALOFPX_SOURCE_DIR}/${path}" actual_sha256)
    string(TOUPPER "${actual_sha256}" actual_sha256)
    if (NOT actual_sha256 STREQUAL expected_sha256)
        message(FATAL_ERROR "Inherited public-domain SHA source drift: ${path}")
    endif()
endforeach()

file(GLOB_RECURSE runtime_paths
    "${HALOFPX_SOURCE_DIR}/common/*.h"
    "${HALOFPX_SOURCE_DIR}/common/*.cpp"
    "${HALOFPX_SOURCE_DIR}/tools/server/*.h"
    "${HALOFPX_SOURCE_DIR}/tools/server/*.cpp")
foreach(runtime_path IN LISTS runtime_paths)
    set(authority_header "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-authority.h")
    set(authority_source "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-authority.cpp")
    if (runtime_path STREQUAL auth_header OR runtime_path STREQUAL auth_source OR
        runtime_path STREQUAL authority_header OR runtime_path STREQUAL authority_source)
        continue()
    endif()
    file(READ "${runtime_path}" runtime_source)
    string(FIND "${runtime_source}" "context_store_verify_manifest_v1" hook_position)
    if (NOT hook_position EQUAL -1)
        message(FATAL_ERROR "L04b verifier unexpectedly entered production source: ${runtime_path}")
    endif()
endforeach()

message(STATUS "HaloFPX L04b verifier is offline, default-excluded, and source-locked")
