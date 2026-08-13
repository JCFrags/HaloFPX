if (NOT DEFINED HALOFPX_SOURCE_DIR OR NOT IS_DIRECTORY "${HALOFPX_SOURCE_DIR}")
    message(FATAL_ERROR "HALOFPX_SOURCE_DIR must name the source tree")
endif()
if (NOT DEFINED HALOFPX_BINARY_DIR OR NOT IS_DIRECTORY "${HALOFPX_BINARY_DIR}")
    message(FATAL_ERROR "HALOFPX_BINARY_DIR must name the configured build tree")
endif()

file(READ "${HALOFPX_SOURCE_DIR}/CMakeLists.txt" root_cmake)
file(READ "${HALOFPX_SOURCE_DIR}/tools/server/CMakeLists.txt" server_cmake)
file(READ "${HALOFPX_SOURCE_DIR}/tools/server/server-context.cpp" server_context)
file(READ "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-world1-live-authority-install-v1.h" header)
file(READ "${HALOFPX_SOURCE_DIR}/tools/server/halofpx-context-store-world1-live-authority-install-v1.cpp" source)

foreach(required
        "option(HALOFPX_CONTEXT_STORE_WORLD1_LIVE_AUTHORITY_INSTALL"
        "Compile the fail-closed world1 live-authority install boundary"
        "HALOFPX_CONTEXT_STORE_WORLD1_LIVE_AUTHORITY_INSTALL requires the world1 prefix product"
        "add_library(halofpx-context-store-world1-live-authority-install-v1 STATIC EXCLUDE_FROM_ALL"
        "context_store_install_world1_live_authority_v1"
        "context_store_world1_live_authority_required_facts_v1"
        "trusted_live_loader_context_lifecycle"
        "operator_components"
        "standalone_world2_authority"
        "incomplete_fact_custody"
        "model_generation_unavailable")
    string(FIND "${root_cmake}${server_cmake}${header}${source}" "${required}" found)
    if (found EQUAL -1)
        message(FATAL_ERROR "missing world1 live-authority boundary token: ${required}")
    endif()
endforeach()

set(default_off_block [=[option(HALOFPX_CONTEXT_STORE_WORLD1_LIVE_AUTHORITY_INSTALL
       "Compile the fail-closed world1 live-authority install boundary"
       OFF)]=])
string(FIND "${root_cmake}" "${default_off_block}" gate_default_off)
if (gate_default_off EQUAL -1)
    message(FATAL_ERROR "world1 live-authority install gate must remain default OFF")
endif()

# This slice wires a refusal boundary only. The production server deliberately
# supplies neither a source nor a generation and therefore cannot install a
# positive authority at this commit.
foreach(required_cold_token
        "#if defined(HALOFPX_CONTEXT_STORE_WORLD1_LIVE_AUTHORITY_INSTALL)"
        "request.source = nullptr"
        "request.expected_model_generation = 0"
        "halofpx_world1_cache_authority = std::move(installed.authority)")
    string(FIND "${server_context}" "${required_cold_token}" found)
    if (found EQUAL -1)
        message(FATAL_ERROR "server lost strict cold install-boundary token: ${required_cold_token}")
    endif()
endforeach()

foreach(single_assignment
        "            request.source ="
        "            request.expected_model_generation =")
    string(LENGTH "${server_context}" request_text_length)
    string(LENGTH "${single_assignment}" request_assignment_length)
    string(REPLACE "${single_assignment}" "" request_text_without_assignment
        "${server_context}")
    string(LENGTH "${request_text_without_assignment}" request_text_without_length)
    math(EXPR request_assignment_bytes
        "${request_text_length} - ${request_text_without_length}")
    math(EXPR request_assignment_count
        "${request_assignment_bytes} / ${request_assignment_length}")
    if (NOT request_assignment_count EQUAL 1)
        message(FATAL_ERROR
            "world1 install request custody must have one cold assignment for ${single_assignment}")
    endif()
