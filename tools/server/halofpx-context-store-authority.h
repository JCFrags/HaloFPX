#pragma once

#include "halofpx-context-store-bootstrap-token.h"
#include "halofpx-context-store-anchor.h"
#include "halofpx-context-store-protected-registry.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace halofpx {

struct context_store_bootstrap_authority_config {
    context_store_anchor_key_record anchor_signing_key;
    context_store_bootstrap_admin_key_record bootstrap_admin_key;
    context_store_manifest_key_record manifest_authentication_key;
    context_store_protected_registry_key_record protected_registry_authentication_key;
    context_store_compatibility_expectation trusted_compatibility;
    std::array<uint8_t, 16> store_uuid {};
    context_store_format_digest namespace_id {};
    uint64_t policy_epoch = 0;
    context_store_format_digest checkpoint_lineage_id {};
    uint64_t manifest_key_generation = 0;
    uint64_t authority_epoch = 0;
    // Borrowed synchronously. Construction retains neither these exact bytes
    // nor the registry authentication secret.
    const uint8_t * protected_registry_snapshot_data = nullptr;
    size_t protected_registry_snapshot_size = 0;
};

struct context_store_bootstrap_request {
    // Borrowed synchronously. Planning never retains these bytes.
    const uint8_t * manifest_data = nullptr;
    size_t manifest_size = 0;
    const uint8_t * authorization_token_data = nullptr;
    size_t authorization_token_size = 0;
};

class context_store_bootstrap_authority;
struct context_store_bootstrap_result;

// An opaque, memory-only authorization plan. It authorizes no I/O and carries
// no durable replay assertion. Only an authorized result can expose one.
class context_store_bootstrap_plan {
public:
    context_store_bootstrap_plan() = default;

    bool authorized() const noexcept { return authorized_; }
    const context_store_bootstrap_command_id * command_id() const noexcept {
        return authorized_ ? &command_id_ : nullptr;
    }
    uint64_t authorization_sequence() const noexcept { return authorized_ ? authorization_sequence_ : 0; }
    size_t object_count() const noexcept { return authorized_ ? object_count_ : 0; }
    const context_store_format_digest * selected_manifest_digest() const noexcept {
        return authorized_ ? &selected_manifest_digest_ : nullptr;
    }
    const context_store_format_digest * authority_snapshot_commitment() const noexcept {
        return authorized_ ? &authority_snapshot_commitment_ : nullptr;
    }
    const context_store_format_digest * authorization_token_digest() const noexcept {
        return authorized_ ? &authorization_token_digest_ : nullptr;
    }
    const context_store_format_digest * plan_commitment() const noexcept {
        return authorized_ ? &plan_commitment_ : nullptr;
    }
    const context_store_registered_id * bootstrap_admin_key_id() const noexcept {
        return authorized_ ? &bootstrap_admin_key_id_ : nullptr;
    }
    uint64_t bootstrap_admin_key_generation() const noexcept {
        return authorized_ ? bootstrap_admin_key_generation_ : 0;
    }
    const context_store_authenticated_anchor * anchor() const noexcept {
        return authorized_ ? &anchor_ : nullptr;
    }

private:
    bool authorized_ = false;
    context_store_bootstrap_command_id command_id_ {};
    uint64_t authorization_sequence_ = 0;
    size_t object_count_ = 0;
    context_store_format_digest selected_manifest_digest_ {};
    context_store_format_digest authority_snapshot_commitment_ {};
    context_store_format_digest authorization_token_digest_ {};
    context_store_format_digest plan_commitment_ {};
    context_store_registered_id bootstrap_admin_key_id_;
    uint64_t bootstrap_admin_key_generation_ = 0;
    context_store_authenticated_anchor anchor_;

    friend class context_store_bootstrap_authority;
};

enum class context_store_bootstrap_status : uint8_t {
    authorized_unexecuted,
    invalid_authority,
    invalid_request,
    authorization_rejected,
    manifest_rejected,
    signing_failed,
};

struct context_store_bootstrap_result {
    context_store_bootstrap_status status = context_store_bootstrap_status::invalid_authority;

    bool has_authorized_plan() const noexcept {
        return status == context_store_bootstrap_status::authorized_unexecuted && plan_.authorized();
    }
    const context_store_bootstrap_plan * authorized_plan() const noexcept {
        return has_authorized_plan() ? &plan_ : nullptr;
    }

private:
    context_store_bootstrap_plan plan_;
    friend class context_store_bootstrap_authority;
};

// A bounded authority snapshot. It synchronously copies only the three keys
// needed for planning, verifies the transient fourth registry key and borrowed
// snapshot, exposes no key bytes, and wipes private storage on destruction.
// Planning is deterministic, stateless, and performs no I/O.
class context_store_bootstrap_authority {
public:
    explicit context_store_bootstrap_authority(
        const context_store_bootstrap_authority_config & config) noexcept;
    ~context_store_bootstrap_authority() noexcept;

    context_store_bootstrap_authority(const context_store_bootstrap_authority &) = delete;
    context_store_bootstrap_authority & operator=(const context_store_bootstrap_authority &) = delete;
    context_store_bootstrap_authority(context_store_bootstrap_authority &&) = delete;
    context_store_bootstrap_authority & operator=(context_store_bootstrap_authority &&) = delete;

    bool valid() const noexcept { return valid_; }
    const context_store_format_digest * authority_scope_commitment() const noexcept {
        return valid_ ? &authority_scope_commitment_ : nullptr;
    }
    context_store_bootstrap_result plan(const context_store_bootstrap_request & request) const noexcept;

private:
    bool valid_ = false;
    context_store_registered_id anchor_key_id_;
    uint64_t anchor_key_generation_ = 0;
    std::array<uint8_t, context_store_master_key_max_bytes> anchor_key_ {};
    size_t anchor_key_size_ = 0;
    context_store_registered_id admin_key_id_;
    uint64_t admin_key_generation_ = 0;
    std::array<uint8_t, context_store_master_key_max_bytes> admin_key_ {};
    size_t admin_key_size_ = 0;
    context_store_registered_id manifest_key_id_;
    uint64_t manifest_key_generation_ = 0;
    std::array<uint8_t, context_store_master_key_max_bytes> manifest_key_ {};
    size_t manifest_key_size_ = 0;
    context_store_compatibility_expectation trusted_compatibility_;
    context_store_anchor_body bootstrap_body_;
    context_store_registered_id protected_registry_id_;
    uint64_t protected_registry_epoch_ = 0;
    context_store_format_digest protected_registry_snapshot_digest_ {};
    context_store_format_digest protected_registry_policy_digest_ {};
    uint64_t expected_authorization_sequence_ = 0;
    context_store_format_digest authority_scope_commitment_ {};
    context_store_format_digest snapshot_commitment_ {};
};

// Deterministic public descriptor. No key bytes or secret-derived values enter
// this commitment; an offline token issuer can compute it independently.
bool context_store_bootstrap_authority_scope_commitment(
    const context_store_bootstrap_authority_config & config,
    context_store_format_digest & commitment) noexcept;

bool context_store_bootstrap_authority_base_scope_commitment(
    const context_store_bootstrap_authority_config & config,
    context_store_format_digest & commitment) noexcept;

const char * context_store_bootstrap_status_name(context_store_bootstrap_status status) noexcept;

} // namespace halofpx
