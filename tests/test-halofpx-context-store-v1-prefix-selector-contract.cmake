if (NOT DEFINED HALOFPX_SOURCE_DIR OR NOT IS_DIRECTORY "${HALOFPX_SOURCE_DIR}")
    message(FATAL_ERROR "HALOFPX_SOURCE_DIR must name the source tree")
endif()

file(READ "${HALOFPX_SOURCE_DIR}/CMakeLists.txt" root_cmake)
file(READ "${HALOFPX_SOURCE_DIR}/tools/server/CMakeLists.txt" server_cmake)
string(FIND "${root_cmake}"
    "option(HALOFPX_CONTEXT_STORE_LONGEST_PREFIX_CANARY\n       \"Compile the non-product exact longest-prefix catalog selector canary\"\n       OFF)"
    default_off_option)
if (default_off_option EQUAL -1)
    message(FATAL_ERROR "longest-prefix selector option must remain literal default OFF")
endif()
string(FIND "${root_cmake}"
    "HALOFPX_CONTEXT_STORE_LONGEST_PREFIX_CANARY requires the exact-key catalog canary gate"
    catalog_gate)
string(FIND "${root_cmake}"
    "HALOFPX_CONTEXT_STORE_LONGEST_PREFIX_CANARY requires LLAMA_BUILD_COMMON, LLAMA_BUILD_TOOLS, and LLAMA_BUILD_SERVER"
    graph_gate)
string(FIND "${root_cmake}"
    "HALOFPX_CONTEXT_STORE_LONGEST_PREFIX_CANARY is supported only on Linux"
    linux_gate)
string(FIND "${root_cmake}" "add_subdirectory(tests)" tests_subdirectory)
string(FIND "${root_cmake}" "add_subdirectory(tools)" tools_subdirectory)
foreach(position IN ITEMS
        default_off_option catalog_gate graph_gate linux_gate
        tests_subdirectory tools_subdirectory)
    if (${position} EQUAL -1)
        message(FATAL_ERROR "missing early longest-prefix build position: ${position}")
    endif()
endforeach()
foreach(gate IN ITEMS default_off_option catalog_gate graph_gate linux_gate)
    if (NOT ${${gate}} LESS ${tests_subdirectory})
        message(FATAL_ERROR "longest-prefix ${gate} must precede test traversal")
    endif()
endforeach()
if (NOT ${tests_subdirectory} LESS ${tools_subdirectory})
    message(FATAL_ERROR "contract assumes tests precede the tools implementation target")
endif()
set(selector_target_block
"if (HALOFPX_CONTEXT_STORE_LONGEST_PREFIX_CANARY)\n        add_library(halofpx-context-store-v1-prefix-selector STATIC EXCLUDE_FROM_ALL\n            halofpx-context-store-v1-prefix-selector.cpp\n            halofpx-context-store-v1-prefix-selector.h\n        )\n        target_link_libraries(halofpx-context-store-v1-prefix-selector PUBLIC\n            halofpx-context-store-v1-catalog\n            halofpx-context-store-exact-session)\n        target_compile_features(halofpx-context-store-v1-prefix-selector PRIVATE cxx_std_17)\n    endif()")
string(FIND "${server_cmake}" "${selector_target_block}" selector_target)
if (selector_target EQUAL -1)
    message(FATAL_ERROR "missing isolated longest-prefix selector target")
endif()
string(FIND "${server_cmake}"
    "option(HALOFPX_CONTEXT_STORE_LONGEST_PREFIX_CANARY"
    late_option)
if (NOT late_option EQUAL -1)
    message(FATAL_ERROR "longest-prefix selector option was redeclared after test traversal")
endif()

# The exact isolated declaration above and the focused test link are the only
# admitted CMake references. Removing the declaration must leave no selector
# reference in tools/server, so a direct or intermediate product link cannot
# hide above the server-context marker.
string(REPLACE "${selector_target_block}" "" server_without_selector_target "${server_cmake}")
string(FIND "${server_without_selector_target}"
    "halofpx-context-store-v1-prefix-selector" extra_server_reference)
if (NOT extra_server_reference EQUAL -1)
    message(FATAL_ERROR "longest-prefix selector gained a non-isolated tools/server reference")
endif()

file(GLOB_RECURSE cmake_lists "${HALOFPX_SOURCE_DIR}/*/CMakeLists.txt")
foreach(cmake_list IN LISTS cmake_lists)
    if (cmake_list STREQUAL "${HALOFPX_SOURCE_DIR}/tools/server/CMakeLists.txt" OR
        cmake_list STREQUAL "${HALOFPX_SOURCE_DIR}/tests/CMakeLists.txt")
        continue()
    endif()
    file(READ "${cmake_list}" cmake_text)
    string(FIND "${cmake_text}"
        "halofpx-context-store-v1-prefix-selector" unexpected_reference)
    if (NOT unexpected_reference EQUAL -1)
        message(FATAL_ERROR
            "longest-prefix selector gained an unadmitted CMake reference: ${cmake_list}")
    endif()
endforeach()

file(READ
    "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-v1-prefix-selector.cpp"
    selector_source)
file(READ
    "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-v1-prefix-selector.h"
    selector_header)
string(APPEND selector_source "\n${selector_header}")
foreach(required
        "context_store_resolve_exact_session_v1"
        "restore_exact"
        "authenticated_state_corrupt"
        "residual_token_count")
    string(FIND "${selector_source}" "${required}" position)
    if (position EQUAL -1)
        message(FATAL_ERROR "missing exact-prefix selector boundary: ${required}")
    endif()
endforeach()
string(TOLOWER "${selector_source}" selector_lower)
foreach(forbidden "fnv" "<string>" "std::string" "string_view" "prompt_text" "system_boundary")
    string(FIND "${selector_lower}" "${forbidden}" position)
    if (NOT position EQUAL -1)
        message(FATAL_ERROR "weak/noncanonical selector mechanism present: ${forbidden}")
    endif()
endforeach()

message(STATUS "HaloFPX exact longest-prefix selector remains default-off and non-product")
