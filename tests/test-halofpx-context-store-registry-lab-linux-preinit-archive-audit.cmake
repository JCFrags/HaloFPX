if(NOT DEFINED ARCHIVE_MANIFEST OR NOT DEFINED NM_TOOL OR NOT DEFINED AR_TOOL)
    message(FATAL_ERROR "ARCHIVE_MANIFEST, NM_TOOL, and AR_TOOL are required")
endif()
include("${ARCHIVE_MANIFEST}")
if(NOT EXISTS "${HALOFPX_L05S_ARCHIVE}")
    message(FATAL_ERROR "L05s archive does not exist: ${HALOFPX_L05S_ARCHIVE}")
endif()

execute_process(COMMAND "${AR_TOOL}" t "${HALOFPX_L05S_ARCHIVE}"
    RESULT_VARIABLE AR_RESULT OUTPUT_VARIABLE MEMBERS ERROR_VARIABLE AR_ERROR)
if(NOT AR_RESULT EQUAL 0)
    message(FATAL_ERROR "archive listing failed: ${AR_ERROR}")
endif()
string(REGEX MATCHALL "[^\r\n]+" MEMBER_LINES "${MEMBERS}")
list(LENGTH MEMBER_LINES MEMBER_COUNT)
if(NOT MEMBER_COUNT EQUAL 1)
    message(FATAL_ERROR "L05s archive must contain exactly one provider object; got ${MEMBER_COUNT}")
endif()

execute_process(COMMAND "${NM_TOOL}" -A -C "${HALOFPX_L05S_ARCHIVE}"
    RESULT_VARIABLE NM_RESULT OUTPUT_VARIABLE SYMBOLS ERROR_VARIABLE NM_ERROR)
if(NOT NM_RESULT EQUAL 0)
    message(FATAL_ERROR "archive symbol audit failed: ${NM_ERROR}")
endif()
foreach(FORBIDDEN IN ITEMS
        "concrete_registry_lab_observation"
        "preflight_context_v1"
        "registry_lab::read_only"
        "registry_lab::wire"
        "server_context"
        "llama_server"
        "publication_coordinator")
    string(FIND "${SYMBOLS}" "${FORBIDDEN}" POSITION)
    if(NOT POSITION EQUAL -1)
        message(FATAL_ERROR "forbidden archive symbol: ${FORBIDDEN}")
    endif()
endforeach()
