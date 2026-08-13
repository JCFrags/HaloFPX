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

if (NOT HALOFPX_CONTEXT_STORE_EXACT_KEY_CATALOG_CANARY)
    string(FIND "${server_help}" "full-v1-exact-key-catalog-canary" catalog_position)
    if (NOT catalog_position EQUAL -1)
        message(FATAL_ERROR "multi-entry catalog feature-off CLI surface is exposed")
    endif()
else()
    string(FIND "${server_help}" "full-v1-exact-key-catalog-canary" catalog_position)
    if (catalog_position EQUAL -1)
        message(FATAL_ERROR "catalog canary build lost its explicit runtime mode")
    endif()
endif()

if (NOT HALOFPX_CONTEXT_STORE_EXACT_KEY_CANARY)
    string(FIND "${server_help}" "full-v1-exact-key-canary" exact_key_position)
    if (NOT exact_key_position EQUAL -1)
        message(FATAL_ERROR "exact-key feature-off CLI surface is exposed")
    endif()
else()
    string(FIND "${server_help}" "full-v1-exact-key-canary" exact_key_position)
    if (exact_key_position EQUAL -1)
        message(FATAL_ERROR "exact-key canary build lost its explicit runtime mode")
    endif()
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

if (NOT HALOFPX_CONTEXT_STORE_FULL_V1_CANARY)
    string(FIND "${server_help}" "full-v1-rw-canary" full_v1_canary_position)
    if (NOT full_v1_canary_position EQUAL -1)
        message(FATAL_ERROR "full-v1 feature-off CLI surface is exposed")
    endif()
else()
    string(FIND "${server_help}" "full-v1-rw-canary" full_v1_canary_position)
    if (full_v1_canary_position EQUAL -1)
        message(FATAL_ERROR "full-v1 canary build lost its explicit runtime mode")
    endif()
endif()

set(inclusive_telemetry_binary_markers
    "selected_slot_transition_measured"
    "state_apply_input_bytes_valid"
    "postlaunch_idle_slot_saves_measured"
    "preprompt_cache_maintenance_ns")
file(STRINGS "${HALOFPX_SERVER}" inclusive_telemetry_binary_strings
    REGEX "selected_slot_transition_measured|state_apply_input_bytes_valid|postlaunch_idle_slot_saves_measured|preprompt_cache_maintenance_ns")
foreach(inclusive_telemetry_marker IN LISTS inclusive_telemetry_binary_markers)
    string(FIND "${inclusive_telemetry_binary_strings}"
        "${inclusive_telemetry_marker}" binary_marker_position)
    if (HALOFPX_CONTEXT_STORE_WORLD1_PREFIX_PRODUCT AND
        binary_marker_position EQUAL -1)
        message(FATAL_ERROR
            "world1 prefix product binary lost telemetry marker: ${inclusive_telemetry_marker}")
    elseif (NOT HALOFPX_CONTEXT_STORE_WORLD1_PREFIX_PRODUCT AND
            NOT binary_marker_position EQUAL -1)
        message(FATAL_ERROR
            "feature-off server binary contains product telemetry marker: ${inclusive_telemetry_marker}")
    endif()
endforeach()

if (NOT HALOFPX_CONTEXT_STORE_WORLD1_PREFIX_PRODUCT)
    # The selector alone remains non-product and must expose no server mode.
    foreach(forbidden_prefix_surface
            "longest-prefix"
            "prefix-selector"
            "full-v1-world1-prefix-product")
        string(FIND "${server_help}" "${forbidden_prefix_surface}" prefix_position)
        if (NOT prefix_position EQUAL -1)
            message(FATAL_ERROR "prefix feature-off surface is exposed: ${forbidden_prefix_surface}")
        endif()
    endforeach()
else()
    string(FIND "${server_help}" "full-v1-world1-prefix-product" prefix_product_position)
    if (prefix_product_position EQUAL -1)
        message(FATAL_ERROR "world1 prefix product build lost its explicit runtime mode")
    endif()
    file(READ "${HALOFPX_SOURCE_DIR}/tools/server/server-context.cpp" server_context_source)
    foreach(required_product_lifecycle_guard
            "halofpx_prefix_product_request_fits_slot"
            "task.n_tokens() < slot.n_ctx"
            "task.n_tokens() <= static_cast<int32_t>(llama_n_ubatch(ctx_tgt))"
            "task.need_logits() && !llama_get_memory(ctx_tgt)"
            "carrier.bound_authority"
            "context_store_world1_cache_authority_v1_matches"
            "telemetry.clear_attempt_measurements()"
            "selected.selected_slot_transition_measured = false"
            "selected.selected_slot_transition_ns = 0"
            "lookup_clock.finish()"
            "install_clock.finish()"
            "slot.n_prompt_tokens_cache > 0")
        string(FIND "${server_context_source}" "${required_product_lifecycle_guard}"
            lifecycle_guard_position)
        if (lifecycle_guard_position EQUAL -1)
            message(FATAL_ERROR
                "world1 prefix product lost pre-restore lifecycle guard: ${required_product_lifecycle_guard}")
        endif()
    endforeach()
    file(READ "${HALOFPX_SOURCE_DIR}/tools/server/server-task.cpp" server_task_source)
    foreach(required_product_telemetry
            "\"match_kind\""
            "\"reuse_tier\""
            "\"restored_state_tokens\""
            "\"logical_residual_tokens\""
            "\"work_accounting_valid\""
            "\"actual_prompt_tokens\""
            "\"avoided_prompt_tokens\""
            "\"selected_slot_transition_measured\""
            "\"selected_slot_transition_ns\""
            "\"lookup_total_ns\""
            "\"state_install_cleanup_ns\""
            "\"state_apply_input_bytes_valid\""
            "\"state_apply_input_bytes\""
            "\"postlaunch_idle_slot_saves_measured\""
            "\"postlaunch_idle_slot_saves_ns\""
            "\"preprompt_cache_maintenance_valid\""
            "\"preprompt_cache_maintenance_ns\"")
        string(FIND "${server_task_source}" "${required_product_telemetry}"
            product_telemetry_position)
        if (product_telemetry_position EQUAL -1)
            message(FATAL_ERROR
                "world1 prefix product lost telemetry field: ${required_product_telemetry}")
        endif()
    endforeach()
    foreach(forbidden_product_telemetry
            "{\"source\""
            "{\"restored_tokens\""
            "{\"residual_tokens\""
            "{\"lookup_validation_ns\""
            "{\"state_install_ns\""
            "{\"physical_bytes\""
            "{\"read_bytes\""
            "{\"total_io_bytes\"")
        string(FIND "${server_task_source}" "${forbidden_product_telemetry}"
            ambiguous_product_telemetry_position)
        if (NOT ambiguous_product_telemetry_position EQUAL -1)
            message(FATAL_ERROR
                "world1 prefix product exposes ambiguous telemetry field: ${forbidden_product_telemetry}")
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
