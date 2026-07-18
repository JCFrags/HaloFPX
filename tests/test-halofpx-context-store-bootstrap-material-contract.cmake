file(READ "${SOURCE_ROOT}/tools/server/CMakeLists.txt" server_cmake)
file(READ "${SOURCE_ROOT}/tools/server/halofpx-context-store-bootstrap-material.h" header)
file(READ "${SOURCE_ROOT}/tools/server/halofpx-context-store-bootstrap-material.cpp" implementation)

foreach(marker
    "halofpx-context-store-bootstrap-material STATIC EXCLUDE_FROM_ALL"
    "context_store_bootstrap_material_synthetic_proof"
    "context_store_bootstrap_material_synthetic_witness"
    "prepare_exact_material_set_and_durable_close"
    "maximum_terminal_attempts_ = 512"
    "halofpx.bootstrap-material-root-policy.v1"
    "halofpx.bootstrap-material-authority-source.v1"
    "halofpx.bootstrap-material-source-set.v1"
    "halofpx.bootstrap-material-set.v1"
    "halofpx.bootstrap-material-preparation.v1"
    "halofpx.bootstrap-material-durable-close.v1")
    string(FIND "${server_cmake}${header}${implementation}" "${marker}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "missing L05m contract marker: ${marker}")
    endif()
endforeach()

string(FIND "${server_cmake}" "set(TARGET server-context)" runtime_start)
if(runtime_start EQUAL -1)
    message(FATAL_ERROR "cannot locate runtime server CMake graph")
endif()
string(SUBSTRING "${server_cmake}" ${runtime_start} -1 runtime_graph)
string(FIND "${runtime_graph}" "halofpx-context-store-bootstrap-material" server_linkage)
if(NOT server_linkage EQUAL -1)
    message(FATAL_ERROR "L05m target occurs in runtime server CMake graph")
endif()

set(surface "${header}${implementation}")
foreach(forbidden
    "std::filesystem"
    "<filesystem>"
    "<fstream>"
    "CreateFile"
    "fopen("
    "FILE *"
    "protected_anchor_storage"
    "create_anchor"
    "replace_anchor")
    string(FIND "${surface}" "${forbidden}" found)
    if(NOT found EQUAL -1)
        message(FATAL_ERROR "forbidden L05m implementation surface: ${forbidden}")
    endif()
endforeach()
