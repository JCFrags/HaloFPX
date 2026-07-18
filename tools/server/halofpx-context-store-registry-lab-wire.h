#pragma once

#include "halofpx-context-store-protected-registry-successor.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace halofpx {

constexpr size_t context_store_registry_lab_wire_max_bytes = 4096;
constexpr size_t context_store_registry_lab_credential_max_bytes = 180;

enum class context_store_registry_lab_kind : uint8_t {
    root = 0, head = 1, prepare = 2, close = 3, abort_record = 4, quarantine = 5,
};

enum class context_store_registry_lab_wire_status : uint8_t {
    authenticated_semantic_only,
    structural_rejection,
    invalid_credential,
    authentication_failed,
    semantic_rejection,
};

struct context_store_registry_lab_credential {
    context_store_registry_lab_credential() noexcept = default;
    ~context_store_registry_lab_credential() noexcept;
    context_store_registry_lab_credential(const context_store_registry_lab_credential &) = delete;
    context_store_registry_lab_credential & operator=(const context_store_registry_lab_credential &) = delete;
    context_store_registry_lab_credential(context_store_registry_lab_credential && other) noexcept;
    context_store_registry_lab_credential & operator=(context_store_registry_lab_credential && other) noexcept;
    void clear() noexcept;

    context_store_registered_id key_id;
    uint64_t generation = 0;
    std::array<uint8_t, 32> secret {};
};

struct context_store_registry_lab_expectation {
    context_store_format_digest root_id {};
    context_store_format_digest path_policy_commitment {};
    std::array<uint8_t, 16> store_uuid {};
    std::array<uint8_t, 16> filesystem_uuid {};
    context_store_registered_id registry_id;
    uint64_t registry_epoch = 0;
    uint64_t predecessor_high_water = 0;
    uint64_t predecessor_selector_generation = 0;
    uint64_t successor_selector_generation = 0;
    uint64_t mount_id = 0;
    uint64_t owner_uid = 0;
    uint64_t lock_st_dev = 0;
    uint64_t lock_st_ino = 0;
    context_store_format_digest initial_head_digest {};
    context_store_format_digest successor_head_digest {};
    context_store_format_digest predecessor_envelope_digest {};
    context_store_format_digest successor_envelope_digest {};
    context_store_format_digest prepare_digest {};
    context_store_format_digest operation_commitment {};
    context_store_format_digest attempt_id {};
    context_store_format_digest quarantine_event_id {};
    context_store_format_digest quarantine_previous_record_digest {};
    context_store_format_digest quarantine_head_digest {};
    bool quarantine_attributable = false;
    bool quarantine_has_previous_record = false;
    bool quarantine_has_head = false;
    uint64_t quarantine_reason = 0;
    uint64_t quarantine_phase = 0;
    uint64_t slot = 0;
    const uint8_t * predecessor = nullptr;
    size_t predecessor_size = 0;
    const uint8_t * successor = nullptr;
    size_t successor_size = 0;
};

struct context_store_registry_lab_wire_result {
    context_store_registry_lab_wire_status status = context_store_registry_lab_wire_status::structural_rejection;
    context_store_format_digest content_digest {};
    bool authenticated() const noexcept {
        return status == context_store_registry_lab_wire_status::authenticated_semantic_only;
    }
};

bool context_store_registry_lab_parse_credential_v1(
    const uint8_t * data, size_t size, context_store_registry_lab_credential & output) noexcept;

bool context_store_registry_lab_path_policy_v1(
    const uint8_t * parent, size_t parent_size,
    const uint8_t * root, size_t root_size,
    const std::array<uint8_t, 16> & filesystem_uuid,
    const std::array<uint8_t, 16> & subvolume_uuid,
    uint64_t mount_id, uint64_t st_dev, uint64_t owner_uid,
    context_store_format_digest & output) noexcept;

context_store_registry_lab_wire_result context_store_registry_lab_verify_v1(
    context_store_registry_lab_kind kind,
    const uint8_t * data, size_t size,
    const context_store_registry_lab_credential & credential,
    const context_store_registry_lab_expectation & expectation) noexcept;

const char * context_store_registry_lab_wire_status_name(context_store_registry_lab_wire_status status) noexcept;

} // namespace halofpx
