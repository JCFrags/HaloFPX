if(NOT DEFINED SOURCE_ROOT OR SOURCE_ROOT STREQUAL "")
    message(FATAL_ERROR "SOURCE_ROOT is required")
endif()

set(GGML_OPTIONS "${SOURCE_ROOT}/ggml/CMakeLists.txt")
set(HIP_CMAKE "${SOURCE_ROOT}/ggml/src/ggml-hip/CMakeLists.txt")
set(QKV_HEADER "${SOURCE_ROOT}/ggml/src/ggml-cuda/rocmfpx-qkv-q8-reuse.h")
set(CUDA_DISPATCH "${SOURCE_ROOT}/ggml/src/ggml-cuda/ggml-cuda.cu")
set(MMQ_SOURCE "${SOURCE_ROOT}/ggml/src/ggml-cuda/mmq.cu")
set(BACKEND_TEST "${SOURCE_ROOT}/tests/test-backend-ops.cpp")
set(SELECTOR_TEST "${SOURCE_ROOT}/tests/test-halofpx-rocmfpx-qkv-q8-reuse.cpp")
set(TEST_CMAKE "${SOURCE_ROOT}/tests/CMakeLists.txt")
set(CI_WORKFLOW "${SOURCE_ROOT}/.github/workflows/halofpx-ci.yml")

foreach(REQUIRED_FILE IN ITEMS
        "${GGML_OPTIONS}"
        "${HIP_CMAKE}"
        "${QKV_HEADER}"
        "${CUDA_DISPATCH}"
        "${MMQ_SOURCE}"
        "${BACKEND_TEST}"
        "${SELECTOR_TEST}"
        "${TEST_CMAKE}"
        "${CI_WORKFLOW}")
    if(NOT EXISTS "${REQUIRED_FILE}")
        message(FATAL_ERROR "missing QKV source-contract input: ${REQUIRED_FILE}")
    endif()
endforeach()

file(READ "${GGML_OPTIONS}" GGML_OPTIONS_TEXT)
file(READ "${HIP_CMAKE}" HIP_CMAKE_TEXT)
file(READ "${QKV_HEADER}" QKV_HEADER_TEXT)
file(READ "${CUDA_DISPATCH}" CUDA_DISPATCH_TEXT)
file(READ "${MMQ_SOURCE}" MMQ_SOURCE_TEXT)
file(READ "${BACKEND_TEST}" BACKEND_TEST_TEXT)
file(READ "${SELECTOR_TEST}" SELECTOR_TEST_TEXT)
file(READ "${TEST_CMAKE}" TEST_CMAKE_TEXT)
file(READ "${CI_WORKFLOW}" CI_WORKFLOW_TEXT)

function(require_text SOURCE_VARIABLE REQUIRED_TEXT FAILURE_TEXT)
    string(FIND "${${SOURCE_VARIABLE}}" "${REQUIRED_TEXT}" MATCH_INDEX)
    if(MATCH_INDEX EQUAL -1)
        message(FATAL_ERROR "${FAILURE_TEXT}: ${REQUIRED_TEXT}")
    endif()
endfunction()

function(extract_unique_region SOURCE_VARIABLE BEGIN_MARKER END_MARKER OUTPUT_VARIABLE)
    set(SOURCE_TEXT "${${SOURCE_VARIABLE}}")
    string(REGEX MATCHALL "${BEGIN_MARKER}" BEGIN_MATCHES "${SOURCE_TEXT}")
    string(REGEX MATCHALL "${END_MARKER}" END_MATCHES "${SOURCE_TEXT}")
    list(LENGTH BEGIN_MATCHES BEGIN_COUNT)
    list(LENGTH END_MATCHES END_COUNT)
    if(NOT BEGIN_COUNT EQUAL 1 OR NOT END_COUNT EQUAL 1)
        message(FATAL_ERROR
            "QKV source markers must be unique: ${BEGIN_MARKER}=${BEGIN_COUNT}, ${END_MARKER}=${END_COUNT}")
    endif()
    string(FIND "${SOURCE_TEXT}" "${BEGIN_MARKER}" BEGIN_INDEX)
    string(FIND "${SOURCE_TEXT}" "${END_MARKER}" END_INDEX)
    if(BEGIN_INDEX EQUAL -1 OR END_INDEX EQUAL -1 OR END_INDEX LESS_EQUAL BEGIN_INDEX)
        message(FATAL_ERROR "QKV source markers are missing or reversed: ${BEGIN_MARKER}")
    endif()
    string(LENGTH "${END_MARKER}" END_MARKER_LENGTH)
    math(EXPR REGION_LENGTH "${END_INDEX} - ${BEGIN_INDEX} + ${END_MARKER_LENGTH}")
    string(SUBSTRING "${SOURCE_TEXT}" ${BEGIN_INDEX} ${REGION_LENGTH} REGION_TEXT)
    set(${OUTPUT_VARIABLE} "${REGION_TEXT}" PARENT_SCOPE)
