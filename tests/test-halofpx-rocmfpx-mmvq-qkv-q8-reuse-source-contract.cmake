if(NOT DEFINED SOURCE_ROOT OR SOURCE_ROOT STREQUAL "")
    message(FATAL_ERROR "SOURCE_ROOT is required")
endif()

set(GGML_OPTIONS "${SOURCE_ROOT}/ggml/CMakeLists.txt")
set(HIP_CMAKE "${SOURCE_ROOT}/ggml/src/ggml-hip/CMakeLists.txt")
set(CUDA_DISPATCH "${SOURCE_ROOT}/ggml/src/ggml-cuda/ggml-cuda.cu")
set(MMVQ_SOURCE "${SOURCE_ROOT}/ggml/src/ggml-cuda/mmvq.cu")
set(MMVQ_HEADER "${SOURCE_ROOT}/ggml/src/ggml-cuda/rocmfpx-mmvq-qkv-q8-reuse.h")
set(PROMPT_QKV_HEADER "${SOURCE_ROOT}/ggml/src/ggml-cuda/rocmfpx-qkv-q8-reuse.h")
set(BACKEND_SOURCE "${SOURCE_ROOT}/ggml/src/ggml-backend.cpp")
set(BACKEND_TEST "${SOURCE_ROOT}/tests/test-backend-ops.cpp")
set(HOST_TEST "${SOURCE_ROOT}/tests/test-halofpx-rocmfpx-mmvq-qkv-q8-reuse.cpp")

foreach(REQUIRED_FILE IN ITEMS
        "${GGML_OPTIONS}" "${HIP_CMAKE}" "${CUDA_DISPATCH}"
        "${MMVQ_SOURCE}" "${MMVQ_HEADER}" "${PROMPT_QKV_HEADER}"
        "${BACKEND_SOURCE}" "${BACKEND_TEST}" "${HOST_TEST}")
    if(NOT EXISTS "${REQUIRED_FILE}")
        message(FATAL_ERROR "missing source-contract input: ${REQUIRED_FILE}")
    endif()
endforeach()

foreach(INPUT IN ITEMS GGML_OPTIONS HIP_CMAKE CUDA_DISPATCH MMVQ_SOURCE MMVQ_HEADER PROMPT_QKV_HEADER BACKEND_SOURCE BACKEND_TEST HOST_TEST)
    file(READ "${${INPUT}}" ${INPUT}_TEXT)
endforeach()

function(require_text TEXT_VARIABLE NEEDLE FAILURE)
    string(FIND "${${TEXT_VARIABLE}}" "${NEEDLE}" MATCH)
    if(MATCH EQUAL -1)
        message(FATAL_ERROR "${FAILURE}: ${NEEDLE}")
    endif()
endfunction()

function(extract_unique_region TEXT_VARIABLE BEGIN_MARKER END_MARKER OUTPUT_VARIABLE)
    string(REGEX MATCHALL "${BEGIN_MARKER}" BEGIN_MATCHES "${${TEXT_VARIABLE}}")
    string(REGEX MATCHALL "${END_MARKER}" END_MATCHES "${${TEXT_VARIABLE}}")
    list(LENGTH BEGIN_MATCHES BEGIN_COUNT)
    list(LENGTH END_MATCHES END_COUNT)
    if(NOT BEGIN_COUNT EQUAL 1 OR NOT END_COUNT EQUAL 1)
        message(FATAL_ERROR "expected one ${BEGIN_MARKER}/${END_MARKER} region")
    endif()
    string(FIND "${${TEXT_VARIABLE}}" "${BEGIN_MARKER}" BEGIN_OFFSET)
    string(FIND "${${TEXT_VARIABLE}}" "${END_MARKER}" END_OFFSET)
    if(END_OFFSET LESS_EQUAL BEGIN_OFFSET)
        message(FATAL_ERROR "source region is reversed: ${BEGIN_MARKER}")
    endif()
    math(EXPR REGION_LENGTH "${END_OFFSET} - ${BEGIN_OFFSET}")
    string(SUBSTRING "${${TEXT_VARIABLE}}" ${BEGIN_OFFSET} ${REGION_LENGTH} REGION)
    set(${OUTPUT_VARIABLE} "${REGION}" PARENT_SCOPE)
endfunction()

require_text(GGML_OPTIONS_TEXT
    "option(GGML_HIP_ROCMFPX_MMVQ_QKV_Q8_REUSE    \"ggml: reuse one Q8_1 activation conversion across strict n=1 ROCmFPX Q/K/V MMVQs on gfx1151\" OFF)"
    "MMVQ QKV option is missing or not default-off")
require_text(GGML_OPTIONS_TEXT
    "if (GGML_HIP_ROCMFPX_MMVQ_QKV_Q8_REUSE AND NOT GGML_HIP)"
    "feature-on without HIP is not rejected")
