if(NOT DEFINED SOURCE_ROOT OR SOURCE_ROOT STREQUAL "")
    message(FATAL_ERROR "SOURCE_ROOT is required")
endif()

set(GGML_OPTIONS "${SOURCE_ROOT}/ggml/CMakeLists.txt")
set(HIP_CMAKE "${SOURCE_ROOT}/ggml/src/ggml-hip/CMakeLists.txt")
set(MOE_HEADER "${SOURCE_ROOT}/ggml/src/ggml-cuda/rocmfpx-moe-q8-reuse.h")
set(MMVQ_QKV_HEADER "${SOURCE_ROOT}/ggml/src/ggml-cuda/rocmfpx-mmvq-qkv-q8-reuse.h")
set(PROMPT_QKV_HEADER "${SOURCE_ROOT}/ggml/src/ggml-cuda/rocmfpx-qkv-q8-reuse.h")
set(CUDA_DISPATCH "${SOURCE_ROOT}/ggml/src/ggml-cuda/ggml-cuda.cu")
set(MMQ_SOURCE "${SOURCE_ROOT}/ggml/src/ggml-cuda/mmq.cu")
set(BACKEND_TEST "${SOURCE_ROOT}/tests/test-backend-ops.cpp")
set(SELECTOR_TEST "${SOURCE_ROOT}/tests/test-halofpx-rocmfpx-moe-q8-reuse.cpp")
set(COMPOSITION_TEST "${SOURCE_ROOT}/tests/test-halofpx-rocmfpx-qkv-moe-composition.cpp")
set(TEST_CMAKE "${SOURCE_ROOT}/tests/CMakeLists.txt")
set(CI_WORKFLOW "${SOURCE_ROOT}/.github/workflows/halofpx-ci.yml")

foreach(REQUIRED_FILE IN ITEMS
        "${GGML_OPTIONS}"
        "${HIP_CMAKE}"
        "${MOE_HEADER}"
        "${MMVQ_QKV_HEADER}"
        "${PROMPT_QKV_HEADER}"
        "${CUDA_DISPATCH}"
        "${MMQ_SOURCE}"
        "${BACKEND_TEST}"
        "${SELECTOR_TEST}"
        "${COMPOSITION_TEST}"
        "${TEST_CMAKE}"
        "${CI_WORKFLOW}")
    if(NOT EXISTS "${REQUIRED_FILE}")
        message(FATAL_ERROR "missing routed-MoE source-contract input: ${REQUIRED_FILE}")
    endif()
endforeach()

file(READ "${GGML_OPTIONS}" GGML_OPTIONS_TEXT)
file(READ "${HIP_CMAKE}" HIP_CMAKE_TEXT)
file(READ "${MOE_HEADER}" MOE_HEADER_TEXT)
file(READ "${MMVQ_QKV_HEADER}" MMVQ_QKV_HEADER_TEXT)
file(READ "${PROMPT_QKV_HEADER}" PROMPT_QKV_HEADER_TEXT)
file(READ "${CUDA_DISPATCH}" CUDA_DISPATCH_TEXT)
file(READ "${MMQ_SOURCE}" MMQ_SOURCE_TEXT)
file(READ "${BACKEND_TEST}" BACKEND_TEST_TEXT)
file(READ "${SELECTOR_TEST}" SELECTOR_TEST_TEXT)
file(READ "${COMPOSITION_TEST}" COMPOSITION_TEST_TEXT)
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
            "routed-MoE source markers must be unique: ${BEGIN_MARKER}=${BEGIN_COUNT}, ${END_MARKER}=${END_COUNT}")
    endif()
    string(FIND "${SOURCE_TEXT}" "${BEGIN_MARKER}" BEGIN_INDEX)
    string(FIND "${SOURCE_TEXT}" "${END_MARKER}" END_INDEX)
    if(BEGIN_INDEX EQUAL -1 OR END_INDEX EQUAL -1 OR END_INDEX LESS_EQUAL BEGIN_INDEX)
        message(FATAL_ERROR "routed-MoE source markers are missing or reversed: ${BEGIN_MARKER}")
    endif()
    string(LENGTH "${END_MARKER}" END_MARKER_LENGTH)
    math(EXPR REGION_LENGTH "${END_INDEX} - ${BEGIN_INDEX} + ${END_MARKER_LENGTH}")
    string(SUBSTRING "${SOURCE_TEXT}" ${BEGIN_INDEX} ${REGION_LENGTH} REGION_TEXT)
    set(${OUTPUT_VARIABLE} "${REGION_TEXT}" PARENT_SCOPE)
endfunction()

