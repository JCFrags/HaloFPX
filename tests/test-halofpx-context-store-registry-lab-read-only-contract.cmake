set(ROOT "${SOURCE_ROOT}")
set(H "${ROOT}/tools/server/halofpx-context-store-registry-lab-read-only-internal.h")
set(C "${ROOT}/tools/server/halofpx-context-store-registry-lab-read-only.cpp")
set(T "${ROOT}/tests/test-halofpx-context-store-registry-lab-read-only.cpp")
foreach(F IN ITEMS "${H}" "${C}" "${T}")
  if(NOT EXISTS "${F}")
    message(FATAL_ERROR "missing L05R registry-lab quarantine source: ${F}")
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
      message(FATAL_ERROR "L05R fake-only quarantine lane contains forbidden surface ${FORBIDDEN}: ${F}")
    endif()
  endforeach()
endforeach()

file(READ "${C}" IMPLEMENTATION)
file(READ "${H}" HEADER_TEXT)
file(READ "${T}" TEST_TEXT)
foreach(UNAVAILABLE IN ITEMS
    "before_first_mutation" "quarantine reason"
    "modeled_advanced_closed")
  string(FIND "${IMPLEMENTATION}" "${UNAVAILABLE}" HIT)
  if(NOT HIT EQUAL -1)
    message(FATAL_ERROR "L05R implementation opened an unavailable mutation surface: ${UNAVAILABLE}")
  endif()
