if(NOT DEFINED SOURCE_ROOT OR NOT DEFINED BINARY_ROOT OR NOT DEFINED GENERATOR)
  message(FATAL_ERROR "SOURCE_ROOT, BINARY_ROOT, and GENERATOR are required")
endif()

set(GATE "${SOURCE_ROOT}/cmake/halofpx-registry-lab-linux-gates.cmake")
function(run_gate NAME EXPECT_SUCCESS SYSTEM PREINIT INITIALIZER MUTATION EXPECT_TEXT)
  execute_process(
    COMMAND "${CMAKE_COMMAND}"
      -DCMAKE_SYSTEM_NAME=${SYSTEM}
      -DHALOFPX_REGISTRY_LAB_LINUX_PREINIT=${PREINIT}
      -DHALOFPX_REGISTRY_LAB_LINUX_INITIALIZER=${INITIALIZER}
      -DHALOFPX_REGISTRY_LAB_LINUX_MUTATION=${MUTATION}
      -P "${GATE}"
    RESULT_VARIABLE RESULT OUTPUT_VARIABLE OUTPUT ERROR_VARIABLE ERROR)
  if(EXPECT_SUCCESS)
    if(NOT RESULT EQUAL 0)
      message(FATAL_ERROR "gate ${NAME} unexpectedly failed: ${OUTPUT}${ERROR}")
    endif()
  else()
    if(RESULT EQUAL 0)
      message(FATAL_ERROR "gate ${NAME} unexpectedly passed")
    endif()
    string(REGEX REPLACE "[ \r\n\t]+" " " DIAGNOSTIC "${OUTPUT}${ERROR}")
    string(FIND "${DIAGNOSTIC}" "${EXPECT_TEXT}" HIT)
    if(HIT EQUAL -1)
      message(FATAL_ERROR "gate ${NAME} failed without exact diagnostic: ${OUTPUT}${ERROR}")
    endif()
  endif()
endfunction()

run_gate(off_linux TRUE Linux OFF OFF OFF "")
run_gate(preinit_linux TRUE Linux ON OFF OFF "")
run_gate(initializer_linux TRUE Linux ON ON OFF "")
run_gate(initializer_without_preinit FALSE Linux OFF ON OFF
  "HALOFPX_REGISTRY_LAB_LINUX_INITIALIZER requires HALOFPX_REGISTRY_LAB_LINUX_PREINIT")
run_gate(preinit_off_linux FALSE Windows ON OFF OFF
  "HALOFPX_REGISTRY_LAB_LINUX_PREINIT is available only on Linux")
run_gate(initializer_off_linux FALSE Windows ON ON OFF
  "HALOFPX_REGISTRY_LAB_LINUX_PREINIT is available only on Linux")
run_gate(mutation_linux FALSE Linux ON ON ON
  "HALOFPX_REGISTRY_LAB_LINUX_MUTATION is not admitted by ADR-0025")

set(FEATURE_OFF_BUILD "${BINARY_ROOT}/halofpx-l05t-nested-feature-off")
set(CONFIGURE_COMMAND "${CMAKE_COMMAND}" -S "${SOURCE_ROOT}" -B "${FEATURE_OFF_BUILD}"
  -G "${GENERATOR}"
  -DCMAKE_BUILD_TYPE=Release
  -DLLAMA_BUILD_TESTS=ON
  -DLLAMA_BUILD_SERVER=ON
  -DLLAMA_BUILD_EXAMPLES=OFF
  -DLLAMA_BUILD_TOOLS=ON
  -DLLAMA_BUILD_WEBUI=OFF
  -DLLAMA_USE_PREBUILT_WEBUI=OFF
  -DGGML_HIP=OFF
  -DGGML_VULKAN=OFF
  -DHALOFPX_REGISTRY_LAB_LINUX_PREINIT=OFF
  -DHALOFPX_REGISTRY_LAB_LINUX_INITIALIZER=OFF
  -DHALOFPX_REGISTRY_LAB_LINUX_MUTATION=OFF)
if(DEFINED MAKE_PROGRAM AND NOT MAKE_PROGRAM STREQUAL "")
  list(APPEND CONFIGURE_COMMAND -DCMAKE_MAKE_PROGRAM=${MAKE_PROGRAM})
endif()
execute_process(COMMAND ${CONFIGURE_COMMAND}
  RESULT_VARIABLE CONFIGURE_RESULT OUTPUT_VARIABLE CONFIGURE_OUTPUT ERROR_VARIABLE CONFIGURE_ERROR)
if(NOT CONFIGURE_RESULT EQUAL 0)
  message(FATAL_ERROR "feature-off configure failed: ${CONFIGURE_OUTPUT}${CONFIGURE_ERROR}")
endif()

execute_process(COMMAND "${CMAKE_COMMAND}" --build "${FEATURE_OFF_BUILD}" --target help
  RESULT_VARIABLE HELP_RESULT OUTPUT_VARIABLE TARGETS ERROR_VARIABLE HELP_ERROR)
if(NOT HELP_RESULT EQUAL 0)
  message(FATAL_ERROR "feature-off target inventory failed: ${HELP_ERROR}")
endif()
execute_process(COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${FEATURE_OFF_BUILD}" -N
  RESULT_VARIABLE CTEST_RESULT OUTPUT_VARIABLE TESTS ERROR_VARIABLE CTEST_ERROR)
if(NOT CTEST_RESULT EQUAL 0)
  message(FATAL_ERROR "feature-off CTest inventory failed: ${CTEST_ERROR}")
endif()
foreach(TEXT IN ITEMS "${TARGETS}" "${TESTS}")
  string(FIND "${TEXT}" "registry-lab-linux-initializer" LEAK)
  if(NOT LEAK EQUAL -1)
    message(FATAL_ERROR "initializer leaked into feature-off target or CTest inventory")
  endif()
endforeach()

message(STATUS "PASS: L05t executable build-gate and feature-off absence matrix")