require_text(GGML_OPTIONS_TEXT
    "option(GGML_HIP_ROCMFPX_MOE_Q8_REUSE         \"ggml: reuse routed-ID preparation and one Q8_1 activation conversion across ROCmFPX MoE gate/up MMQs on gfx1151\" OFF)"
    "routed-MoE reuse option is missing or is not default-off")
require_text(GGML_OPTIONS_TEXT
    "if (GGML_HIP_ROCMFPX_MOE_Q8_REUSE AND NOT GGML_HIP)"
    "non-HIP routed-MoE option refusal is missing")
require_text(HIP_CMAKE_TEXT
    "target_compile_definitions(ggml-hip PRIVATE GGML_HIP_ROCMFPX_MOE_Q8_REUSE)"
    "routed-MoE reuse macro is not private to the HIP backend")

foreach(REQUIRED_HEADER_TEXT IN ITEMS
        "GGML_TYPE_Q2_0_ROCMFPX"
        "GGML_TYPE_Q3_0_ROCMFPX"
        "GGML_TYPE_Q6_0_ROCMFPX"
        "GGML_TYPE_Q8_0_ROCMFPX"
        "contract.gfx1151_hip"
        "contract.routed_moe_pair"
        "contract.no_bias_glu_pair"
        "contract.exact_shared_activation"
        "contract.exact_shared_ids"
        "contract.valid_routing_layout"
        "contract.valid_index_geometry"
        "contract.local_non_split"
        "contract.both_mmq_paths"
        "contract.single_stream")
    require_text(MOE_HEADER_TEXT "${REQUIRED_HEADER_TEXT}" "missing routed-MoE selector contract")
endforeach()

# The two QKV optimizers own disjoint op-param marker slots. Routed MoE does
# not rewrite or mark a graph, so it must not consume either range.
require_text(MMVQ_QKV_HEADER_TEXT
    "HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_MAGIC_PARAM = 12"
    "decode QKV marker ownership changed")
require_text(MMVQ_QKV_HEADER_TEXT
    "HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_ROLE_PARAM  = 13"
    "decode QKV role-marker ownership changed")
require_text(PROMPT_QKV_HEADER_TEXT
    "HALOFPX_ROCMFPX_QKV_Q8_REUSE_MAGIC_PARAM = 14"
    "prompt QKV marker ownership changed")
require_text(PROMPT_QKV_HEADER_TEXT
    "HALOFPX_ROCMFPX_QKV_Q8_REUSE_ROLE_PARAM = 15"
    "prompt QKV role-marker ownership changed")
foreach(FORBIDDEN_MOE_MARKER IN ITEMS "MAGIC_PARAM" "ROLE_PARAM" "ggml_set_op_params")
    string(FIND "${MOE_HEADER_TEXT}" "${FORBIDDEN_MOE_MARKER}" FORBIDDEN_MOE_MARKER_OFFSET)
    if(NOT FORBIDDEN_MOE_MARKER_OFFSET EQUAL -1)
        message(FATAL_ERROR "routed-MoE feature must not own graph marker state: ${FORBIDDEN_MOE_MARKER}")
    endif()
endforeach()

extract_unique_region(
    CUDA_DISPATCH_TEXT
    "HALOFPX_MOE_Q8_REUSE_SELECTOR_BEGIN"
    "HALOFPX_MOE_Q8_REUSE_SELECTOR_END"
    SELECTOR_REGION)

extract_unique_region(
    CUDA_DISPATCH_TEXT
    "HALOFPX_MOE_Q8_REUSE_DISPATCH_BEGIN"
    "HALOFPX_MOE_Q8_REUSE_DISPATCH_END"
    DISPATCH_REGION)
foreach(REQUIRED_DISPATCH_TEXT IN ITEMS
        "op == GGML_OP_MUL_MAT_ID"
        "ggml_cuda_should_reuse_rocmfpx_moe_q8("
        "ggml_cuda_mul_mat_id_q_rocmfpx_pair("
        "cgraph->nodes[i]->src[2]"
        "return 1;")
    require_text(DISPATCH_REGION "${REQUIRED_DISPATCH_TEXT}" "routed-MoE dispatch contract is incomplete")
endforeach()

