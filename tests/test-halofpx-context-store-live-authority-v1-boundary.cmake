if(NOT HALOFPX_CONTEXT_STORE_LIVE_AUTHORITY)
    message(FATAL_ERROR "live-authority boundary test requires its explicit test-only gate")
endif()

file(READ "${HALOFPX_SOURCE_DIR}/tools/server/CMakeLists.txt" server_cmake)
file(READ "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-live-authority-v1.h" header)
file(READ "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-live-authority-v1.cpp" source)
file(READ "${HALOFPX_SOURCE_DIR}/tools/server/server-context.cpp" server_context)
file(READ "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-state-transformer-v1.cpp" world1_state)
file(READ "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-v1-transformer-codec.cpp" world1_codec)

foreach(required
    "option(HALOFPX_CONTEXT_STORE_LIVE_AUTHORITY"
    "Build the isolated live-derived cache compatibility authority"
    "add_library(halofpx-context-store-live-authority-v1 STATIC EXCLUDE_FROM_ALL"
    "context_store_build_live_authority_v1"
    "context_store_live_authority_world_size_v1 = 2"
    "missing_model_artifact"
    "ambiguous_template_selection"
    "mutable_build_identity"
    "placeholder_equal_facts"
    "halofpx.compat-component.v1"
    "halofpx.source-tree.v1"
    "halofpx.rank-ownership.v1"
    "halofpx.rank-placement.v1")
    string(FIND "${server_cmake}${header}${source}" "${required}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "missing live-authority boundary token: ${required}")
    endif()
endforeach()

string(REGEX MATCH
    "option\\(HALOFPX_CONTEXT_STORE_LIVE_AUTHORITY[ \t\r\n]+\"Build the isolated live-derived cache compatibility authority\"[ \t\r\n]+OFF\\)"
    gate_default_off "${server_cmake}")
if(gate_default_off STREQUAL "")
    message(FATAL_ERROR "live-authority gate must remain default OFF")
endif()

# This slice must not change the admitted world-one server route.
foreach(required
    "profile.world_size == 1"
    "profile.rank == 0")
    string(FIND "${world1_state}" "${required}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "world-one transformer admission changed: ${required}")
    endif()
endforeach()
string(FIND "${world1_codec}" "candidate.world_size() != 1" codec_world1)
if(codec_world1 EQUAL -1)
    message(FATAL_ERROR "world-one transformer codec refusal boundary changed")
endif()
foreach(required
    "profile.world_size = 1"
    "profile.rank = 0"
    "exact.world_size = 1"
    "exact.rank = 0")
    string(FIND "${server_context}" "${required}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "world-one server route changed: ${required}")
    endif()
endforeach()

# The existing operator component list remains a canary-only seam. This new
# builder is not included, invoked, linked, or exposed through the server.
foreach(required_canary_token
    "halofpx_context_store_compatibility_components"
    "HALOFPX_CONTEXT_STORE_COMPONENT_AUTHORITY")
    string(FIND "${server_context}${server_cmake}" "${required_canary_token}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "existing canary component seam changed: ${required_canary_token}")
    endif()
endforeach()
foreach(forbidden_server_token
    "halofpx-context-store-live-authority-v1.h"
    "context_store_build_live_authority_v1")
    string(FIND "${server_context}" "${forbidden_server_token}" found)
    if(NOT found EQUAL -1)
        message(FATAL_ERROR "standalone authority entered server-context: ${forbidden_server_token}")
    endif()
endforeach()
foreach(forbidden_link
    "target_link_libraries(server-context PRIVATE halofpx-context-store-live-authority-v1"
    "target_link_libraries(llama-server PRIVATE halofpx-context-store-live-authority-v1"
    "target_link_libraries(llama-common PRIVATE halofpx-context-store-live-authority-v1")
    string(FIND "${server_cmake}" "${forbidden_link}" found)
    if(NOT found EQUAL -1)
        message(FATAL_ERROR "standalone authority linked into product target: ${forbidden_link}")
    endif()
endforeach()

foreach(forbidden
    "llama_context"
    "ggml_"
    "RPC_CMD"
    "filesystem"
    "fstream"
    "socket"
    "open("
    "rename("
    "unlink("
    "fsync("
    "CachyLlama")
    string(FIND "${header}${source}" "${forbidden}" found)
    if(NOT found EQUAL -1)
        message(FATAL_ERROR "forbidden product/I/O/donor token in standalone authority: ${forbidden}")
    endif()
endforeach()
