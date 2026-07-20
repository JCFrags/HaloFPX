if(NOT DEFINED SOURCE_ROOT)
    message(FATAL_ERROR "SOURCE_ROOT is required")
endif()

set(server_cmake_path "${SOURCE_ROOT}/tools/server/CMakeLists.txt")
set(header_path "${SOURCE_ROOT}/tools/server/halofpx-context-store-v1-transformer-codec.h")
set(source_path "${SOURCE_ROOT}/tools/server/halofpx-context-store-v1-transformer-codec.cpp")
file(READ "${server_cmake_path}" server_cmake)
file(READ "${header_path}" header_text)
file(READ "${source_path}" source_text)

string(FIND "${server_cmake}"
    "add_library(halofpx-context-store-v1-transformer-codec STATIC EXCLUDE_FROM_ALL" excluded_pos)
if(excluded_pos EQUAL -1)
    message(FATAL_ERROR "full-v1 transformer codec must remain STATIC EXCLUDE_FROM_ALL")
endif()
string(REGEX MATCHALL "halofpx-context-store-v1-transformer-codec" target_mentions "${server_cmake}")
list(LENGTH target_mentions target_mention_count)
if(NOT target_mention_count EQUAL 6)
    message(FATAL_ERROR "full-v1 transformer codec escaped its isolated definition graph: ${target_mention_count}")
endif()
string(FIND "${server_cmake}"
    "if (HALOFPX_CONTEXT_STORE_FULL_V1_CANARY)" full_v1_gate_pos)
string(FIND "${server_cmake}"
    "target_link_libraries(\${TARGET} PRIVATE halofpx-context-store-v1-server-canary)" full_v1_link_pos)
if(full_v1_gate_pos EQUAL -1 OR full_v1_link_pos EQUAL -1 OR
   full_v1_link_pos LESS full_v1_gate_pos)
    message(FATAL_ERROR "full-v1 codec server edge is not confined to its explicit gate")
endif()
string(FIND "${server_cmake}"
    "target_link_libraries(halofpx-context-store-v1-transformer-codec PUBLIC\n    halofpx-context-store-auth\n    halofpx-context-store-state-transformer-v1)" dependency_pos)
if(dependency_pos EQUAL -1)
    message(FATAL_ERROR "full-v1 transformer codec dependency graph widened")
endif()

string(REGEX REPLACE "//[^\n]*" "" header_code "${header_text}")
string(REGEX REPLACE "//[^\n]*" "" source_code "${source_text}")
set(code "${header_code}\n${source_code}")
foreach(forbidden IN ITEMS
        "<filesystem>" "<fstream>" "windows.h" "unistd.h" "sys/stat.h"
        "CreateFile" "ReadFile" "fopen(" "ifstream" "std::filesystem"
        "server-context" "server-task" "llama-server" "llama_state_seq_set_data"
        "context_store_restore_transformer_state" "CachyLLama" "llama-ai"
        "cachyllama" "llama_ai")
    string(FIND "${code}" "${forbidden}" forbidden_pos)
    if(NOT forbidden_pos EQUAL -1)
        message(FATAL_ERROR "forbidden filesystem/product/live-restore/donor term: ${forbidden}")
    endif()
endforeach()

foreach(required IN ITEMS
        "context_store_verify_manifest_v1("
        "context_store_manifest_digest_v1("
        "context_store_sha256("
        "context_store_hmac_sha256("
        "context_store_v1_read_only_candidate")
    string(FIND "${code}" "${required}" required_pos)
    if(required_pos EQUAL -1)
        message(FATAL_ERROR "missing target-native authenticated format/read API: ${required}")
    endif()
endforeach()

message(STATUS "HaloFPX excluded full-v1 transformer codec static contract passed")
