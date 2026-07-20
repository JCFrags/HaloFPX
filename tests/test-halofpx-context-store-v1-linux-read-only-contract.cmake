if(NOT DEFINED SOURCE_ROOT)
    message(FATAL_ERROR "SOURCE_ROOT is required")
endif()

set(server_cmake_path "${SOURCE_ROOT}/tools/server/CMakeLists.txt")
set(header_path "${SOURCE_ROOT}/tools/server/halofpx-context-store-v1-linux-read-only.h")
set(source_path "${SOURCE_ROOT}/tools/server/halofpx-context-store-v1-linux-read-only.cpp")
file(READ "${server_cmake_path}" server_cmake)
file(READ "${header_path}" header_text)
file(READ "${source_path}" source_text)

string(FIND "${server_cmake}"
    "if (CMAKE_SYSTEM_NAME STREQUAL \"Linux\")\n    add_library(halofpx-context-store-v1-linux-read-only STATIC EXCLUDE_FROM_ALL"
    linux_excluded_pos)
if(linux_excluded_pos EQUAL -1)
    message(FATAL_ERROR "filesystem reader must remain an exact Linux-only STATIC EXCLUDE_FROM_ALL target")
endif()
string(REGEX MATCHALL "halofpx-context-store-v1-linux-read-only" target_mentions "${server_cmake}")
list(LENGTH target_mentions target_mention_count)
if(NOT target_mention_count EQUAL 5)
    message(FATAL_ERROR "filesystem reader escaped its isolated target graph: ${target_mention_count} mentions")
endif()
string(FIND "${server_cmake}"
    "target_link_libraries(halofpx-context-store-v1-linux-read-only PUBLIC\n        halofpx-context-store-v1-read-only)"
    dependency_pos)
if(dependency_pos EQUAL -1)
    message(FATAL_ERROR "filesystem reader must terminate at the excluded memory-only provider")
endif()

string(FIND "${source_text}" "#if !defined(__linux__)" linux_guard_pos)
if(linux_guard_pos EQUAL -1)
    message(FATAL_ERROR "filesystem reader lacks a compile-time Linux guard")
endif()

# Audit executable text after comments are removed. The adapter may perform
# bounded direct reads, but owns no discovery, writer, server, or live restore.
string(REGEX REPLACE "//[^\n]*" "" header_code "${header_text}")
string(REGEX REPLACE "//[^\n]*" "" source_code "${source_text}")
set(code "${header_code}\n${source_code}")
foreach(forbidden IN ITEMS
        "readdir" "getdents" "glob(" "std::filesystem" "<filesystem>" "<fstream>"
        "opendir" "llama-server" "server-context" "llama_context" "ggml_" "gguf"
        "CachyLLama" "llama-ai" "cachyllama" "llama_ai" "publish_generation"
        "restore_context" "decode_state")
    string(FIND "${code}" "${forbidden}" forbidden_pos)
    if(NOT forbidden_pos EQUAL -1)
        message(FATAL_ERROR "forbidden discovery/product/live-state/writer/donor term: ${forbidden}")
    endif()
endforeach()

foreach(required IN ITEMS
        "SYS_openat2"
        "RESOLVE_BENEATH"
        "RESOLVE_NO_SYMLINKS"
        "RESOLVE_NO_MAGICLINKS"
        "RESOLVE_NO_XDEV"
        "O_NOFOLLOW"
        "digest_name(\"m-\""
        "digest_name(\"o-\"")
    string(FIND "${source_code}" "${required}" required_pos)
    if(required_pos EQUAL -1)
        message(FATAL_ERROR "missing required bounded filesystem control: ${required}")
    endif()
endforeach()
string(FIND "${source_code}" "context_store_capabilities capabilities() const noexcept override { return {}; }" caps_pos)
if(caps_pos EQUAL -1)
    message(FATAL_ERROR "filesystem reader capabilities are not default closed")
endif()
string(FIND "${source_code}" "return context_store_publish_status::disabled;" publish_pos)
if(publish_pos EQUAL -1)
    message(FATAL_ERROR "filesystem reader publish path is not unconditionally disabled")
endif()

message(STATUS "HaloFPX excluded Linux full-v1 filesystem reader static contract passed")
