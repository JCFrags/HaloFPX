set(ROOT "${SOURCE_ROOT}")
set(H "${ROOT}/tools/server/halofpx-context-store-registry-lab-read-only-internal.h")
set(C "${ROOT}/tools/server/halofpx-context-store-registry-lab-read-only.cpp")
set(T "${ROOT}/tests/test-halofpx-context-store-registry-lab-read-only.cpp")
foreach(F IN ITEMS "${H}" "${C}" "${T}")
  if(NOT EXISTS "${F}")
    message(FATAL_ERROR "missing L05Q registry-lab recovery source: ${F}")
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
      message(FATAL_ERROR "L05Q fake-only recovery contains forbidden surface ${FORBIDDEN}: ${F}")
    endif()
  endforeach()
endforeach()

file(READ "${C}" IMPLEMENTATION)
file(READ "${H}" HEADER_TEXT)
foreach(UNAVAILABLE IN ITEMS
    "before_first_mutation" "quarantine reason" "context_store_registry_lab_admit_quarantine"
    "context_store_registry_lab_encode_quarantine" "modeled_advanced_closed")
  string(FIND "${IMPLEMENTATION}" "${UNAVAILABLE}" HIT)
  if(NOT HIT EQUAL -1)
    message(FATAL_ERROR "L05Q implementation opened an unavailable mutation surface: ${UNAVAILABLE}")
  endif()
endforeach()

file(READ "${T}" TEST_TEXT)
foreach(REQUIRED IN ITEMS
    "admitted_all == 55" "admitted_14 == 43" "forbidden_14 == 437"
    "losses == 11" "deaths == 16" "rejected == 437" "executed == 43"
    "process_wide_death" "restart_round_trip" "allocation_free_after_construction"
    "operation_5_core" "operation_5_hostile_bytes" "operation_5_key_selection_before_kdf"
    "scanned_slots(h) == 512" "bit_mutations == truncations * 8"
    "pairwise_precedence_combinations == 55" "replay_positions == 512" "corrupt_positions == 512")
  string(FIND "${TEST_TEXT}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "L05p focused test is missing required closure marker: ${REQUIRED}")
  endif()
endforeach()

foreach(REQUIRED IN ITEMS
    "enum class quarantine_reason" "enum class quarantine_shape"
    "quarantine_diagnosis_view" "select_quarantine_reason_for_test"
    "precedence { 13,12,1,15,2,10,4,5,6,7,8,9,11,0 }"
    "authenticated_initialized_root" "reason_admits_shape"
    "quarantine_diagnosis(size_t handle)" "--l05r-diagnosis")
  string(FIND "${IMPLEMENTATION}${HEADER_TEXT}${TEST_TEXT}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "L05R diagnosis contract is missing required marker: ${REQUIRED}")
  endif()
endforeach()
foreach(REQUIRED IN ITEMS
    "singleton_cases == 13" "pairwise_cases == 78" "diagnosed == 16"
    "event > static_cast<uint16_t>(operation::quarantine_staging_directory_sync)"
    "_set_error_mode(_OUT_TO_STDERR)"
    "_set_abort_behavior(_WRITE_ABORT_MSG, _WRITE_ABORT_MSG | _CALL_REPORTFAULT)")
  string(FIND "${TEST_TEXT}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "L05R focused test is missing required closure marker: ${REQUIRED}")
  endif()
endforeach()
foreach(REQUIRED IN ITEMS
    "recovery_admitted == 287" "quarantine_admitted == 155" "quarantine_forbidden == 805"
    "admitted_algebra_count() == 497"
    "operation_6_recovery_success_and_prior_request_isolation"
    "operation_6_state_critical_mismatch_is_unavailable" "operation_6_post_admission_faults"
    "operation_6_unavailable_requires_state_mismatch"
    "operation_6_allocation_free_after_construction" "operation_6_accounting_boundaries")
  string(FIND "${TEST_TEXT}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "L05Q focused test is missing recovery closure marker: ${REQUIRED}")
  endif()
endforeach()
foreach(REQUIRED IN ITEMS
    "executed == 3072" "expected_projections = 10335"
    "rejected != 1393" "truncations == 712" "bit_mutations == 5696"
    "semantic_attacks == 8" "rejected == 91" "--l05q-products" "--l05q-exhaustive"
    "restart_projection_count" "project_restart" "restore_restart"
    "l05q_restart_contains_credential_secret" "l05q_restart_file_contains_credential_secret")
  string(FIND "${TEST_TEXT}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "L05Q exhaustive test is missing required closure marker: ${REQUIRED}")
  endif()
endforeach()

foreach(REQUIRED IN ITEMS
    "current.action_latched && state_.modeled_available_bytes < registry_minimum_reserve_bytes"
    "authenticated_readback_contradiction" "product.code == primitive_code::unavailable) derived = primitive_code::ok")
  string(FIND "${IMPLEMENTATION}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "L05Q implementation is missing fail-closed post-admission marker: ${REQUIRED}")
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
foreach(REQUIRED IN ITEMS
    "authenticated_record_v1" "decode_record" "difference |= a[i] ^ b[i]"
    "product.op == operation::recovery_validation" "{ 201 }"
    "snapshot_owner_" "recovery_classification::continue_to_mutation")
  string(FIND "${IMPLEMENTATION}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "L05p implementation is missing operation-5 closure marker: ${REQUIRED}")
  endif()
endforeach()

foreach(REQUIRED IN ITEMS
    "action_mutation_admission = 6" "staging_directory_sync_after_successor = 36"
    "staging_directory_sync_after_head = 46" "terminal_create = 60" "attempts_directory_sync = 64"
    "quarantine_event_id_acquire = 69" "quarantine_staging_create = 70"
    "quarantine_staging_directory_sync = 76"
    "halofpx.registry-lab-recovery-action.v1" "script_matches_recovery" "derive_recovery_terminal"
    "context_store_registry_lab_terminal_class_v1::recovered" "return step(handle)")
  string(FIND "${IMPLEMENTATION}${HEADER_TEXT}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "L05Q implementation is missing recovery closure marker: ${REQUIRED}")
  endif()
endforeach()

message(STATUS "PASS: L05R fake-only recovery diagnosis contract")
