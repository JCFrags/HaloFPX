if(NOT DEFINED SOURCE_ROOT OR SOURCE_ROOT STREQUAL "")
    message(FATAL_ERROR "SOURCE_ROOT is required")
endif()

set(GGML_OPTIONS "${SOURCE_ROOT}/ggml/CMakeLists.txt")
set(HIP_CMAKE "${SOURCE_ROOT}/ggml/src/ggml-hip/CMakeLists.txt")
set(CUDA_DISPATCH "${SOURCE_ROOT}/ggml/src/ggml-cuda/ggml-cuda.cu")
set(MMQ_SOURCE "${SOURCE_ROOT}/ggml/src/ggml-cuda/mmq.cu")

foreach(REQUIRED_FILE IN ITEMS "${GGML_OPTIONS}" "${HIP_CMAKE}" "${CUDA_DISPATCH}" "${MMQ_SOURCE}")
    if(NOT EXISTS "${REQUIRED_FILE}")
        message(FATAL_ERROR "missing source-contract input: ${REQUIRED_FILE}")
    endif()
endforeach()

file(READ "${GGML_OPTIONS}" GGML_OPTIONS_TEXT)
file(READ "${HIP_CMAKE}" HIP_CMAKE_TEXT)
file(READ "${CUDA_DISPATCH}" CUDA_DISPATCH_TEXT)
file(READ "${MMQ_SOURCE}" MMQ_SOURCE_TEXT)

string(FIND "${GGML_OPTIONS_TEXT}"
    "option(GGML_HIP_ROCMFPX_FFN_Q8_REUSE         \"ggml: reuse one Q8_1 activation conversion across dense ROCmFPX FFN MMQ pairs\" OFF)"
    OPTION_DEFAULT_OFF)
if(OPTION_DEFAULT_OFF EQUAL -1)
    message(FATAL_ERROR "ROCmFPX FFN Q8_1 reuse option is missing or is not default-off")
endif()

string(FIND "${HIP_CMAKE_TEXT}"
    "target_compile_definitions(ggml-hip PRIVATE GGML_HIP_ROCMFPX_FFN_Q8_REUSE)"
    HIP_PRIVATE_DEFINITION)
if(HIP_PRIVATE_DEFINITION EQUAL -1)
    message(FATAL_ERROR "reuse macro is not private to the HIP backend target")
endif()

string(FIND "${CUDA_DISPATCH_TEXT}" "// HALOFPX_FFN_Q8_REUSE_DISPATCH_BEGIN" DISPATCH_BEGIN)
string(FIND "${CUDA_DISPATCH_TEXT}" "// HALOFPX_FFN_Q8_REUSE_DISPATCH_END" DISPATCH_END)
if(DISPATCH_BEGIN EQUAL -1 OR DISPATCH_END EQUAL -1 OR DISPATCH_END LESS_EQUAL DISPATCH_BEGIN)
    message(FATAL_ERROR "paired dispatch source markers are missing or out of order")
endif()
math(EXPR DISPATCH_LENGTH "${DISPATCH_END} - ${DISPATCH_BEGIN}")
string(SUBSTRING "${CUDA_DISPATCH_TEXT}" ${DISPATCH_BEGIN} ${DISPATCH_LENGTH} DISPATCH_SOURCE)
foreach(REQUIRED_DISPATCH_TEXT IN ITEMS
        "ggml_cuda_should_reuse_rocmfpx_ffn_q8(*cuda_ctx, cgraph->nodes[i], cgraph->nodes[i + 1], glu)"
        "ggml_cuda_mul_mat_q_rocmfpx_pair("
        "return 1;")
    string(FIND "${DISPATCH_SOURCE}" "${REQUIRED_DISPATCH_TEXT}" DISPATCH_MATCH)
    if(DISPATCH_MATCH EQUAL -1)
        message(FATAL_ERROR "missing paired-dispatch contract text: ${REQUIRED_DISPATCH_TEXT}")
    endif()
endforeach()

string(FIND "${MMQ_SOURCE_TEXT}" "// HALOFPX_FFN_Q8_REUSE_PAIR_BEGIN" PAIR_BEGIN)
string(FIND "${MMQ_SOURCE_TEXT}" "// HALOFPX_FFN_Q8_REUSE_PAIR_END" PAIR_END)
if(PAIR_BEGIN EQUAL -1 OR PAIR_END EQUAL -1 OR PAIR_END LESS_EQUAL PAIR_BEGIN)
    message(FATAL_ERROR "paired MMQ source markers are missing or out of order")
endif()

math(EXPR PAIR_LENGTH "${PAIR_END} - ${PAIR_BEGIN}")
string(SUBSTRING "${MMQ_SOURCE_TEXT}" ${PAIR_BEGIN} ${PAIR_LENGTH} PAIR_SOURCE)
string(REGEX MATCHALL "quantize_mmq_q8_1_cuda\\(" PAIR_QUANTIZE_CALLS "${PAIR_SOURCE}")
list(LENGTH PAIR_QUANTIZE_CALLS PAIR_QUANTIZE_CALL_COUNT)
if(NOT PAIR_QUANTIZE_CALL_COUNT EQUAL 1)
    message(FATAL_ERROR "paired MMQ path must contain exactly one Q8_1 conversion call; found ${PAIR_QUANTIZE_CALL_COUNT}")
endif()

string(REGEX MATCHALL "ggml_cuda_mul_mat_q_switch_type\\(" PAIR_MMQ_CALLS "${PAIR_SOURCE}")
list(LENGTH PAIR_MMQ_CALLS PAIR_MMQ_CALL_COUNT)
if(NOT PAIR_MMQ_CALL_COUNT EQUAL 1)
    message(FATAL_ERROR "paired source must have one shared MMQ launch helper call; found ${PAIR_MMQ_CALL_COUNT}")
endif()

string(FIND "${PAIR_SOURCE}" "launch(src0_a, dst_a);" PAIR_LAUNCH_A)
string(FIND "${PAIR_SOURCE}" "launch(src0_b, dst_b);" PAIR_LAUNCH_B)
if(PAIR_LAUNCH_A EQUAL -1 OR PAIR_LAUNCH_B EQUAL -1 OR PAIR_LAUNCH_B LESS_EQUAL PAIR_LAUNCH_A)
    message(FATAL_ERROR "paired MMQs do not preserve A-then-B graph order")
endif()

message(STATUS "PASS: ROCmFPX FFN Q8_1 reuse default-off/source contract")