endfunction()

require_text(GGML_OPTIONS_TEXT
    "option(GGML_HIP_ROCMFPX_QKV_Q8_REUSE         \"ggml: reuse one Q8_1 activation conversion across separate ROCmFPX Q/K/V prompt MMQs on gfx1151\" OFF)"
    "QKV reuse option is missing or is not default-off")
require_text(GGML_OPTIONS_TEXT
    "if (GGML_HIP_ROCMFPX_QKV_Q8_REUSE AND NOT GGML_HIP)"
    "non-HIP QKV option refusal is missing")
require_text(HIP_CMAKE_TEXT
    "target_compile_definitions(ggml-hip PRIVATE GGML_HIP_ROCMFPX_QKV_Q8_REUSE)"
    "QKV reuse macro is not private to the HIP backend")

foreach(REQUIRED_HEADER_TEXT IN ITEMS
        "GGML_TYPE_Q2_0_ROCMFPX"
        "GGML_TYPE_Q3_0_ROCMFPX"
        "GGML_TYPE_Q6_0_ROCMFPX"
        "GGML_TYPE_Q8_0_ROCMFPX"
        "contract.activation_columns > 8"
        "contract.gfx1151_hip"
        "contract.no_fused_wqkv"
        "contract.no_lora"
        "contract.no_bias_or_clamp"
        "contract.exact_shared_activation"
        "contract.local_non_split"
        "contract.safe_allocation_views"
        "contract.single_stream"
        "Qcur-"
        "Kcur-"
        "Vcur-"
        "blk.%d.%s.weight"
        "consumer->op != GGML_OP_RESHAPE"
        "halofpx_rocmfpx_qkv_crossed_writes_are_safe"
        "ggml_op_is_empty(crossed->op)"
        "crossed->view_src"
        "halofpx_rocmfpx_qkv_plan_graph_reorder"
        "HALOFPX_ROCMFPX_QKV_Q8_REUSE_GRAPH_MAGIC")
    require_text(QKV_HEADER_TEXT "${REQUIRED_HEADER_TEXT}" "missing host selector/reorder contract")
endforeach()

extract_unique_region(
    CUDA_DISPATCH_TEXT
    "HALOFPX_QKV_Q8_REUSE_REORDER_BEGIN"
    "HALOFPX_QKV_Q8_REUSE_REORDER_END"
    REORDER_REGION)
foreach(REQUIRED_REORDER_TEXT IN ITEMS
        "halofpx_rocmfpx_qkv_dispatch_graph_reorder"
        "qkv_reorder.eligible_groups != 0"
        "halofpx_qkv_graph_groups_planned.fetch_add"
        "cuda_ctx->stream_context().reset();"
        "return;")
    require_text(REORDER_REGION "${REQUIRED_REORDER_TEXT}" "missing exact pre-allocation reorder behavior")
endforeach()

extract_unique_region(
    CUDA_DISPATCH_TEXT
    "HALOFPX_QKV_Q8_REUSE_DISPATCH_BEGIN"
    "HALOFPX_QKV_Q8_REUSE_DISPATCH_END"
    DISPATCH_REGION)
