if(NOT DEFINED SOURCE_ROOT)
    message(FATAL_ERROR "SOURCE_ROOT is required")
endif()

set(server_cmake_path "${SOURCE_ROOT}/tools/server/CMakeLists.txt")
set(header_path "${SOURCE_ROOT}/tools/server/halofpx-context-store-v1-linux-publish.h")
set(source_path "${SOURCE_ROOT}/tools/server/halofpx-context-store-v1-linux-publish.cpp")
file(READ "${server_cmake_path}" server_cmake)
file(READ "${header_path}" header_text)
file(READ "${source_path}" source_text)

string(FIND "${server_cmake}"
    "if (CMAKE_SYSTEM_NAME STREQUAL \"Linux\")\n    add_library(halofpx-context-store-v1-linux-read-only STATIC EXCLUDE_FROM_ALL"
    linux_scope_pos)
string(FIND "${server_cmake}"
    "    add_library(halofpx-context-store-v1-linux-publish STATIC EXCLUDE_FROM_ALL"
    excluded_pos)
if(linux_scope_pos EQUAL -1 OR excluded_pos EQUAL -1 OR excluded_pos LESS linux_scope_pos)
    message(FATAL_ERROR "publisher must remain a Linux-scoped STATIC EXCLUDE_FROM_ALL target")
endif()
string(REGEX MATCHALL "halofpx-context-store-v1-linux-publish" target_mentions "${server_cmake}")
list(LENGTH target_mentions target_mention_count)
if(NOT target_mention_count EQUAL 6)
    message(FATAL_ERROR "publisher escaped its isolated target graph: ${target_mention_count} mentions")
endif()
string(FIND "${server_cmake}"
    "target_link_libraries(halofpx-context-store-v1-linux-generation-one PUBLIC\n        halofpx-context-store-v1-linux-publish"
    authority_consumer_pos)
if(authority_consumer_pos EQUAL -1)
    message(FATAL_ERROR "the sole downstream publisher consumer must be the excluded generation-one authority")
endif()
string(FIND "${server_cmake}"
    "target_link_libraries(halofpx-context-store-v1-linux-publish PUBLIC\n        halofpx-context-store-v1-read-only\n        halofpx-context-store-v1-linux-read-only)"
    dependency_pos)
if(dependency_pos EQUAL -1)
    message(FATAL_ERROR "publisher must terminate at the two excluded full-v1 seams")
endif()

string(REGEX REPLACE "//[^\n]*" "" header_code "${header_text}")
string(REGEX REPLACE "//[^\n]*" "" source_code "${source_text}")
set(code "${header_code}\n${source_code}")
foreach(forbidden IN ITEMS
        "readdir" "getdents" "glob(" "opendir" "std::filesystem" "<filesystem>"
        "llama-server" "server-context" "llama_context" "ggml_" "gguf"
        "CachyLLama" "llama-ai" "cachyllama" "llama_ai"
        "publish_generation" "advance_anchor" "write_anchor" "set_anchor"
        "restore" "codec" "enumerat")
    string(FIND "${code}" "${forbidden}" forbidden_pos)
    if(NOT forbidden_pos EQUAL -1)
        message(FATAL_ERROR "forbidden product/discovery/anchor/live-state/donor term: ${forbidden}")
    endif()
endforeach()

foreach(required IN ITEMS
        "#if !defined(__linux__)"
        "SYS_openat2"
        "RESOLVE_BENEATH"
        "RESOLVE_NO_SYMLINKS"
        "RESOLVE_NO_MAGICLINKS"
        "RESOLVE_NO_XDEV"
        "O_NOFOLLOW"
        "F_OFD_SETLK"
        "exact_file_bytes"
        "materialized_non_authoritative"
        "incomplete_or_uncertain_discard_root"
        "::fsync"
        "SYS_renameat2"
        "RENAME_NOREPLACE")
    string(FIND "${source_text}" "${required}" required_pos)
    if(required_pos EQUAL -1)
        message(FATAL_ERROR "missing required immutable publication control: ${required}")
    endif()
endforeach()

string(FIND "${source_code}" "validate_and_copy(source, material)" preverify_pos)
string(FIND "${source_code}" "for (size_t i = 0; i < material.frames.size(); ++i)" object_loop_pos)
string(FIND "${source_code}" "if (!sync_fd(objects.get()) || !sync_fd(staging.get()))" object_sync_pos)
string(FIND "${source_code}" "const std::string final_manifest = manifest_name(material.selected_manifest_digest);" manifest_pos)
string(FIND "${source_code}" "publish_file(staging.get(), manifests.get()" manifest_publish_pos)
if(preverify_pos EQUAL -1 OR object_loop_pos EQUAL -1 OR object_sync_pos EQUAL -1 OR
   manifest_pos EQUAL -1 OR manifest_publish_pos EQUAL -1 OR
   NOT preverify_pos LESS manifest_pos OR NOT manifest_pos LESS object_loop_pos OR
   NOT object_loop_pos LESS object_sync_pos OR NOT object_sync_pos LESS manifest_publish_pos)
    message(FATAL_ERROR "publisher does not preverify and durably publish all objects before visibility manifest")
endif()

message(STATUS "HaloFPX excluded Linux full-v1 synthetic materialization contract passed")
