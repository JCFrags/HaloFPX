if (NOT DEFINED HALOFPX_SOURCE_DIR OR NOT IS_DIRECTORY "${HALOFPX_SOURCE_DIR}")
    message(FATAL_ERROR "HALOFPX_SOURCE_DIR must name the source tree")
endif()

set(contract_path "${HALOFPX_SOURCE_DIR}/docs/halofpx/contracts/context-store-v1.json")
file(READ "${contract_path}" contract)

foreach(member
        contract status feature_default persistent_reads_enabled
        persistent_writes_enabled legacy_server_hits_enabled
        admitted_state_profiles admitted_codecs compatibility_required
        scope format publication distributed failure_default
        quarantine_never_authoritative replay_anchor donor_formats)
    string(JSON member_type ERROR_VARIABLE json_error TYPE "${contract}" "${member}")
    if (json_error)
        message(FATAL_ERROR "L02 contract missing/invalid member ${member}: ${json_error}")
    endif()
endforeach()

foreach(false_member persistent_reads_enabled persistent_writes_enabled legacy_server_hits_enabled)
    string(JSON value GET "${contract}" "${false_member}")
    if (value)
        message(FATAL_ERROR "L02 contract unexpectedly enabled ${false_member}")
    endif()
endforeach()

foreach(empty_member admitted_state_profiles admitted_codecs)
    string(JSON length LENGTH "${contract}" "${empty_member}")
    if (NOT length EQUAL 0)
        message(FATAL_ERROR "L02 must admit zero ${empty_member}")
    endif()
endforeach()

function(require_json_value expected)
    string(JSON actual ERROR_VARIABLE value_error GET "${contract}" ${ARGN})
    if (value_error OR NOT actual STREQUAL "${expected}")
        string(JOIN "." path_text ${ARGN})
        message(FATAL_ERROR "L02 contract ${path_text}: expected '${expected}', got '${actual}' (${value_error})")
    endif()
endfunction()

function(require_json_boolean expected)
    string(JSON actual ERROR_VARIABLE value_error GET "${contract}" ${ARGN})
    if (value_error)
        string(JOIN "." path_text ${ARGN})
        message(FATAL_ERROR "L02 contract ${path_text}: ${value_error}")
    endif()
    if (expected AND NOT actual)
        message(FATAL_ERROR "L02 contract ${ARGN} must be true")
    elseif (NOT expected AND actual)
        message(FATAL_ERROR "L02 contract ${ARGN} must be false")
    endif()
endfunction()

string(JSON feature_default GET "${contract}" feature_default)
if (NOT feature_default STREQUAL "off")
    message(FATAL_ERROR "L02 feature default must be off")
endif()

require_json_value("design-only" status)
require_json_value("halofpx.context-store.l02.v1" contract)
require_json_value("authenticated-private-per-principal" scope default)
require_json_value("trusted-server-authn-policy" scope authority)
require_json_value("HMAC-SHA-256" scope namespace_kdf)
require_json_boolean(FALSE scope anonymous_persistence)
require_json_boolean(FALSE scope cross_principal_reuse)
require_json_boolean(FALSE scope shared_reuse)
require_json_boolean(FALSE scope implicit_scope_fallback)
require_json_value("new-namespace-old-read-disabled" scope key_rotation)

require_json_value("deterministic-CBOR-RFC8949" format manifest_encoding)
require_json_value("HaloFPX" format owner)
require_json_value("1" format major)
require_json_value("context-store-v1.cddl" format manifest_schema)
require_json_value("closed-integer-keys-no-extensions" format manifest_field_registry)
require_json_value("HMAC-SHA-256-protected-key-before-payload-decode" format manifest_authentication)
require_json_value("SHA-256" format digest)
require_json_value("48414c4f4f424a01" format object_magic_hex)
require_json_value("halofpx.object.v1" format object_domain)
require_json_value("reject" format unknown_fields)
require_json_value("reject" format duplicate_fields)
require_json_value("reject" format trailing_bytes)
require_json_value("1048576" format manifest_max_bytes)
require_json_value("16" format nesting_max)
require_json_value("128" format components_max)
require_json_value("16384" format metadata_value_max_bytes)
require_json_boolean(FALSE format indefinite_lengths)