endforeach()

string(REGEX MATCHALL
    "halofpx_world1_cache_authority[ \t\r\n]*="
    authority_install_assignments "${server_context}")
list(LENGTH authority_install_assignments authority_install_assignment_count)
if (NOT authority_install_assignment_count EQUAL 1)
    message(FATAL_ERROR
        "world1 server authority must have exactly one non-reset assignment through the typed installer")
endif()

foreach(forbidden_server_route
        "#include \"halofpx-context-store-live-authority-v1.h\""
        "context_store_build_live_authority_v1(")
    string(FIND "${server_context}" "${forbidden_server_route}" found)
    if (NOT found EQUAL -1)
        message(FATAL_ERROR
            "forbidden world2 route entered the world1 server: ${forbidden_server_route}")
    endif()
endforeach()

# The world-two standalone builder and operator component list are not source
# authority for this installer.
foreach(forbidden_install_token
        "halofpx-context-store-live-authority-v1.h"
        "context_store_build_live_authority_v1"
        "halofpx_context_store_compatibility_components"
        "getenv("
        "filesystem"
        "fstream")
    string(FIND "${header}${source}" "${forbidden_install_token}" found)
    if (NOT found EQUAL -1)
        message(FATAL_ERROR
            "forbidden authority construction/input entered install boundary: ${forbidden_install_token}")
    endif()
endforeach()

# Inspect the generated Linux build graph, not CMakeCache (which necessarily
# records the OFF option name). This proves the install source/macro is absent
# when the gate is off and present only in the explicitly gated build. The
# boundary cannot be enabled on other systems; their source contract still runs
# above without assuming a particular multi-configuration generator layout.
if (NOT HALOFPX_SYSTEM_NAME STREQUAL "Linux")
    if (HALOFPX_CONTEXT_STORE_WORLD1_LIVE_AUTHORITY_INSTALL)
        message(FATAL_ERROR "world1 install boundary cannot be enabled outside Linux")
    endif()
    message(STATUS "HaloFPX world1 live-authority install boundary passed")
    return()
endif()

set(graph_text "")
set(graph_candidates
    "${HALOFPX_BINARY_DIR}/build.ninja"
    "${HALOFPX_BINARY_DIR}/Makefile"
    "${HALOFPX_BINARY_DIR}/tools/server/Makefile"
    "${HALOFPX_BINARY_DIR}/tools/server/CMakeFiles/server-context.dir/flags.make"
    "${HALOFPX_BINARY_DIR}/tools/server/CMakeFiles/server-context.dir/build.make"
    "${HALOFPX_BINARY_DIR}/tools/server/CMakeFiles/server-context.dir/link.txt"
    "${HALOFPX_BINARY_DIR}/tools/server/CMakeFiles/llama-server.dir/link.txt")
foreach(graph_file IN LISTS graph_candidates)
    if (EXISTS "${graph_file}")
        file(READ "${graph_file}" graph_part)
        string(APPEND graph_text "\n${graph_part}")
    endif()
endforeach()
if (graph_text STREQUAL "")
    message(FATAL_ERROR "no supported generated build graph was available")
endif()

foreach(graph_token
        "halofpx-context-store-world1-live-authority-install-v1.cpp"
        "HALOFPX_CONTEXT_STORE_WORLD1_LIVE_AUTHORITY_INSTALL=1")
    string(FIND "${graph_text}" "${graph_token}" graph_position)
    if (HALOFPX_CONTEXT_STORE_WORLD1_LIVE_AUTHORITY_INSTALL)
        if (graph_position EQUAL -1)
            message(FATAL_ERROR "install-on graph lost token: ${graph_token}")
        endif()
    elseif (NOT graph_position EQUAL -1)
        message(FATAL_ERROR "install-off graph contains gated token: ${graph_token}")
    endif()
endforeach()

message(STATUS "HaloFPX world1 live-authority install boundary passed")