endforeach()
foreach(REQUIRED IN ITEMS
    "value.size != 14" "script_matches_quarantine"
    "quarantine_event_id_witness quarantine_event_authority"
    "quarantine_event_confirmed" "quarantine_event_authority.authorize"
    "quarantine_event_authority.consume" "rederived_bytes" "rederived_action"
    "derive_quarantine_artifact" "context_store_registry_lab_admit_quarantine_v1"
    "context_store_registry_lab_encode_quarantine_v1" "context_store_registry_lab_verify_v1"
    "halofpx.registry-lab-quarantine-action.v1"
    "operation::quarantine_staging_create" "operation::quarantine_staging_write"
    "operation::quarantine_staging_readback" "operation::quarantine_file_sync"
    "operation::quarantine_publish_rename" "operation::quarantine_root_directory_sync"
    "operation::quarantine_staging_directory_sync"
    "quarantine_publication_happy_path_contract" "1, 2, 3, 4, 5, 69, 6, 70, 71, 72, 73, 74, 75, 76, 90, 91, 92")
  string(FIND "${IMPLEMENTATION}${HEADER_TEXT}${TEST_TEXT}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "L05R quarantine publication closure is missing: ${REQUIRED}")
  endif()
endforeach()
foreach(REQUIRED IN ITEMS
    "struct quarantine_encoding_inputs" "derive_quarantine_encoding_inputs"
    "quarantine_encoding_inputs_for_test" "quarantine_encoding_inputs_test_audit"
    "quarantine_encoding_inputs_contract" "quarantine_shape::successor"
    "selected_matches_shape" "predecessor_head_witness"
    "audit.transition_present" "audit.standalone_head_present"
    "audit.predecessor_head_digest" "audit.successor_head_digest" "audit.prepare_digest"
    "~quarantine_encoding_inputs() noexcept { clear(); }" "output.clear()"
    "audit.explicit_wipe_verified" "inputs.clear()"
    "wipe_trivially_copyable_object" "volatile uint8_t * bytes"
    "trivially_copyable_object_is_zero(inputs.value)"
    "trivially_copyable_object_is_zero(inputs.transition)"
    "trivially_copyable_object_is_zero(inputs.standalone_predecessor_head)")
  string(FIND "${IMPLEMENTATION}${HEADER_TEXT}${TEST_TEXT}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "L05R pre-event quarantine encoding seam is missing: ${REQUIRED}")
  endif()
endforeach()

foreach(REQUIRED IN ITEMS
    "admitted_all == 55" "admitted_14 == 43" "forbidden_14 == 437"
    "losses == 11" "deaths == 16" "rejected == 437" "executed == 43"
    "process_wide_death" "restart_round_trip" "allocation_free_after_construction"
    "operation_5_core" "operation_5_hostile_bytes" "operation_5_key_selection_before_kdf"
    "scanned_slots(h) == 512" "bit_mutations == truncations * 8"
    "pairwise_precedence_combinations == 55" "replay_positions == 512" "corrupt_positions == 512")
  string(FIND "${TEST_TEXT}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "L05R focused test is missing required inherited closure marker: ${REQUIRED}")
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
    "derive_quarantine_action_commitment" "writer.map(15)"
    "halofpx.registry-lab-quarantine-action.v1"
    "quarantine_action_commitment_for_test" "quarantine_action_commitment_contract"
    "test_quarantine_action_commitment" "admitted == 25 && rejected == 39"
    "quarantine_encoded_length < 1" "quarantine_encoded_length > 1024"
    "sensitivities == 13"
    "f049302b4e309f45e8591d02a551168563e503279771c88b1c93168d13df1968"
    "5f60142504bb61ff54525f54c709b0ae4085d31772b444fad742f09d08cdea1f"
    "6b021681823318f9ca268c1419270d699402ae35180af91fa2a937ff5bc4e14e"
    "7bbdf63c2760b87fa5ce5b18d04b350afe310495267242f87ac9b87e4ced476e")
  string(FIND "${IMPLEMENTATION}${HEADER_TEXT}${TEST_TEXT}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "L05R quarantine action commitment is missing: ${REQUIRED}")
  endif()
endforeach()
foreach(REQUIRED IN ITEMS
    "derive_quarantine_diagnosis_commitment" "writer.map(12)"
    "quarantine_diagnosis_commitment_for_test"
    "halofpx.registry-lab-quarantine-diagnosis.v1"
    "test_quarantine_diagnosis_commitment" "commitment_nonzero == publishable"
    "diagnosis.diagnosis_commitment != test_quarantine_diagnosis_commitment(invocation, diagnosis)"
    "092371f4b14a2df8523f09f2e50ea7683b3041b3beaf391761a17d4556f009d5"
    "3e8ff4967a3eccf257fffa0f3e9e5bb77e5f14b08f2d32ef9772c3592e5bb8f8"
    "26b2227f62292c195285025f828cc028e8ff8acbecc7a7c67368332cd1fedd02")
  string(FIND "${IMPLEMENTATION}${HEADER_TEXT}${TEST_TEXT}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "L05R diagnosis commitment is missing required closure marker: ${REQUIRED}")
  endif()
endforeach()
foreach(REQUIRED IN ITEMS "admitted == 25 && rejected == 39" "sensitivities == 10" "quarantine_shape::successor")
  string(FIND "${TEST_TEXT}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "L05R diagnosis commitment test matrix is missing: ${REQUIRED}")
  endif()
endforeach()
foreach(REQUIRED IN ITEMS
    "std::atomic<uint64_t> quarantine_event_issuance_sequence"
    "compare_exchange_weak" "observed == UINT64_MAX"
    "HALOFPX-L05R-EV1" "0x484650584c303552"
    "fake_quarantine_event_id_matches_test_oracle"
    "class quarantine_event_id_witness" "~quarantine_event_id_witness() noexcept { clear(); }"
    "quarantine_event_id_witness(const quarantine_event_id_witness &) = delete"
    "other.clear()" "wrong_invocation_rejected" "wrong_diagnosis_rejected"
    "replay_rejected" "moved_from_rejected" "quarantine_event_authority_for_test"
    "quarantine_event_concurrency_begin_for_test" "quarantine_event_concurrency_worker_for_test"
    "quarantine_event_concurrency_finish_for_test" "retained_ids")
  string(FIND "${IMPLEMENTATION}${HEADER_TEXT}${TEST_TEXT}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "L05R private event authority is missing required closure marker: ${REQUIRED}")
  endif()
endforeach()
foreach(REQUIRED IN ITEMS
    "quarantine_event_authority_contract" "concurrency.retained == 64"
    "audit.issued == 32 && audit.consumed == 32"
    "audit.nonzero && audit.distinct && audit.exact_encoding && audit.invalid_binding_rejected"
    "audit.move_source_wiped && audit.explicit_wipe_verified && audit.destructor_clear_path_exercised"
    "concurrency.pairwise_distinct && concurrency.exact_encoding")
  string(FIND "${TEST_TEXT}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "L05R private event authority tests are missing required marker: ${REQUIRED}")
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
    "l05r_quarantine_product_execution_and_preentry_rejection"
    "admitted == 155 && reached == 155 && rejected == 805 && frontiers == 18"
    "--l05r-products" "l05r_expected_projection_count"
    "l05r_validate_projection_frontier" "same_restart(*image, *deterministic)"
    "sequence_values_consumed == (blocked ? 0U : 1U)"
    "l05r_quarantine_operation_6_revalidation" "state_axes == 22 && private_axes == 6"
    "inject_quarantine_private_fault_for_test" "maximum_logical_authority"
    "l05r_quarantine_hostile_readback" "inject_quarantine_retagged_readback_for_test" "attacks == 4"
    "l05r_quarantine_no_replace_and_writer_isolation" "blockers == 6"
    "l05r_quarantine_script_shape_rejection" "rejected == 61"
    "l05r_quarantine_allocation_free_after_construction")
  string(FIND "${IMPLEMENTATION}${HEADER_TEXT}${TEST_TEXT}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "L05R exhaustive quarantine closure is missing: ${REQUIRED}")
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
    message(FATAL_ERROR "L05R focused test is missing inherited recovery closure marker: ${REQUIRED}")
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
    message(FATAL_ERROR "L05R contract is missing inherited L05Q exhaustive marker: ${REQUIRED}")
  endif()
endforeach()

foreach(REQUIRED IN ITEMS
    "current.action_latched && state_.modeled_available_bytes < registry_minimum_reserve_bytes"
    "authenticated_readback_contradiction" "const bool primitive_failed"
    "derived = primitive_code::ok" "derived = primitive_code::capacity_exhausted"
    "derived = primitive_code::reserve_exhausted"
    "quarantine_effect_authorized" "quarantine_sequence_values_consumed"
    "quarantine_event_fail_next_issuance_for_test")
  string(FIND "${IMPLEMENTATION}" "${REQUIRED}" HIT)
  if(HIT EQUAL -1)
    message(FATAL_ERROR "L05R implementation is missing fail-closed post-admission marker: ${REQUIRED}")
  endif()
endforeach()

file(READ "${ROOT}/tools/server/CMakeLists.txt" SERVER_CMAKE)
if(NOT SERVER_CMAKE MATCHES "add_library\\(halofpx-context-store-registry-lab-read-only STATIC EXCLUDE_FROM_ALL")
  message(FATAL_ERROR "L05R read-only target is not STATIC EXCLUDE_FROM_ALL")
endif()
string(FIND "${SERVER_CMAKE}" "target_link_libraries(halofpx-context-store-registry-lab-read-only PRIVATE\n    halofpx-context-store-registry-lab-wire)" NARROW_LINK)
if(NARROW_LINK EQUAL -1)
  message(FATAL_ERROR "L05R target does not have the single narrow private wire edge")
endif()
foreach(FORBIDDEN_SURFACE IN ITEMS "install(TARGETS halofpx-context-store-registry-lab-read-only" "PUBLIC halofpx-context-store-registry-lab-read-only" "INTERFACE halofpx-context-store-registry-lab-read-only")
  string(FIND "${SERVER_CMAKE}" "${FORBIDDEN_SURFACE}" HIT)
  if(NOT HIT EQUAL -1)
    message(FATAL_ERROR "L05R target has a public/install/export surface: ${FORBIDDEN_SURFACE}")
  endif()
endforeach()
string(FIND "${SERVER_CMAKE}" "set(TARGET server-context)" PRODUCT_MARKER)
if(PRODUCT_MARKER EQUAL -1)
  message(FATAL_ERROR "server product marker is missing")
endif()
string(SUBSTRING "${SERVER_CMAKE}" ${PRODUCT_MARKER} -1 PRODUCT_TAIL)
string(FIND "${PRODUCT_TAIL}" "halofpx-context-store-registry-lab-read-only" PRODUCT_EDGE)
if(NOT PRODUCT_EDGE EQUAL -1)
  message(FATAL_ERROR "L05R internal target leaked into product linkage")
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
    message(FATAL_ERROR "L05R implementation is missing operation-5 closure marker: ${REQUIRED}")
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
    message(FATAL_ERROR "L05R implementation is missing inherited recovery closure marker: ${REQUIRED}")
  endif()
endforeach()

message(STATUS "PASS: L05R fake-only quarantine publication contract")