require_json_value("1" publication writers_per_root)
require_json_boolean(FALSE publication destination_replacement)
require_json_value("protected-anchor-bound-authenticated-immutable-manifest" publication visibility_authority)
require_json_value("objects-manifest-anchor" publication anchor_linearization_order)
require_json_boolean(TRUE publication object_sync_before_visibility)
require_json_boolean(TRUE publication same_filesystem_atomic_publish)
require_json_boolean(TRUE publication directory_sync_required_where_supported)
require_json_value("closed" publication writer_gate)

require_json_boolean(TRUE distributed rank_local_objects)
require_json_boolean(TRUE distributed whole_generation_acceptance)
require_json_value("reject" distributed mixed_generation)
require_json_boolean(FALSE distributed state_payload_on_restore_control_plane)
require_json_value("mutually-authenticated-confidential-channel-plus-full-message-authenticator" distributed ready_authentication)
require_json_value("separate-compatible-checkpoint-or-recompute" distributed single_node_fallback)
require_json_value("miss-recompute" failure_default)
require_json_boolean(TRUE quarantine_never_authoritative)
require_json_value("per-store-namespace-policy-lineage-protected-manifest-predecessor-anchor" replay_anchor)
require_json_value("offline-bounded-read-only-inventory-only" donor_formats)

set(expected_compatibility
    model_bytes_and_shards model_metadata tokenizer_bytes_and_policy
    chat_template_bytes_renderer_and_rendered_output system_and_tool_context
    adapter_projector_set_and_order runtime_abi_and_build backend_and_device_abi
    quantization_and_kv_layout context_rope_window_and_position
    sampler_and_logits_processors grammar_parser_and_tool_state
    rng_state_and_counter target_draft_mtp_speculative_state
    topology_plan_rank_world_placement_epoch security_domain_and_scope_policy)
string(JSON compatibility_length LENGTH "${contract}" compatibility_required)
list(LENGTH expected_compatibility expected_compatibility_length)
if (NOT compatibility_length EQUAL expected_compatibility_length)
    message(FATAL_ERROR "L02 compatibility registry length changed")
endif()
set(compatibility_index 0)
foreach(expected IN LISTS expected_compatibility)
    require_json_value("${expected}" compatibility_required ${compatibility_index})
    math(EXPR compatibility_index "${compatibility_index} + 1")
endforeach()

set(schema_path "${HALOFPX_SOURCE_DIR}/docs/halofpx/contracts/context-store-v1.cddl")
file(READ "${schema_path}" schema)
file(SHA256 "${schema_path}" schema_sha256)
if (NOT schema_sha256 STREQUAL "a7c147b4a5ed11a3975d293b9b1a8cad61e7b1c884b3bd43a38085b829497369")
    message(FATAL_ERROR "reviewed L02 CDDL changed: ${schema_sha256}")
endif()
foreach(schema_marker
        "authenticated-manifest-v1 = {"
        "manifest-auth-input-v1 = {"
        "manifest-body-v1 = {"
        "compatibility-manifest = {"
        "compatibility-component-input-v1 ="
        "namespace-preimage-v1 = {"
        "authenticated-publication-anchor-v1 = {"
        "publication-anchor-auth-input-v1 = {"
        "publication-anchor-v1 = {"
        "authenticated-control-message-v1 = {"
        "control-auth-input-v1 = {"
        "control-message-v1 ="
        "ready-body-v1 = {"
        "commit-body-v1 = {"
        "abort-body-v1 = {")
    string(FIND "${schema}" "${schema_marker}" marker_position)
    if (marker_position EQUAL -1)
        message(FATAL_ERROR "L02 closed schema missing ${schema_marker}")
    endif()
endforeach()

string(JSON writer_gate GET "${contract}" publication writer_gate)
if (NOT writer_gate STREQUAL "closed")
    message(FATAL_ERROR "L02 writer gate must remain closed")
endif()

foreach(adr
        0001-complete-state-admission.md
        0002-scope-and-authority.md
        0003-target-owned-storage-format.md
        0004-publication-and-failure.md
        0005-distributed-ownership-and-threat-model.md)
    set(adr_path "${HALOFPX_SOURCE_DIR}/docs/halofpx/decisions/${adr}")
    if (NOT EXISTS "${adr_path}")
        message(FATAL_ERROR "missing L02 decision: ${adr}")
    endif()
    file(READ "${adr_path}" adr_text)
    string(FIND "${adr_text}" "Status: accepted for L02" status_position)
    if (status_position EQUAL -1)
        message(FATAL_ERROR "L02 decision is not accepted: ${adr}")
    endif()
endforeach()

message(STATUS "HaloFPX L02 contracts are present, parseable, and default-off")
