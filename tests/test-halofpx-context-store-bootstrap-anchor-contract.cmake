file(READ "${SOURCE_ROOT}/tools/server/CMakeLists.txt" server_cmake)
file(READ "${SOURCE_ROOT}/tools/server/halofpx-context-store-bootstrap-anchor.h" header)
file(READ "${SOURCE_ROOT}/tools/server/halofpx-context-store-bootstrap-anchor.cpp" implementation)

foreach(marker
    "halofpx-context-store-bootstrap-anchor STATIC EXCLUDE_FROM_ALL"
    "context_store_bootstrap_anchor_synthetic_proof"
    "context_store_bootstrap_anchor_synthetic_uncertain_handle"
    "inspect_create_if_absent_synchronize_and_durable_close"
    "fence_original_observe_current_synchronize_and_durable_close"
    "maximum_terminal_attempts_ = 512"
    "halofpx.bootstrap-anchor-root-policy.v1"
    "halofpx.bootstrap-anchor-material-source.v1"
    "halofpx.bootstrap-anchor-create.v1"
    "halofpx.bootstrap-anchor-durable-close.v1"
    "halofpx.bootstrap-anchor-reconciliation.v1"
    "halofpx.bootstrap-anchor-reconciliation-fence.v1"
    "halofpx.bootstrap-anchor-reconciliation-durable-close.v1")
    string(FIND "${server_cmake}${header}${implementation}" "${marker}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "missing L05n contract marker: ${marker}")
    endif()
endforeach()

string(FIND "${server_cmake}" "set(TARGET server-context)" runtime_start)
string(SUBSTRING "${server_cmake}" ${runtime_start} -1 runtime_graph)
string(FIND "${runtime_graph}" "halofpx-context-store-bootstrap-anchor" runtime_linkage)
if(NOT runtime_linkage EQUAL -1)
    message(FATAL_ERROR "L05n target occurs in runtime graph")
endif()

set(surface "${header}${implementation}")
foreach(forbidden
    "std::filesystem" "<filesystem>" "<fstream>" "CreateFile" "fopen("
    "FILE *" "context_store_publication_writer" "context_store_provider"
    "server-context" "serialize(" "conversion operator")
    string(FIND "${surface}" "${forbidden}" found)
    if(NOT found EQUAL -1)
        message(FATAL_ERROR "forbidden L05n surface: ${forbidden}")
    endif()
endforeach()