require_text(HIP_CMAKE_TEXT
    "target_compile_definitions(ggml-hip PRIVATE GGML_HIP_ROCMFPX_MMVQ_QKV_Q8_REUSE)"
    "feature macro is not private to ggml-hip")
require_text(GGML_OPTIONS_TEXT
    "option(GGML_HIP_ROCMFPX_QKV_Q8_REUSE         \"ggml: reuse one Q8_1 activation conversion across separate ROCmFPX Q/K/V prompt MMQs on gfx1151\" OFF)"
    "composed prompt QKV option was removed or is no longer default-off")
require_text(HIP_CMAKE_TEXT
    "target_compile_definitions(ggml-hip PRIVATE GGML_HIP_ROCMFPX_QKV_Q8_REUSE)"
    "composed prompt QKV macro is no longer private to ggml-hip")
require_text(PROMPT_QKV_HEADER_TEXT
    "HALOFPX_ROCMFPX_QKV_Q8_REUSE_MAGIC_PARAM = 14"
    "prompt QKV marker ownership changed")
require_text(PROMPT_QKV_HEADER_TEXT
    "HALOFPX_ROCMFPX_QKV_Q8_REUSE_ROLE_PARAM = 15"
    "prompt QKV role-marker ownership changed")
require_text(MMVQ_HEADER_TEXT
    "HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_MAGIC_PARAM = 12"
    "decode QKV marker ownership changed")
require_text(MMVQ_HEADER_TEXT
    "HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_ROLE_PARAM  = 13"
    "decode QKV role-marker ownership changed")
require_text(MMVQ_HEADER_TEXT
    "contract.activation_rows == 1"
    "selector is not strict n=1")
require_text(MMVQ_HEADER_TEXT
    "halofpx_rocmfpx_mmvq_qkv_view_root(crossed)"
    "crossed-write gate does not resolve nested view ancestry")
require_text(MMVQ_HEADER_TEXT
    "halofpx_rocmfpx_mmvq_qkv_validate_marked_runtime_group"
    "read-only runtime graph revalidation is missing")
require_text(CUDA_DISPATCH_TEXT
    "halofpx_rocmfpx_mmvq_qkv_validate_marked_runtime_group(cgraph, node_index)"
    "runtime dispatch trusts optimizer markers without current-graph revalidation")
require_text(BACKEND_SOURCE_TEXT
    "ggml_backend_graph_optimize(sched->backends[split->backend_id], &split->graph);"
    "production optimizer is not called before scheduler graph allocation")

extract_unique_region(
    CUDA_DISPATCH_TEXT
    "HALOFPX_MMVQ_QKV_Q8_REUSE_OPTIMIZER_BEGIN"
    "HALOFPX_MMVQ_QKV_Q8_REUSE_OPTIMIZER_END"
    OPTIMIZER_REGION)
foreach(REQUIRED_OPTIMIZER_TEXT IN ITEMS
        "halofpx_rocmfpx_mmvq_qkv_dispatch_graph_reorder(cgraph, mmvq_gfx1151_hip)"
        "cuda_ctx->stream_context().reset();"
        "return;")
    require_text(OPTIMIZER_REGION "${REQUIRED_OPTIMIZER_TEXT}" "optimizer ownership contract is incomplete")
endforeach()
string(FIND "${CUDA_DISPATCH_TEXT}" "HALOFPX_MMVQ_QKV_Q8_REUSE_OPTIMIZER_BEGIN" DECODE_OPTIMIZER_OFFSET)
string(FIND "${CUDA_DISPATCH_TEXT}" "HALOFPX_QKV_Q8_REUSE_REORDER_BEGIN" PROMPT_OPTIMIZER_OFFSET)
if(DECODE_OPTIMIZER_OFFSET EQUAL -1 OR PROMPT_OPTIMIZER_OFFSET EQUAL -1 OR
        NOT DECODE_OPTIMIZER_OFFSET LESS PROMPT_OPTIMIZER_OFFSET)
    message(FATAL_ERROR "strict n=1 decode optimizer must precede the prompt planner's graph scan")
endif()

# Prompt and decode planning/dispatch must coexist in the production source;
# their admission ranges are intentionally disjoint (>8 prompt columns vs n=1).
foreach(REQUIRED_COMPOSITION_TEXT IN ITEMS
        "HALOFPX_QKV_Q8_REUSE_REORDER_BEGIN"
        "HALOFPX_QKV_Q8_REUSE_DISPATCH_BEGIN"
        "ggml_cuda_mul_mat_q_rocmfpx_triple("
        "HALOFPX_MMVQ_QKV_Q8_REUSE_OPTIMIZER_BEGIN"
        "HALOFPX_MMVQ_QKV_Q8_REUSE_DISPATCH_BEGIN"
        "ggml_cuda_mul_mat_vec_q_rocmfpx_triple(")
    require_text(CUDA_DISPATCH_TEXT "${REQUIRED_COMPOSITION_TEXT}" "prompt/decode QKV composition is incomplete")
