if(NOT DEFINED SOURCE_ROOT)
    message(FATAL_ERROR "SOURCE_ROOT is required")
endif()

set(server_cmake_path "${SOURCE_ROOT}/tools/server/CMakeLists.txt")
set(header_path "${SOURCE_ROOT}/tools/server/halofpx-context-store-v1-read-only.h")
set(source_path "${SOURCE_ROOT}/tools/server/halofpx-context-store-v1-read-only.cpp")
file(READ "${server_cmake_path}" server_cmake)
file(READ "${header_path}" header_text)
file(READ "${source_path}" source_text)

string(FIND "${server_cmake}"
    "add_library(halofpx-context-store-v1-read-only STATIC EXCLUDE_FROM_ALL" excluded_pos)
if(excluded_pos EQUAL -1)
    message(FATAL_ERROR "full-v1 reader must remain a STATIC EXCLUDE_FROM_ALL target")
endif()
string(REGEX MATCHALL "halofpx-context-store-v1-read-only" target_mentions "${server_cmake}")
list(LENGTH target_mentions target_mention_count)
if(NOT target_mention_count EQUAL 7)
    message(FATAL_ERROR "full-v1 reader escaped its isolated definition graph: ${target_mention_count} mentions")
endif()
string(FIND "${server_cmake}"
    "target_link_libraries(halofpx-context-store-v1-linux-publish PUBLIC\n        halofpx-context-store-v1-read-only\n        halofpx-context-store-v1-linux-read-only)" materializer_child_pos)
if(materializer_child_pos EQUAL -1)
    message(FATAL_ERROR "the second additional reader edge must be the excluded synthetic Linux materializer")
endif()
string(FIND "${server_cmake}"
    "target_link_libraries(halofpx-context-store-v1-linux-read-only PUBLIC\n        halofpx-context-store-v1-read-only)" linux_child_pos)
if(linux_child_pos EQUAL -1)
    message(FATAL_ERROR "the sole additional full-v1 reader edge must be the excluded Linux snapshot child")
endif()
string(FIND "${server_cmake}"
    "target_link_libraries(halofpx-context-store-v1-read-only PUBLIC\n    halofpx-context-store-object)" dependency_pos)
if(dependency_pos EQUAL -1)
    message(FATAL_ERROR "full-v1 reader dependency graph must terminate at the object verifier")
endif()

foreach(required_header IN ITEMS
        "#include \"halofpx-context-store-auth.h\""
        "#include \"halofpx-context-store-object.h\""
        "#include \"halofpx-context-store.h\"")
    string(FIND "${header_text}" "${required_header}" required_pos)
    if(required_pos EQUAL -1)
        message(FATAL_ERROR "missing expected target-native dependency: ${required_header}")
    endif()
endforeach()

# Comments describe the negative boundary; strip them before checking executable
# source so the audit proves the implementation imports no product/storage edge.
string(REGEX REPLACE "//[^\n]*" "" header_code "${header_text}")
string(REGEX REPLACE "//[^\n]*" "" source_code "${source_text}")
set(code "${header_code}\n${source_code}")
foreach(forbidden IN ITEMS
        "<filesystem>" "<fstream>" "windows.h" "unistd.h" "sys/stat.h"
        "CreateFile" "ReadFile" "fopen(" "ifstream" "std::filesystem"
        "llama-server" "server-context" "llama_context" "ggml_" "gguf"
        "CachyLLama" "llama-ai" "cachyllama" "llama_ai" "writer")
    string(FIND "${code}" "${forbidden}" forbidden_pos)
    if(NOT forbidden_pos EQUAL -1)
        message(FATAL_ERROR "forbidden filesystem/product/live-state/writer/donor term: ${forbidden}")
    endif()
endforeach()

string(FIND "${source_code}" "context_store_capabilities capabilities() const noexcept override { return {}; }" caps_pos)
if(caps_pos EQUAL -1)
    message(FATAL_ERROR "provider capabilities are not default closed")
endif()
string(FIND "${source_code}" "return context_store_publish_status::disabled;" publish_pos)
if(publish_pos EQUAL -1)
    message(FATAL_ERROR "provider publish path is not unconditionally disabled")
endif()

message(STATUS "HaloFPX excluded full-v1 memory reader static contract passed")
