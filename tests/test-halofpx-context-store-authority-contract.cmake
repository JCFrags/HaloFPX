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
        "context_store_manifest_digest_v1"
        "context_store_verify_manifest_v1"
        "manifest_authentication_key"
        "trusted_compatibility"
        "manifest_data"
        "manifest_size"
        "authorization_token_data"
        "authorization_token_size"
        "context_store_verify_bootstrap_token_v1"
        "context_store_verify_protected_registry_v1"
        "context_store_bootstrap_status::authorization_rejected"
        "protected_registry_snapshot_digest"
        "protected_registry_policy_digest"
        "protected_registry_snapshot_data"
        "protected_registry_authentication_key"
        "expected_authorization_sequence"
        "authority_scope_commitment"
        "authorization_token_digest"
        "plan_commitment"
        "generation = 1"
        "has_predecessor = false"
        "halofpx.bootstrap-authority-snapshot.v1"
        "halofpx.bootstrap-authority-base-scope.v1"
        "halofpx.bootstrap-authority-scope.v2"
        "halofpx.bootstrap-plan.v2"
        "same_secret"
        "nonzero_bytes"
        "wipe\\(")
    if(NOT AUTHORITY_ALL MATCHES "${REQUIRED_PATTERN}")
        message(FATAL_ERROR "missing sealed bootstrap-authority contract marker: ${REQUIRED_PATTERN}")
    endif()
endforeach()

string(REGEX MATCH "struct context_store_bootstrap_authority_config \\{[^}]*\\}" CONFIG_SHAPE "${AUTHORITY_HEADER}")
foreach(FORMER_RAW "context_store_registered_id protected_registry_id" "uint64_t protected_registry_epoch" "context_store_format_digest protected_registry_snapshot_digest" "context_store_format_digest protected_registry_policy_digest" "uint64_t last_consumed_sequence")
    if(CONFIG_SHAPE MATCHES "${FORMER_RAW}")
        message(FATAL_ERROR "former caller-trusted registry scalar remains: ${FORMER_RAW}")
    endif()
endforeach()
foreach(FORMER_SOURCE "config.protected_registry_id" "config.protected_registry_epoch" "config.protected_registry_snapshot_digest" "config.protected_registry_policy_digest" "config.last_consumed_sequence")
    if(AUTHORITY_SOURCE MATCHES "${FORMER_SOURCE}")
        message(FATAL_ERROR "former caller-trusted registry source access remains: ${FORMER_SOURCE}")
    endif()
endforeach()
if(AUTHORITY_HEADER MATCHES "std::array<uint8_t,[^>]*>[^;]*registry[^;]*;" OR AUTHORITY_HEADER MATCHES "protected_registry_snapshot_data_")
    message(FATAL_ERROR "authority must not retain registry secret or borrowed snapshot bytes")
endif()
