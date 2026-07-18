set(ROOT "${SOURCE_ROOT}")
set(H "${ROOT}/tools/server/halofpx-context-store-registry-lab-read-only-internal.h")
set(C "${ROOT}/tools/server/halofpx-context-store-registry-lab-read-only.cpp")
set(T "${ROOT}/tests/test-halofpx-context-store-registry-lab-read-only.cpp")
foreach(F IN ITEMS "${H}" "${C}" "${T}")
  if(NOT EXISTS "${F}")
    message(FATAL_ERROR "missing L05p operations 1-4 source: ${F}")
  endif()
endforeach()

foreach(F IN ITEMS "${H}" "${C}")
  file(READ "${F}" TEXT)
  foreach(FORBIDDEN IN ITEMS
      "#include <filesystem>" "#include <fstream>" "std::filesystem"
      "open(" "fopen(" "read(" "write(" "rename(" "fsync(" "fdatasync("
      "CreateFile" "ReadFile" "WriteFile" "MoveFile" "DeleteFile"
      "getenv(" "system(" "std::thread" "#include <thread>" "std::function"
      "std::vector" "std::string" "malloc(" "calloc(" "realloc(" "operator new"
      "concrete_registry_lab_observation" "modeled_registry_lab_terminal_disposition"
      "halofpx-context-store-bootstrap-consumption" "halofpx-context-store-bootstrap-material"
      "halofpx-context-store-bootstrap-anchor" "llama-ai" "CachyLLama")
    string(FIND "${TEXT}" "${FORBIDDEN}" HIT)
    if(NOT HIT EQUAL -1)
      message(FATAL_ERROR "L05p operations 1-4 contains forbidden surface ${FORBIDDEN}: ${F}")
    endif()
  endforeach()
endforeach()

file(READ "${C}" IMPLEMENTATION)
foreach(UNAVAILABLE IN ITEMS
    "case operation::recovery_validation" "operation 6" "before_first_mutation"
    "authenticated_record_v1" "decode_" "quarantine reason" "publication")
  string(FIND "${IMPLEMENTATION}" "${UNAVAILABLE}" HIT)
  if(NOT HIT EQUAL -1)
    message(FATAL_ERROR "L05p implementation opened unavailable operation-5/mutation surface: ${UNAVAILABLE}")
  endif()
endforeach()

file(READ "${T}" TEST_TEXT)
foreach(REQUIRED IN ITEMS
    "admitted_all == 55" "admitted_14 == 43" "forbidden_14 == 437"
    "losses == 11" "deaths == 16" "rejected == 437" "executed == 43"
    "process_wide_death" "restart_round_trip" "allocation_free_after_construction")
  string(FIND "${TEST_TEXT}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "L05p focused test is missing required closure marker: ${REQUIRED}")
  endif()
endforeach()

file(READ "${ROOT}/tools/server/CMakeLists.txt" SERVER_CMAKE)
if(NOT SERVER_CMAKE MATCHES "add_library\\(halofpx-context-store-registry-lab-read-only STATIC EXCLUDE_FROM_ALL")
  message(FATAL_ERROR "L05p read-only target is not STATIC EXCLUDE_FROM_ALL")
endif()
string(FIND "${SERVER_CMAKE}" "target_link_libraries(halofpx-context-store-registry-lab-read-only PRIVATE\n    halofpx-context-store-registry-lab-wire)" NARROW_LINK)
if(NARROW_LINK EQUAL -1)
  message(FATAL_ERROR "L05p target does not have the single narrow private wire edge")
endif()
foreach(FORBIDDEN_SURFACE IN ITEMS "install(TARGETS halofpx-context-store-registry-lab-read-only" "PUBLIC halofpx-context-store-registry-lab-read-only" "INTERFACE halofpx-context-store-registry-lab-read-only")
  string(FIND "${SERVER_CMAKE}" "${FORBIDDEN_SURFACE}" HIT)
  if(NOT HIT EQUAL -1)
    message(FATAL_ERROR "L05p target has a public/install/export surface: ${FORBIDDEN_SURFACE}")
  endif()
endforeach()
string(FIND "${SERVER_CMAKE}" "set(TARGET server-context)" PRODUCT_MARKER)
if(PRODUCT_MARKER EQUAL -1)
  message(FATAL_ERROR "server product marker is missing")
endif()
string(SUBSTRING "${SERVER_CMAKE}" ${PRODUCT_MARKER} -1 PRODUCT_TAIL)
string(FIND "${PRODUCT_TAIL}" "halofpx-context-store-registry-lab-read-only" PRODUCT_EDGE)
if(NOT PRODUCT_EDGE EQUAL -1)
  message(FATAL_ERROR "L05p internal target leaked into product linkage")
endif()

set(DETECTOR_CLEAN "bounded_fake_read_only_engine")
foreach(PROBE IN ITEMS "std::filesystem" "concrete_registry_lab_observation" "operation 6" "llama-ai")
  string(FIND "${DETECTOR_CLEAN}" "${PROBE}" CLEAN_HIT)
  if(NOT CLEAN_HIT EQUAL -1)
    message(FATAL_ERROR "detector negative control unexpectedly matched ${PROBE}")
  endif()
  set(DETECTOR_DIRTY "${DETECTOR_CLEAN}${PROBE}")
  string(FIND "${DETECTOR_DIRTY}" "${PROBE}" DIRTY_HIT)
  if(DIRTY_HIT EQUAL -1)
    message(FATAL_ERROR "detector positive control failed for ${PROBE}")
  endif()
endforeach()
message(STATUS "PASS: L05p operations 1-4 internal/read-only contract")