# Decode gets the first strict n=1 opportunity, prompt QKV gets the next graph
# group fallback, and routed MoE stays later in the ordinary fusion scan. This
# preserves the merged features' early returns and avoids an extra graph scan.
string(FIND "${CUDA_DISPATCH_TEXT}" "HALOFPX_MMVQ_QKV_Q8_REUSE_OPTIMIZER_BEGIN" DECODE_OPTIMIZER_OFFSET)
string(FIND "${CUDA_DISPATCH_TEXT}" "HALOFPX_QKV_Q8_REUSE_REORDER_BEGIN" PROMPT_OPTIMIZER_OFFSET)
string(FIND "${CUDA_DISPATCH_TEXT}" "HALOFPX_MOE_Q8_REUSE_OPTIMIZER" MOE_OPTIMIZER_OFFSET)
if(DECODE_OPTIMIZER_OFFSET EQUAL -1 OR PROMPT_OPTIMIZER_OFFSET EQUAL -1 OR
        NOT DECODE_OPTIMIZER_OFFSET LESS PROMPT_OPTIMIZER_OFFSET)
    message(FATAL_ERROR "decode QKV optimizer must precede the prompt QKV graph scan")
endif()
if(NOT MOE_OPTIMIZER_OFFSET EQUAL -1)
    message(FATAL_ERROR "routed-MoE reuse must not add a graph optimizer scan")
endif()

string(FIND "${CUDA_DISPATCH_TEXT}" "HALOFPX_MMVQ_QKV_Q8_REUSE_DISPATCH_BEGIN" DECODE_DISPATCH_OFFSET)
string(FIND "${CUDA_DISPATCH_TEXT}" "HALOFPX_QKV_Q8_REUSE_DISPATCH_BEGIN" PROMPT_DISPATCH_OFFSET)
string(FIND "${CUDA_DISPATCH_TEXT}" "HALOFPX_MOE_Q8_REUSE_DISPATCH_BEGIN" MOE_DISPATCH_OFFSET)
if(DECODE_DISPATCH_OFFSET EQUAL -1 OR PROMPT_DISPATCH_OFFSET EQUAL -1 OR MOE_DISPATCH_OFFSET EQUAL -1 OR
        NOT DECODE_DISPATCH_OFFSET LESS PROMPT_DISPATCH_OFFSET OR
        NOT PROMPT_DISPATCH_OFFSET LESS MOE_DISPATCH_OFFSET)
    message(FATAL_ERROR "runtime dispatch order must remain decode QKV -> prompt QKV -> routed MoE")
endif()
string(FIND "${DISPATCH_REGION}" "stream_context().reset()" MOE_STREAM_RESET_OFFSET)
if(NOT MOE_STREAM_RESET_OFFSET EQUAL -1)
    message(FATAL_ERROR "routed-MoE dispatch must not clobber optimizer stream ownership")
endif()

extract_unique_region(
    MMQ_SOURCE_TEXT
    "HALOFPX_MOE_Q8_REUSE_PAIR_BEGIN"
    "HALOFPX_MOE_Q8_REUSE_PAIR_END"
    PAIR_REGION)
string(REGEX MATCHALL "ggml_cuda_launch_mm_ids_helper\\(" PAIR_ID_HELPERS "${PAIR_REGION}")
list(LENGTH PAIR_ID_HELPERS PAIR_ID_HELPER_COUNT)
if(NOT PAIR_ID_HELPER_COUNT EQUAL 1)
    message(FATAL_ERROR "routed-MoE pair must contain exactly one ID helper; found ${PAIR_ID_HELPER_COUNT}")
endif()
string(REGEX MATCHALL "quantize_mmq_q8_1_cuda\\(" PAIR_QUANTIZE_CALLS "${PAIR_REGION}")
list(LENGTH PAIR_QUANTIZE_CALLS PAIR_QUANTIZE_CALL_COUNT)
if(NOT PAIR_QUANTIZE_CALL_COUNT EQUAL 1)
    message(FATAL_ERROR "routed-MoE pair must contain exactly one Q8_1 conversion; found ${PAIR_QUANTIZE_CALL_COUNT}")
endif()
string(REGEX MATCHALL "ggml_cuda_mul_mat_q_switch_type\\(" PAIR_MMQ_CALLS "${PAIR_REGION}")
list(LENGTH PAIR_MMQ_CALLS PAIR_MMQ_CALL_COUNT)
if(NOT PAIR_MMQ_CALL_COUNT EQUAL 1)
    message(FATAL_ERROR "routed-MoE pair must have one shared MMQ launch callsite; found ${PAIR_MMQ_CALL_COUNT}")
endif()
string(FIND "${PAIR_REGION}" "launch(src0_a, dst_a);" LAUNCH_A)
string(FIND "${PAIR_REGION}" "launch(src0_b, dst_b);" LAUNCH_B)
if(LAUNCH_A EQUAL -1 OR LAUNCH_B LESS_EQUAL LAUNCH_A)
    message(FATAL_ERROR "routed-MoE pair does not preserve A-then-B graph order")