endforeach()
string(FIND "${CUDA_DISPATCH_TEXT}" "HALOFPX_MMVQ_QKV_Q8_REUSE_DISPATCH_BEGIN" DECODE_DISPATCH_OFFSET)
string(FIND "${CUDA_DISPATCH_TEXT}" "HALOFPX_QKV_Q8_REUSE_DISPATCH_BEGIN" PROMPT_DISPATCH_OFFSET)
if(DECODE_DISPATCH_OFFSET EQUAL -1 OR PROMPT_DISPATCH_OFFSET EQUAL -1 OR
        NOT DECODE_DISPATCH_OFFSET LESS PROMPT_DISPATCH_OFFSET)
    message(FATAL_ERROR "strict n=1 decode dispatch must precede the prompt selector's graph-group fallback")
endif()

extract_unique_region(
    CUDA_DISPATCH_TEXT
    "HALOFPX_MMVQ_QKV_Q8_REUSE_DISPATCH_BEGIN"
    "HALOFPX_MMVQ_QKV_Q8_REUSE_DISPATCH_END"
    DISPATCH_REGION)
foreach(REQUIRED_DISPATCH_TEXT IN ITEMS
        "ggml_cuda_should_reuse_rocmfpx_mmvq_qkv_q8(*cuda_ctx, cgraph, i)"
        "ggml_cuda_mul_mat_vec_q_rocmfpx_triple("
        "return 2;")
    require_text(DISPATCH_REGION "${REQUIRED_DISPATCH_TEXT}" "triple dispatch contract is incomplete")
endforeach()

extract_unique_region(
    MMVQ_SOURCE_TEXT
    "HALOFPX_MMVQ_QKV_Q8_REUSE_TRIPLE_BEGIN"
    "HALOFPX_MMVQ_QKV_Q8_REUSE_TRIPLE_END"
    TRIPLE_REGION)
string(REGEX MATCHALL "quantize_row_q8_1_cuda\\(" TRIPLE_QUANTIZE_CALLS "${TRIPLE_REGION}")
list(LENGTH TRIPLE_QUANTIZE_CALLS TRIPLE_QUANTIZE_COUNT)
if(NOT TRIPLE_QUANTIZE_COUNT EQUAL 1)
    message(FATAL_ERROR "triple must contain exactly one Q8_1 conversion call; found ${TRIPLE_QUANTIZE_COUNT}")
endif()
string(REGEX MATCHALL "ggml_cuda_pool_alloc<char>" TRIPLE_ALLOCATIONS "${TRIPLE_REGION}")
list(LENGTH TRIPLE_ALLOCATIONS TRIPLE_ALLOCATION_COUNT)
if(NOT TRIPLE_ALLOCATION_COUNT EQUAL 1)
    message(FATAL_ERROR "triple must contain exactly one shared pool allocation")
endif()
string(REGEX MATCHALL "mul_mat_vec_q_switch_type\\(" TRIPLE_MMVQ_CALLS "${TRIPLE_REGION}")
list(LENGTH TRIPLE_MMVQ_CALLS TRIPLE_MMVQ_CALL_COUNT)
if(NOT TRIPLE_MMVQ_CALL_COUNT EQUAL 1)
    message(FATAL_ERROR "triple launch helper must contain one unchanged MMVQ dispatcher call")
endif()
string(FIND "${TRIPLE_REGION}" "launch(src0_q, dst_q);" LAUNCH_Q)
string(FIND "${TRIPLE_REGION}" "launch(src0_k, dst_k);" LAUNCH_K)
string(FIND "${TRIPLE_REGION}" "launch(src0_v, dst_v);" LAUNCH_V)
if(LAUNCH_Q EQUAL -1 OR LAUNCH_K LESS_EQUAL LAUNCH_Q OR LAUNCH_V LESS_EQUAL LAUNCH_K)
    message(FATAL_ERROR "triple does not preserve Q -> K -> V MMVQ submission order")
endif()
foreach(REQUIRED_TRIPLE_TEXT IN ITEMS
        "ctx.halofpx_mmvq_qkv_q8_conversions_submitted.fetch_add(1"
        "ctx.halofpx_mmvq_qkv_mmvq_submissions.fetch_add(1"
        "ctx.halofpx_mmvq_qkv_triple_dispatches.fetch_add(1")
    require_text(TRIPLE_REGION "${REQUIRED_TRIPLE_TEXT}" "submission evidence counter is missing")
