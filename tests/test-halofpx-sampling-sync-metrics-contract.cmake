if (NOT DEFINED HALOFPX_SOURCE_DIR OR NOT IS_DIRECTORY "${HALOFPX_SOURCE_DIR}")
    message(FATAL_ERROR "HALOFPX_SOURCE_DIR must name the source tree")
endif()

file(READ "${HALOFPX_SOURCE_DIR}/CMakeLists.txt" root_cmake)
set(expected_option [=[option(HALOFPX_SAMPLING_SYNC_COALESCE_CANARY
       "Coalesce redundant completed output synchronizations during sampling"
       OFF)]=])
string(FIND "${root_cmake}" "${expected_option}" option_default_off)
if (option_default_off EQUAL -1)
    message(FATAL_ERROR "sampling synchronization canary is not default OFF")
endif()

file(READ "${HALOFPX_SOURCE_DIR}/src/llama-ext.h" llama_ext)
foreach(required_getter_contract
        "Reading the counters does not synchronize"
        "llama_get_output_sync_stats"
        "uint64_t generation"
        "uint64_t completed_barriers"
        "uint64_t reused_barriers"
        "uint64_t graph_submissions"
        "uint64_t output_transfers")
    string(FIND "${llama_ext}" "${required_getter_contract}" contract_position)
    if (contract_position EQUAL -1)
        message(FATAL_ERROR "non-sync output counter contract is missing: ${required_getter_contract}")
    endif()
endforeach()

file(READ "${HALOFPX_SOURCE_DIR}/tools/server/server-task.h" server_task_header)
foreach(required_carrier
        "server_sampling_sync_metrics sampling_sync"
        "bool include_sampling_sync_metrics = false"
        "Set only by GET /metrics"
        "GET /slots shares the task type"
        "completion, slot-route, or streaming-request metadata")
    string(FIND "${server_task_header}" "${required_carrier}" carrier_position)
    if (carrier_position EQUAL -1)
        message(FATAL_ERROR "typed metrics carrier contract is missing: ${required_carrier}")
    endif()
endforeach()

file(READ "${HALOFPX_SOURCE_DIR}/tools/server/server-task.cpp" server_task_source)
string(FIND "${server_task_source}" "sampling_sync" serialized_sampling_sync)
if (NOT serialized_sampling_sync EQUAL -1)
    message(FATAL_ERROR "context-lifetime sampling counters leaked into task JSON")
endif()

file(READ "${HALOFPX_SOURCE_DIR}/tools/server/server-context.cpp" server_context)
set(getter_call "llama_get_output_sync_stats(")
string(FIND "${server_context}" "${getter_call}" first_getter_call)
if (first_getter_call EQUAL -1)
    message(FATAL_ERROR "server metrics task lost the output counter getter")
endif()
math(EXPR after_first_getter "${first_getter_call} + 1")
string(SUBSTRING "${server_context}" ${after_first_getter} -1 after_first_getter_text)
string(FIND "${after_first_getter_text}" "${getter_call}" second_getter_call)
if (NOT second_getter_call EQUAL -1)
    message(FATAL_ERROR "server must read output counters exactly once, in the metrics task")
endif()

foreach(required_server_seam
        "case SERVER_TASK_TYPE_METRICS"
        "if (task.include_sampling_sync_metrics)"
        "llama_get_output_sync_stats(ctx_tgt, &output_sync_stats)"
        "server_write_sampling_sync_prometheus(prometheus, res_task->sampling_sync)")
    string(FIND "${server_context}" "${required_server_seam}" seam_position)
    if (seam_position EQUAL -1)
        message(FATAL_ERROR "metrics-only server seam is missing: ${required_server_seam}")
    endif()
endforeach()

# GET /metrics and GET /slots intentionally share SERVER_TASK_TYPE_METRICS.
# The flag is default false and must be enabled only by the metrics route, so
# the slots route neither invokes nor gains a failure dependency on the getter.
set(include_assignment "task.include_sampling_sync_metrics = true;")
string(FIND "${server_context}" "this->get_metrics =" metrics_route_start)
string(FIND "${server_context}" "this->get_slots =" slots_route_start)
string(FIND "${server_context}" "this->post_slots =" post_slots_route_start)
if (metrics_route_start EQUAL -1 OR slots_route_start EQUAL -1 OR post_slots_route_start EQUAL -1)
    message(FATAL_ERROR "metrics/slots route boundaries are missing")
endif()
math(EXPR metrics_route_length "${slots_route_start} - ${metrics_route_start}")
math(EXPR slots_route_length "${post_slots_route_start} - ${slots_route_start}")
string(SUBSTRING "${server_context}" ${metrics_route_start} ${metrics_route_length} metrics_route)
string(SUBSTRING "${server_context}" ${slots_route_start} ${slots_route_length} slots_route)
string(FIND "${metrics_route}" "${include_assignment}" metrics_flag_position)
if (metrics_flag_position EQUAL -1)
    message(FATAL_ERROR "GET /metrics no longer requests sampling synchronization metrics")
endif()
string(FIND "${slots_route}" "${include_assignment}" slots_flag_position)
if (NOT slots_flag_position EQUAL -1)
    message(FATAL_ERROR "GET /slots must not request sampling synchronization metrics")
endif()
string(FIND "${server_context}" "${include_assignment}" first_include_assignment)
math(EXPR after_first_include_assignment "${first_include_assignment} + 1")
string(SUBSTRING "${server_context}" ${after_first_include_assignment} -1 after_first_include_text)
string(FIND "${after_first_include_text}" "${include_assignment}" second_include_assignment)
if (NOT second_include_assignment EQUAL -1)
    message(FATAL_ERROR "sampling synchronization metrics flag must be enabled by GET /metrics only")
endif()

file(READ "${HALOFPX_SOURCE_DIR}/tools/server/server-metrics-prometheus.h" prometheus_header)
foreach(required_metric
        "halofpx_sampling_sync_output_epochs_total"
        "halofpx_sampling_sync_completed_barriers_total"
        "halofpx_sampling_sync_reused_barriers_total"
        "halofpx_sampling_sync_graph_submissions_total"
        "halofpx_sampling_sync_output_transfers_total"
        "std::to_chars")
    string(FIND "${prometheus_header}" "${required_metric}" metric_position)
    if (metric_position EQUAL -1)
        message(FATAL_ERROR "exact cumulative metric contract is missing: ${required_metric}")
    endif()
endforeach()

string(FIND "${prometheus_header}" "halofpx_sampling_sync_generations_total" stale_generation_name)
if (NOT stale_generation_name EQUAL -1)
    message(FATAL_ERROR "output epochs must not be mislabeled as request/model generations")
endif()

foreach(forbidden_request_attribution
        "id_task"
        "id_slot"
        "request_id"
        "completion_id"
        "sse_event")
    string(FIND "${prometheus_header}" "${forbidden_request_attribution}" forbidden_position)
    if (NOT forbidden_position EQUAL -1)
        message(FATAL_ERROR "cumulative output metrics gained request attribution: ${forbidden_request_attribution}")
    endif()
endforeach()

message(STATUS "HaloFPX sampling synchronization metrics source contract passed")