endif()
foreach(REQUIRED_PAIR_TEXT IN ITEMS
        "ctx.halofpx_moe_ids_helper_submissions.fetch_add(1"
        "ctx.halofpx_moe_q8_conversions_submitted.fetch_add(1"
        "ctx.halofpx_moe_mmq_submissions.fetch_add(1"
        "ctx.halofpx_moe_pair_dispatches.fetch_add(1")
    require_text(PAIR_REGION "${REQUIRED_PAIR_TEXT}" "routed-MoE runtime counter is missing")
endforeach()

extract_unique_region(
    MMQ_SOURCE_TEXT
    "HALOFPX_MOE_Q8_REUSE_LEGACY_ID_METRICS_BEGIN"
    "HALOFPX_MOE_Q8_REUSE_LEGACY_ID_METRICS_END"
    LEGACY_ID_METRICS_REGION)
foreach(REQUIRED_LEGACY_COUNTER_TEXT IN ITEMS
        "halofpx_rocmfpx_moe_q8_reuse_type(src0->type)"
        "halofpx_moe_ids_helper_submissions.fetch_add(1"
        "halofpx_moe_q8_conversions_submitted.fetch_add(1"
        "halofpx_moe_mmq_submissions.fetch_add(1")
    require_text(LEGACY_ID_METRICS_REGION "${REQUIRED_LEGACY_COUNTER_TEXT}"
        "legacy routed-MMQ fallback counter seam is incomplete")
endforeach()

extract_unique_region(
    CUDA_DISPATCH_TEXT
    "HALOFPX_MOE_Q8_REUSE_METRICS_PROC_BEGIN"
    "HALOFPX_MOE_Q8_REUSE_METRICS_PROC_END"
    METRICS_REGION)
foreach(REQUIRED_METRICS_TEXT IN ITEMS
        "metrics->struct_size != sizeof(halofpx_rocmfpx_moe_q8_reuse_metrics_v1)"
        "metrics->version != HALOFPX_ROCMFPX_MOE_Q8_REUSE_METRICS_VERSION"
        "ggml_backend_synchronize(backend);"
        "halofpx_moe_pair_dispatches.store(0"
        "halofpx_moe_ids_helper_submissions.store(0"
        "halofpx_moe_q8_conversions_submitted.store(0"
        "halofpx_moe_mmq_submissions.store(0")
    require_text(METRICS_REGION "${REQUIRED_METRICS_TEXT}" "routed-MoE metrics proc is incomplete")
endforeach()

# Marker ownership is a whole-feature invariant, not just a header rule. Scan
# every owned selector/dispatch/paired/legacy-metrics/metrics region so a later
# implementation edit cannot silently consume either QKV optimizer's slots.
foreach(MOE_REGION_VARIABLE IN ITEMS
        SELECTOR_REGION
        DISPATCH_REGION
        PAIR_REGION
        LEGACY_ID_METRICS_REGION
        METRICS_REGION)
    foreach(FORBIDDEN_MOE_MARKER IN ITEMS "MAGIC_PARAM" "ROLE_PARAM" "ggml_set_op_params" "op_params[")
        string(FIND "${${MOE_REGION_VARIABLE}}" "${FORBIDDEN_MOE_MARKER}" FORBIDDEN_MOE_MARKER_OFFSET)
        if(NOT FORBIDDEN_MOE_MARKER_OFFSET EQUAL -1)
            message(FATAL_ERROR
                "routed-MoE owned region must not consume graph marker state: ${MOE_REGION_VARIABLE}/${FORBIDDEN_MOE_MARKER}")
        endif()
    endforeach()
endforeach()

foreach(REQUIRED_BACKEND_TEST_TEXT IN ITEMS
        "test_halofpx_rocmfpx_moe_q8_reuse"
        "ROCMFPX_MOE_Q8_REUSE"
        "moe-ids-a-padded-view"
        "n_experts - 1"
        "distinct_ids"
        "with_bias"
        "metrics.pair_dispatches == expected_pairs"
        "metrics.ids_helper_submissions == expected_shared_submissions"
        "metrics.q8_conversions_submitted == expected_shared_submissions"
        "metrics.mmq_submissions == expected_mmqs"
        "return { gate_projection, up_projection, out };")
    require_text(BACKEND_TEST_TEXT "${REQUIRED_BACKEND_TEST_TEXT}" "routed-MoE backend correctness case is incomplete")
