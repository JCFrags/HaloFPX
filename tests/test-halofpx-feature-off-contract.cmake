if (NOT DEFINED HALOFPX_SERVER OR NOT EXISTS "${HALOFPX_SERVER}")
    message(FATAL_ERROR "HALOFPX_SERVER must name the built llama-server executable")
endif()

if (NOT DEFINED HALOFPX_SOURCE_DIR OR NOT IS_DIRECTORY "${HALOFPX_SOURCE_DIR}")
    message(FATAL_ERROR "HALOFPX_SOURCE_DIR must name the source tree")
endif()

execute_process(
    COMMAND "${HALOFPX_SERVER}" --help
    RESULT_VARIABLE server_help_result
    OUTPUT_VARIABLE server_help
    ERROR_VARIABLE server_help_error)

if (NOT server_help_result EQUAL 0)
    message(FATAL_ERROR "llama-server --help failed: ${server_help_error}")
endif()

foreach(required_option
        "--host"
        "--port"
        "--api-key"
        "--model"
        "--cache-type-k"
        "--cache-type-v"
        "--slot-save-path"
        "--offline")
    string(FIND "${server_help}" "${required_option}" option_position)
    if (option_position EQUAL -1)
        message(FATAL_ERROR "feature-off CLI surface lost required option: ${required_option}")
    endif()
endforeach()

if (NOT HALOFPX_CONTEXT_STORE_CANARY)
    string(FIND "${server_help}" "--halofpx-context-store-" canary_option_position)
    if (NOT canary_option_position EQUAL -1)
        message(FATAL_ERROR "feature-off CLI exposes HaloFPX context-store options")
    endif()
elseif (NOT HALOFPX_CONTEXT_STORE_PROTECTED_CANARY)
    foreach(forbidden_surface
            "protected-rw-canary"
            "--halofpx-context-store-anchor-root"
            "--halofpx-context-store-uuid")
        string(FIND "${server_help}" "${forbidden_surface}" forbidden_position)
        if (NOT forbidden_position EQUAL -1)
            message(FATAL_ERROR "protected feature-off CLI exposes: ${forbidden_surface}")
        endif()
    endforeach()
endif()

if (NOT HALOFPX_CONTEXT_STORE_COMPONENT_AUTHORITY)
    string(FIND "${server_help}"
        "--halofpx-context-store-compatibility-component" canonical_component_position)
    if (NOT canonical_component_position EQUAL -1)
        message(FATAL_ERROR "canonical compatibility feature-off CLI surface is exposed")
    endif()
else()
    string(FIND "${server_help}"
        "--halofpx-context-store-compatibility-component" canonical_component_position)
    if (canonical_component_position EQUAL -1)
        message(FATAL_ERROR "canonical compatibility build lost its explicit component option")
    endif()
endif()

file(READ "${HALOFPX_SOURCE_DIR}/common/common.h" common_header)
string(REGEX MATCH "hostname[ \t]+=[ \t]+\"127\\.0\\.0\\.1\"" loopback_default "${common_header}")
if (NOT loopback_default)
    message(FATAL_ERROR "feature-off server no longer defaults to loopback")
endif()

file(READ "${HALOFPX_SOURCE_DIR}/tools/server/server.cpp" server_source)
foreach(required_route
        "\"/health\""
        "\"/v1/models\""
        "\"/v1/chat/completions\""
        "\"/completion\""
        "\"/props\""
        "\"/slots\"")
    string(FIND "${server_source}" "${required_route}" route_position)
    if (route_position EQUAL -1)
        message(FATAL_ERROR "feature-off server lost required route registration: ${required_route}")
    endif()
endforeach()

message(STATUS "HaloFPX feature-off contract passed")
