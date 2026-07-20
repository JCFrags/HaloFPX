if (NOT DEFINED HALOFPX_SOURCE_DIR OR NOT IS_DIRECTORY "${HALOFPX_SOURCE_DIR}")
    message(FATAL_ERROR "HALOFPX_SOURCE_DIR must name the source tree")
endif()

set(object_header "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-object.h")
set(object_source "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-object.cpp")
file(READ "${object_header}" header)
file(READ "${object_source}" source)
set(verifier "${header}\n${source}")

file(READ "${HALOFPX_SOURCE_DIR}/tools/server/CMakeLists.txt" server_cmake)
foreach(excluded_target
        "add_library(halofpx-context-store-sha256 STATIC EXCLUDE_FROM_ALL"
        "add_library(halofpx-context-store-object STATIC EXCLUDE_FROM_ALL")
    string(FIND "${server_cmake}" "${excluded_target}" position)
    if (position EQUAL -1)
        message(FATAL_ERROR "L04c offline target is not EXCLUDE_FROM_ALL: ${excluded_target}")
    endif()
endforeach()
string(FIND "${server_cmake}" "# server-context containing the core server logic" runtime_marker)
if (runtime_marker EQUAL -1)
    message(FATAL_ERROR "Cannot locate production server-context CMake section")
endif()
string(SUBSTRING "${server_cmake}" ${runtime_marker} -1 runtime_cmake)
foreach(forbidden_target halofpx-context-store-object halofpx-context-store-sha256)
    string(FIND "${runtime_cmake}" "${forbidden_target}" runtime_link)
    if (NOT runtime_link EQUAL -1)
        message(FATAL_ERROR "L04c dependency entered the production CMake graph: ${forbidden_target}")
    endif()
endforeach()

foreach(forbidden
        "<filesystem>" "<fstream>" "<vector>" "fopen(" "CreateFile" "malloc(" "new "
        "context_store_provider" "context_store_candidate" "llama_state"
        "server_prompt_cache" "OpenSSL" "BCrypt" "CachyLLama" "llama-ai" "LOG_")
    string(FIND "${verifier}" "${forbidden}" position)
    if (NOT position EQUAL -1)
        message(FATAL_ERROR "L04c object verifier contains forbidden dependency: ${forbidden}")
    endif()
endforeach()

string(REGEX MATCHALL "#[ \t]*include[ \t]*\"[^\"]+\"" header_local_includes "${header}")
if (NOT "${header_local_includes}" STREQUAL "#include \"halofpx-context-store-auth.h\"")
    message(FATAL_ERROR "L04c header has non-allowlisted local includes: ${header_local_includes}")
endif()
string(REGEX MATCHALL "#[ \t]*include[ \t]*\"[^\"]+\"" source_local_includes "${source}")
if (NOT "${source_local_includes}" STREQUAL "#include \"halofpx-context-store-object.h\"")
    message(FATAL_ERROR "L04c source has non-allowlisted local includes: ${source_local_includes}")
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
    set(v1_read_only_source "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-v1-read-only.cpp")
    if (runtime_path STREQUAL object_header OR runtime_path STREQUAL object_source OR
        runtime_path STREQUAL v1_read_only_source)
        continue()
    endif()
    file(READ "${runtime_path}" runtime_source)
    string(FIND "${runtime_source}" "context_store_verify_object_frame_v1" hook_position)
    if (NOT hook_position EQUAL -1)
        message(FATAL_ERROR "L04c object verifier unexpectedly entered production source: ${runtime_path}")
    endif()
endforeach()

message(STATUS "HaloFPX L04c object verifier is bounded, offline, and default-excluded")