endforeach()
foreach(REQUIRED_SELECTOR_TEST_TEXT IN ITEMS
        "exact_shared_ids"
        "valid_routing_layout"
        "valid_index_geometry"
        "routed_moe_pair"
        "no_bias_glu_pair")
    require_text(SELECTOR_TEST_TEXT "${REQUIRED_SELECTOR_TEST_TEXT}" "independent routed-MoE negative is missing")
endforeach()
require_text(TEST_CMAKE_TEXT
    "test-halofpx-rocmfpx-moe-q8-reuse-\${HALOFPX_MOE_Q8_REUSE_MODE}"
    "routed-MoE OFF/ON host targets are missing")
require_text(TEST_CMAKE_TEXT
    "test-halofpx-rocmfpx-qkv-moe-composition-matrix"
    "QKV/routed-MoE executable composition matrix target is missing")
foreach(REQUIRED_MATRIX_TEXT IN ITEMS
        "foreach(HALOFPX_PROMPT_MODE IN ITEMS 0 1)"
        "foreach(HALOFPX_DECODE_MODE IN ITEMS 0 1)"
        "foreach(HALOFPX_MOE_MODE IN ITEMS 0 1)"
        "composition-p\${HALOFPX_PROMPT_MODE}-d\${HALOFPX_DECODE_MODE}-m\${HALOFPX_MOE_MODE}")
    require_text(TEST_CMAKE_TEXT "${REQUIRED_MATRIX_TEXT}"
        "QKV/routed-MoE composition matrix does not enumerate all eight states")
endforeach()
foreach(REQUIRED_COMPOSITION_CMAKE_TEXT IN ITEMS
        "GGML_HIP_ROCMFPX_QKV_Q8_REUSE"
        "GGML_HIP_ROCMFPX_MMVQ_QKV_Q8_REUSE"
        "GGML_HIP_ROCMFPX_MOE_Q8_REUSE"
        "HALOFPX_EXPECT_PROMPT_QKV"
        "HALOFPX_EXPECT_DECODE_QKV"
        "HALOFPX_EXPECT_ROUTED_MOE")
    require_text(TEST_CMAKE_TEXT "${REQUIRED_COMPOSITION_CMAKE_TEXT}"
        "QKV/routed-MoE composition compile definition is missing")
endforeach()
foreach(REQUIRED_COMPOSITION_TEST_TEXT IN ITEMS
        "test_decode_then_moe"
        "test_decode_fallback_then_prompt_then_moe"
        "moe_group_preserved"
        "topological")
    require_text(COMPOSITION_TEST_TEXT "${REQUIRED_COMPOSITION_TEST_TEXT}"
        "QKV/routed-MoE executable composition contract is incomplete")
endforeach()

extract_unique_region(
    CI_WORKFLOW_TEXT
    "HALOFPX_MOE_Q8_REUSE_CI_BEGIN"
    "HALOFPX_MOE_Q8_REUSE_CI_END"
    CI_REGION)
foreach(REQUIRED_CI_TEXT IN ITEMS
        "rocmfpx-moe-q8-reuse-contract:"
        "test-halofpx-rocmfpx-moe-q8-reuse-off"
        "test-halofpx-rocmfpx-moe-q8-reuse-on"
        "test-halofpx-rocmfpx-qkv-moe-composition-matrix"
        "test-backend-ops")
    require_text(CI_REGION "${REQUIRED_CI_TEXT}" "routed-MoE CI contract is incomplete")
endforeach()

foreach(REQUIRED_COMPOSE_CI_TEXT IN ITEMS
        "rocmfpx-qkv-reuse-composition-hip-compile:"
        "sha256:bdc8e61026cbb844ede93d44d2c50055f51ebb2041906b60182bf3bee3139054"
        "-DGPU_TARGETS=gfx1151"
        "-DGGML_HIP_ROCMFPX_QKV_Q8_REUSE=\"\${option}\""
        "-DGGML_HIP_ROCMFPX_MMVQ_QKV_Q8_REUSE=\"\${option}\""
        "-DGGML_HIP_ROCMFPX_MOE_Q8_REUSE=\"\${option}\""
        "-DGGML_HIP_ROCMFPX_MOE_Q8_REUSE"
        "/ggml/src/ggml-cuda/ggml-cuda.cu"
        "/ggml/src/ggml-cuda/mmq.cu"
        "/ggml/src/ggml-cuda/mmvq.cu"
        "--target ggml-hip"
        "--offload-arch=gfx1151")
    require_text(CI_WORKFLOW_TEXT "${REQUIRED_COMPOSE_CI_TEXT}"
        "three-feature gfx1151 composition compile contract is incomplete")
endforeach()

message(STATUS "PASS: ROCmFPX routed-MoE Q8_1 reuse default-off/source/runtime-test contract")