string(FIND "${DISPATCH_REGION}" "ggml_cuda_should_reuse_rocmfpx_qkv_q8" MATCHER_INDEX)
string(FIND "${DISPATCH_REGION}" "ggml_cuda_mul_mat_q_rocmfpx_triple" TRIPLE_INDEX)
string(FIND "${DISPATCH_REGION}" "return 2;" RETURN_INDEX)
if(MATCHER_INDEX EQUAL -1 OR TRIPLE_INDEX LESS_EQUAL MATCHER_INDEX OR RETURN_INDEX LESS_EQUAL TRIPLE_INDEX)
    message(FATAL_ERROR "exact QKV matcher -> triple -> skip-two dispatch was removed or reordered")
endif()

extract_unique_region(
    MMQ_SOURCE_TEXT
    "HALOFPX_QKV_Q8_REUSE_TRIPLE_BEGIN"
    "HALOFPX_QKV_Q8_REUSE_TRIPLE_END"
    TRIPLE_REGION)
string(REGEX MATCHALL "quantize_mmq_q8_1_cuda\\(" TRIPLE_QUANTIZE_CALLS "${TRIPLE_REGION}")
list(LENGTH TRIPLE_QUANTIZE_CALLS TRIPLE_QUANTIZE_CALL_COUNT)
if(NOT TRIPLE_QUANTIZE_CALL_COUNT EQUAL 1)
    message(FATAL_ERROR "QKV triple must contain exactly one Q8_1 conversion; found ${TRIPLE_QUANTIZE_CALL_COUNT}")
endif()
string(REGEX MATCHALL "ggml_cuda_mul_mat_q_switch_type\\(" TRIPLE_MMQ_CALLS "${TRIPLE_REGION}")
list(LENGTH TRIPLE_MMQ_CALLS TRIPLE_MMQ_CALL_COUNT)
if(NOT TRIPLE_MMQ_CALL_COUNT EQUAL 1)
    message(FATAL_ERROR "QKV triple must contain one shared launch callsite; found ${TRIPLE_MMQ_CALL_COUNT}")
endif()
string(FIND "${TRIPLE_REGION}" "launch(src0_q, dst_q);" LAUNCH_Q)
string(FIND "${TRIPLE_REGION}" "launch(src0_k, dst_k);" LAUNCH_K)
string(FIND "${TRIPLE_REGION}" "launch(src0_v, dst_v);" LAUNCH_V)
if(LAUNCH_Q EQUAL -1 OR LAUNCH_K LESS_EQUAL LAUNCH_Q OR LAUNCH_V LESS_EQUAL LAUNCH_K)
    message(FATAL_ERROR "QKV triple does not preserve ordered Q -> K -> V MMQ submission")
endif()
foreach(REQUIRED_TRIPLE_TEXT IN ITEMS
        "ctx.halofpx_qkv_q8_conversions_submitted.fetch_add(1"
        "ctx.halofpx_qkv_mmq_submissions.fetch_add(1"
        "ctx.halofpx_qkv_triple_dispatches.fetch_add(1")
    require_text(TRIPLE_REGION "${REQUIRED_TRIPLE_TEXT}" "QKV runtime submission counter is missing")
endforeach()

extract_unique_region(
    CUDA_DISPATCH_TEXT
    "HALOFPX_QKV_Q8_REUSE_METRICS_PROC_BEGIN"
    "HALOFPX_QKV_Q8_REUSE_METRICS_PROC_END"
    METRICS_PROC_REGION)
foreach(REQUIRED_METRICS_TEXT IN ITEMS
        "metrics->struct_size != sizeof(halofpx_rocmfpx_qkv_q8_reuse_metrics_v1)"
        "metrics->version != HALOFPX_ROCMFPX_QKV_Q8_REUSE_METRICS_VERSION"
        "ggml_backend_dev_backend_reg(device) != ggml_backend_cuda_reg()"
        "ggml_backend_synchronize(backend);"
        "halofpx_qkv_triple_dispatches.store(0"
        "halofpx_qkv_q8_conversions_submitted.store(0"
        "halofpx_qkv_mmq_submissions.store(0")
    require_text(METRICS_PROC_REGION "${REQUIRED_METRICS_TEXT}" "metrics proc contract is incomplete")
