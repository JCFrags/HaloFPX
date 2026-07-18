file(READ "${SOURCE_ROOT}/tools/server/CMakeLists.txt" SERVER_CMAKE)
file(READ "${SOURCE_ROOT}/tools/server/halofpx-context-store-authority.cpp" AUTHORITY_SOURCE)
file(READ "${SOURCE_ROOT}/tools/server/halofpx-context-store-authority.h" AUTHORITY_HEADER)

if(NOT SERVER_CMAKE MATCHES "halofpx-context-store-authority STATIC EXCLUDE_FROM_ALL")
    message(FATAL_ERROR "bootstrap authority must remain EXCLUDE_FROM_ALL")
endif()
if(SERVER_CMAKE MATCHES "server-context[^\n]*halofpx-context-store-authority" OR
   SERVER_CMAKE MATCHES "target_link_libraries\\(\\$\\{TARGET\\}[^\\)]*halofpx-context-store-authority")
    message(FATAL_ERROR "bootstrap authority must not enter the server runtime")
endif()
set(AUTHORITY_ALL "${AUTHORITY_SOURCE}\n${AUTHORITY_HEADER}")
foreach(FORBIDDEN_PATTERN
        "#[ \t]*include[ \t]*[<\"]filesystem"
        "#[ \t]*include[ \t]*[<\"]fstream"
        "std::filesystem"
        "fopen[ \t]*\\("
        "CreateFile[A-Z]*[ \t]*\\("
        "openat[0-9]*[ \t]*\\("
        "rename[at0-9_]*[ \t]*\\("
        "context_store_provider"
        "server-context"
        "external_token"
        "replay_history")
    if(AUTHORITY_ALL MATCHES "${FORBIDDEN_PATTERN}")
        message(FATAL_ERROR "forbidden I/O, runtime, or durability claim in bootstrap authority: ${FORBIDDEN_PATTERN}")
    endif()
endforeach()
foreach(REQUIRED_PATTERN
        "context_store_bootstrap_status::authorized_unexecuted"
        "context_store_anchor_status::authenticated_unadmitted"
        "context_store_master_key_max_bytes"
        "context_store_manifest_max_objects"
        "generation = 1"
        "has_predecessor = false"
        "halofpx.bootstrap-authority-snapshot.v1"
        "halofpx.bootstrap-authorization.v1"
        "same_secret"
        "nonzero_bytes"
        "wipe\\(")
    if(NOT AUTHORITY_ALL MATCHES "${REQUIRED_PATTERN}")
        message(FATAL_ERROR "missing sealed bootstrap-authority contract marker: ${REQUIRED_PATTERN}")
    endif()
endforeach()