endforeach()

# The ordinary MMVQ path contributes one conversion and one MMVQ count per
# fallback node; the triple contributes one conversion and increments its
# shared launch helper once per Q/K/V consumer. Requiring exactly two lexical
# sites for each metric prevents an uninstrumented 0/0/3/3 fallback.
string(REGEX MATCHALL
    "halofpx_mmvq_qkv_q8_conversions_submitted\\.fetch_add\\(1"
    ALL_CONVERSION_COUNTERS "${MMVQ_SOURCE_TEXT}")
list(LENGTH ALL_CONVERSION_COUNTERS ALL_CONVERSION_COUNTER_COUNT)
if(NOT ALL_CONVERSION_COUNTER_COUNT EQUAL 2)
    message(FATAL_ERROR "ordinary plus triple conversion counters must have exactly two source sites")
endif()
string(REGEX MATCHALL
    "halofpx_mmvq_qkv_mmvq_submissions\\.fetch_add\\(1"
    ALL_MMVQ_COUNTERS "${MMVQ_SOURCE_TEXT}")
list(LENGTH ALL_MMVQ_COUNTERS ALL_MMVQ_COUNTER_COUNT)
if(NOT ALL_MMVQ_COUNTER_COUNT EQUAL 2)
    message(FATAL_ERROR "ordinary plus triple MMVQ counters must have exactly two source sites")
endif()

extract_unique_region(
    CUDA_DISPATCH_TEXT
    "HALOFPX_MMVQ_QKV_Q8_REUSE_METRICS_PROC_BEGIN"
    "HALOFPX_MMVQ_QKV_Q8_REUSE_METRICS_PROC_END"
    METRICS_REGION)
foreach(REQUIRED_METRICS_TEXT IN ITEMS
        "metrics->struct_size != sizeof(halofpx_rocmfpx_mmvq_qkv_q8_reuse_metrics_v1)"
        "metrics->version != HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_METRICS_VERSION"
        "ggml_backend_synchronize(backend);"
        "halofpx_mmvq_qkv_graph_groups_planned.store(0"
        "halofpx_mmvq_qkv_q8_conversions_submitted.store(0"
        "halofpx_mmvq_qkv_mmvq_submissions.store(0")
    require_text(METRICS_REGION "${REQUIRED_METRICS_TEXT}" "versioned metrics contract is incomplete")
endforeach()

extract_unique_region(
    CUDA_DISPATCH_TEXT
    "HALOFPX_MMVQ_QKV_Q8_REUSE_METRICS_REGISTRATION_BEGIN"
    "HALOFPX_MMVQ_QKV_Q8_REUSE_METRICS_REGISTRATION_END"
    METRICS_REGISTRATION_REGION)
require_text(METRICS_REGISTRATION_REGION
    "halofpx_rocmfpx_mmvq_qkv_q8_reuse_metrics_v1"
    "versioned metrics proc is not registered")

foreach(REQUIRED_BACKEND_TEST_TEXT IN ITEMS
        "test_halofpx_rocmfpx_mmvq_qkv_q8_reuse"
        "backend->iface.graph_optimize(backend, graph);"
        "metrics.graph_groups_planned == expected_planned"
        "metrics.triple_dispatches == expected_triples"
        "metrics.q8_conversions_submitted == expected_conversions"
        "metrics.mmvq_submissions == 3"
        "return { q, k, v, out };"
        "GGML_TYPE_Q2_0_ROCMFPX"
        "GGML_TYPE_Q3_0_ROCMFPX"
        "GGML_TYPE_Q6_0_ROCMFPX"
        "GGML_TYPE_Q8_0_ROCMFPX")
    require_text(BACKEND_TEST_TEXT "${REQUIRED_BACKEND_TEST_TEXT}" "backend production-optimizer seam is incomplete")
endforeach()
string(FIND "${BACKEND_TEST_TEXT}" "halofpx_rocmfpx_mmvq_qkv_plan_graph_reorder(graph)" DIRECT_PLANNER_IN_BACKEND_TEST)
if(NOT DIRECT_PLANNER_IN_BACKEND_TEST EQUAL -1)
    message(FATAL_ERROR "backend-op MMVQ QKV case bypasses the registered production graph optimizer")
endif()

foreach(REQUIRED_HOST_NEGATIVE IN ITEMS
        "nested-crossed-output-write"
        "post-plan consumer mutation retained runtime authorization"
        "forged markers authorized an invalid current graph")
    require_text(HOST_TEST_TEXT "${REQUIRED_HOST_NEGATIVE}" "review hardening negative is missing")
endforeach()

message(STATUS "PASS: strict n=1 ROCmFPX MMVQ QKV Q8 reuse default-off/source contract")
