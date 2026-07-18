if(NOT DEFINED SOURCE_ROOT)
  message(FATAL_ERROR "SOURCE_ROOT required")
endif()
set(H "${SOURCE_ROOT}/tools/server/halofpx-context-store-bootstrap-consumption.h")
set(S "${SOURCE_ROOT}/tools/server/halofpx-context-store-bootstrap-consumption.cpp")
file(READ "${H}" HT)
file(READ "${S}" ST)
file(READ "${SOURCE_ROOT}/tools/server/CMakeLists.txt" CT)
foreach(REQUIRED
    "EXCLUDE_FROM_ALL"
    "halofpx.bootstrap-consumption-reconciliation.v1"
    "fence_original_and_read_current"
    "consumption_uncertain"
    "successor_recovered_terminal"
    "predecessor_confirmed_terminal_no_retry"
    "context_store_bootstrap_reconciliation_witness_max_bytes = 1024"
    "authoritative_present"
    "authoritative_absent"
    "definitely_unconsumed_fenced_no_retry"
    "consumed_same_recovered_unexecuted"
    "observed_successor_data"
    "original_consumption_uncertain_confirmed"
    "original_operation_commitment"
    "reconciliation_commitment"
    "reconciliation_attempt_"
    "has_reconciliation_attempt_"
    "friend class context_store_bootstrap_reconciliation_coordinator"
    "exact_head"
    "std::atomic<bool>")
  if(NOT "${CT}${HT}${ST}" MATCHES "${REQUIRED}")
    message(FATAL_ERROR "missing L05l contract marker: ${REQUIRED}")
  endif()
endforeach()
if(NOT HT MATCHES "private:[\r\n \t]+context_store_bootstrap_backend_outcome execute[ \t]*\\(")
  message(FATAL_ERROR "consumption execution must remain private")
endif()
if(NOT HT MATCHES "execute_reconciliation[\r\n \t]*\\(")
  message(FATAL_ERROR "reconciliation execution must remain backend-owned")
endif()
foreach(FORBIDDEN "filesystem" "fstream" "fopen[ \\t]*\\(" "CreateFile" "rename[at0-9_]*[ \\t]*\\(" "server-context" "std::thread" "sleep_for" "socket" "curl" "create_if_absent" "anchor_absence")
  if("${HT}${ST}" MATCHES "${FORBIDDEN}")
    message(FATAL_ERROR "forbidden L05l production surface: ${FORBIDDEN}")
  endif()
endforeach()
if(CT MATCHES "target_link_libraries\\(server-context[^)]*bootstrap-consumption")
  message(FATAL_ERROR "L05l linked into server-context")
endif()
