set(ROOT "${SOURCE_ROOT}")
set(H "${ROOT}/tools/server/halofpx-context-store-registry-lab-linux-initializer-internal.h")
set(C "${ROOT}/tools/server/halofpx-context-store-registry-lab-linux-initializer.cpp")
foreach(F IN ITEMS "${H}" "${C}")
  if(NOT EXISTS "${F}")
    message(FATAL_ERROR "missing L05t initializer seam source: ${F}")
  endif()
  file(READ "${F}" TEXT)
  foreach(FORBIDDEN IN ITEMS
      "open(" "openat" "fopen(" "read(" "write(" "pwrite(" "rename"
      "fsync(" "fdatasync(" "mkdir" "unlink" "std::filesystem" "getenv("
      "system(" "fork(" "exec" "ptrace" "llama-ai" "CachyLLama")
    string(FIND "${TEXT}" "${FORBIDDEN}" HIT)
    if(NOT HIT EQUAL -1)
      message(FATAL_ERROR "L05t initializer seam contains forbidden mutation token ${FORBIDDEN}: ${F}")
    endif()
  endforeach()
endforeach()

file(READ "${ROOT}/CMakeLists.txt" ROOT_CMAKE)
foreach(REQUIRED IN ITEMS
    "option(HALOFPX_REGISTRY_LAB_LINUX_INITIALIZER"
    "include(cmake/halofpx-registry-lab-linux-gates.cmake)")
  string(FIND "${ROOT_CMAKE}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "missing L05t root build gate: ${REQUIRED}")
  endif()
endforeach()

file(READ "${ROOT}/cmake/halofpx-registry-lab-linux-gates.cmake" GATE_CMAKE)
foreach(REQUIRED IN ITEMS
    "HALOFPX_REGISTRY_LAB_LINUX_INITIALIZER requires HALOFPX_REGISTRY_LAB_LINUX_PREINIT"
    "HALOFPX_REGISTRY_LAB_LINUX_INITIALIZER is available only on Linux"
    "HALOFPX_REGISTRY_LAB_LINUX_MUTATION is not admitted by ADR-0025")
  string(FIND "${GATE_CMAKE}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "missing L05t executable build gate: ${REQUIRED}")
  endif()
endforeach()

file(READ "${ROOT}/tools/server/CMakeLists.txt" SERVER_CMAKE)
foreach(REQUIRED IN ITEMS
    "add_library(halofpx-context-store-registry-lab-linux-initializer STATIC EXCLUDE_FROM_ALL"
    "target_link_libraries(halofpx-context-store-registry-lab-linux-initializer PRIVATE\n        halofpx-context-store-registry-lab-wire)")
  string(FIND "${SERVER_CMAKE}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "missing L05t excluded target boundary: ${REQUIRED}")
  endif()
endforeach()

string(REGEX MATCHALL
    "target_link_libraries\\(halofpx-context-store-registry-lab-linux-initializer"
    INITIALIZER_LINK_CALLS "${SERVER_CMAKE}")
list(LENGTH INITIALIZER_LINK_CALLS INITIALIZER_LINK_CALL_COUNT)
if(NOT INITIALIZER_LINK_CALL_COUNT EQUAL 1)
  message(FATAL_ERROR "initializer must own exactly one target_link_libraries call")
endif()
foreach(FORBIDDEN IN ITEMS
    "target_link_libraries(halofpx-context-store-registry-lab-linux-initializer PUBLIC"
    "target_link_libraries(halofpx-context-store-registry-lab-linux-initializer INTERFACE"
    "install(TARGETS halofpx-context-store-registry-lab-linux-initializer"
    "install(FILES halofpx-context-store-registry-lab-linux-initializer"
    "llama_build(halofpx-context-store-registry-lab-linux-initializer"
    "set(HALOFPX_REGISTRY_LAB_LINUX_INITIALIZER_TARGET")
  string(FIND "${SERVER_CMAKE}" "${FORBIDDEN}" HIT)
  if(NOT HIT EQUAL -1)
    message(FATAL_ERROR "initializer has a forbidden build/export edge: ${FORBIDDEN}")
  endif()
endforeach()

string(FIND "${SERVER_CMAKE}" "set(TARGET server-context)" PRODUCT_MARKER)
if(PRODUCT_MARKER EQUAL -1)
  message(FATAL_ERROR "server product marker is missing")
endif()
string(SUBSTRING "${SERVER_CMAKE}" ${PRODUCT_MARKER} -1 PRODUCT_TAIL)
string(FIND "${PRODUCT_TAIL}" "halofpx-context-store-registry-lab-linux-initializer" PRODUCT_LEAK)
if(NOT PRODUCT_LEAK EQUAL -1)
  message(FATAL_ERROR "L05t initializer seam leaked into product/server linkage")
endif()

message(STATUS "PASS: L05t initializer no-I/O/default-off seam contract")