endforeach()

extract_unique_region(
    CUDA_DISPATCH_TEXT
    "HALOFPX_QKV_Q8_REUSE_METRICS_REGISTRATION_BEGIN"
    "HALOFPX_QKV_Q8_REUSE_METRICS_REGISTRATION_END"
    METRICS_REGISTRATION_REGION)
require_text(METRICS_REGISTRATION_REGION
    "halofpx_rocmfpx_qkv_q8_reuse_metrics_v1"
    "versioned QKV metrics proc is not registered")

foreach(REQUIRED_BACKEND_TEST_TEXT IN ITEMS
        "test_halofpx_rocmfpx_qkv_q8_reuse"
        "HALOFPX_ROCMFPX_QKV_Q8_REUSE"
        "prepare_graph_before_allocation"
        "backend->iface.graph_optimize(backend, graph);"
        "expected_planned = distinct_v_activation ? 0 : 1"
        "before_backend_compare"
        "after_backend_compare"
        "metrics.triple_dispatches == expected_triples"
        "metrics.q8_conversions_submitted == expected_conversions"
        "metrics.mmq_submissions == 3"
        "return { q, k, v, out };"
        "GGML_TYPE_Q2_0_ROCMFPX"
        "GGML_TYPE_Q3_0_ROCMFPX"
        "GGML_TYPE_Q6_0_ROCMFPX"
        "GGML_TYPE_Q8_0_ROCMFPX")
    require_text(BACKEND_TEST_TEXT "${REQUIRED_BACKEND_TEST_TEXT}" "backend-op QKV runtime contract is incomplete")
endforeach()
string(FIND "${BACKEND_TEST_TEXT}" "halofpx_rocmfpx_qkv_plan_graph_reorder(graph)" DIRECT_PLANNER_IN_BACKEND_TEST)
if(NOT DIRECT_PLANNER_IN_BACKEND_TEST EQUAL -1)
    message(FATAL_ERROR "backend-op QKV case bypasses the registered production graph optimizer")
endif()
foreach(REQUIRED_ALIAS_TEST_TEXT IN ITEMS
        "crossed-activation-inplace-write"
        "crossed-weight-copy-write"
        "crossed_activation_metadata"
        "crossed_q_output_write")
    require_text(SELECTOR_TEST_TEXT "${REQUIRED_ALIAS_TEST_TEXT}" "QKV alias crossing test is missing")
endforeach()
require_text(TEST_CMAKE_TEXT
    "test-halofpx-rocmfpx-qkv-q8-reuse-\${HALOFPX_QKV_Q8_REUSE_MODE}"
    "QKV OFF/ON host selector targets are missing")
require_text(TEST_CMAKE_TEXT
    "target_include_directories(test-backend-ops PRIVATE"
    "test-backend-ops does not compile the QKV graph case")
extract_unique_region(
    CI_WORKFLOW_TEXT
    "HALOFPX_QKV_Q8_REUSE_CI_BEGIN"
    "HALOFPX_QKV_Q8_REUSE_CI_END"
    CI_REGION)
require_text(CI_REGION
    "rocmfpx-qkv-q8-reuse-contract:"
    "focused QKV host CI job is missing")
require_text(CI_REGION
    "test-backend-ops"
    "focused QKV CI does not compile the backend-op graph case")
foreach(REQUIRED_HIP_CI_TEXT IN ITEMS
        "rocmfpx-qkv-q8-reuse-hip-compile:"
        "sha256:bdc8e61026cbb844ede93d44d2c50055f51ebb2041906b60182bf3bee3139054"
        "-DGPU_TARGETS=gfx1151"
        "-DGGML_HIP_ROCMFPX_QKV_Q8_REUSE=ON"
        "--target ggml-hip"
        "--offload-arch=gfx1151")
    require_text(CI_REGION "${REQUIRED_HIP_CI_TEXT}" "off-target gfx1151 HIP compile contract is incomplete")
endforeach()

message(STATUS "PASS: ROCmFPX QKV Q8_1 reuse default-off/source/runtime-test contract")
