#pragma once

#include "halofpx-context-store-bootstrap-material.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <memory>

namespace halofpx {

using context_store_bootstrap_anchor_synthetic_id = std::array<uint8_t, 32>;

struct context_store_bootstrap_anchor_synthetic_policy {
    context_store_format_digest anchor_root_identity {}, material_root_identity {}, registry_root_identity {};
    std::array<uint8_t, 16> store_uuid {};
    context_store_format_digest namespace_id {}, checkpoint_lineage_id {};
    uint64_t policy_epoch = 0, manifest_key_generation = 0, writer_authority_epoch = 0;
    context_store_registered_id anchor_authentication_key_id, durability_policy_id;
    uint64_t anchor_authentication_key_generation = 0;
    uint64_t maximum_anchor_envelope_bytes = context_store_anchor_max_bytes;
};

struct context_store_bootstrap_anchor_synthetic_request {
    context_store_bootstrap_anchor_synthetic_id anchor_attempt_id {};
};

enum class context_store_bootstrap_anchor_synthetic_backend_outcome : uint8_t {
    created_backend_claim,
    already_present_no_create,
    anchor_conflict,
    writer_busy,
    read_only,
    storage_error,
    synchronization_error,
    definitely_aborted,
    malformed_response,
    incomplete,
    unconfirmed_close,
    uncertain,
    late_completion_risk,
};

struct context_store_bootstrap_anchor_synthetic_operation {
    context_store_bootstrap_anchor_synthetic_id anchor_attempt_id {};
    context_store_format_digest anchor_root_identity {}, material_root_identity {}, registry_root_identity {};
    context_store_format_digest root_policy_commitment {}, material_source_commitment {}, create_operation_commitment {};
    context_store_format_digest selected_manifest_digest {}, proposed_anchor_digest {};
    context_store_authenticated_anchor proposed_anchor;
};

class context_store_bootstrap_anchor_synthetic_witness {
public:
    context_store_bootstrap_anchor_synthetic_witness() = default;
    context_store_bootstrap_anchor_synthetic_witness(const context_store_bootstrap_anchor_synthetic_witness &) = delete;
    context_store_bootstrap_anchor_synthetic_witness & operator=(const context_store_bootstrap_anchor_synthetic_witness &) = delete;
    context_store_bootstrap_anchor_synthetic_witness(context_store_bootstrap_anchor_synthetic_witness &&) noexcept = default;
    context_store_bootstrap_anchor_synthetic_witness & operator=(context_store_bootstrap_anchor_synthetic_witness &&) noexcept = default;
    context_store_bootstrap_anchor_synthetic_backend_outcome outcome = context_store_bootstrap_anchor_synthetic_backend_outcome::uncertain;
    context_store_bootstrap_anchor_synthetic_id anchor_attempt_id {};
    context_store_format_digest anchor_root_identity {}, material_root_identity {}, registry_root_identity {};
    context_store_format_digest root_policy_commitment {}, material_source_commitment {}, create_operation_commitment {};
    context_store_format_digest selected_manifest_digest {};
    std::array<uint8_t, context_store_anchor_max_bytes> observed_current {};
    size_t observed_current_size = 0;
    context_store_format_digest durable_close_confirmation {};
};

enum class context_store_bootstrap_anchor_synthetic_status : uint8_t {
    created_backend_claim,
    already_present_no_create,
    anchor_conflict,
    invalid_source,
    invalid_request,
    writer_busy,
    read_only,
    storage_error,
    synchronization_error,
    definitely_aborted,
    attempt_replayed,
    history_exhausted,
    anchor_root_quarantined,
    resource_exhausted,
    visibility_uncertain,
};

class context_store_bootstrap_anchor_synthetic_backend;
class context_store_bootstrap_anchor_synthetic_reconciliation_coordinator;
class context_store_bootstrap_anchor_synthetic_test_access;

class context_store_bootstrap_anchor_synthetic_uncertain_handle {
public:
    context_store_bootstrap_anchor_synthetic_uncertain_handle() = default;
    context_store_bootstrap_anchor_synthetic_uncertain_handle(const context_store_bootstrap_anchor_synthetic_uncertain_handle &) = delete;
    context_store_bootstrap_anchor_synthetic_uncertain_handle & operator=(const context_store_bootstrap_anchor_synthetic_uncertain_handle &) = delete;
    context_store_bootstrap_anchor_synthetic_uncertain_handle(context_store_bootstrap_anchor_synthetic_uncertain_handle &&) noexcept;
    context_store_bootstrap_anchor_synthetic_uncertain_handle & operator=(context_store_bootstrap_anchor_synthetic_uncertain_handle &&) noexcept;
    bool valid() const noexcept { return valid_; }
    const context_store_bootstrap_anchor_synthetic_id * original_attempt_id() const noexcept { return valid_ ? &original_attempt_ : nullptr; }
    const context_store_format_digest * original_operation_commitment() const noexcept { return valid_ ? &original_operation_ : nullptr; }
private:
    bool valid_ = false;
    context_store_bootstrap_anchor_synthetic_backend * owner_ = nullptr;
    context_store_bootstrap_anchor_synthetic_id original_attempt_ {};
    context_store_format_digest original_operation_ {};
    friend class context_store_bootstrap_anchor_synthetic_backend;
    friend class context_store_bootstrap_anchor_synthetic_reconciliation_coordinator;
    friend class context_store_bootstrap_anchor_synthetic_test_access;
};

class context_store_bootstrap_anchor_synthetic_proof {
public:
    context_store_bootstrap_anchor_synthetic_proof() = default;
    context_store_bootstrap_anchor_synthetic_proof(const context_store_bootstrap_anchor_synthetic_proof &) = delete;
    context_store_bootstrap_anchor_synthetic_proof & operator=(const context_store_bootstrap_anchor_synthetic_proof &) = delete;
    context_store_bootstrap_anchor_synthetic_proof(context_store_bootstrap_anchor_synthetic_proof &&) noexcept;
    context_store_bootstrap_anchor_synthetic_proof & operator=(context_store_bootstrap_anchor_synthetic_proof &&) noexcept;
    bool valid() const noexcept { return valid_; }
    bool recovered() const noexcept { return valid_ && recovered_; }
    const context_store_bootstrap_material_synthetic_proof * material_proof() const noexcept { return valid_ ? &material_ : nullptr; }
    const context_store_bootstrap_anchor_synthetic_policy * fixed_policy() const noexcept { return valid_ ? &policy_ : nullptr; }
    const context_store_bootstrap_anchor_synthetic_id * anchor_attempt_id() const noexcept { return valid_ ? &anchor_attempt_ : nullptr; }
    const context_store_format_digest * root_policy_commitment() const noexcept { return valid_ ? &root_policy_ : nullptr; }
    const context_store_format_digest * material_source_commitment() const noexcept { return valid_ ? &material_source_ : nullptr; }
    const context_store_format_digest * create_operation_commitment() const noexcept { return valid_ ? &create_operation_ : nullptr; }
    const uint8_t * observed_anchor_data() const noexcept { return valid_ ? observed_.data() : nullptr; }
    size_t observed_anchor_size() const noexcept { return valid_ ? observed_size_ : 0; }
    const context_store_format_digest * durable_close_confirmation() const noexcept { return valid_ ? &close_ : nullptr; }
    uint8_t original_phase() const noexcept { return valid_ ? original_phase_ : 3; }
    const context_store_bootstrap_anchor_synthetic_id * reconciliation_attempt_id() const noexcept { return valid_ && recovered_ ? &reconciliation_attempt_ : nullptr; }
    const context_store_format_digest * reconciliation_commitment() const noexcept { return valid_ && recovered_ ? &reconciliation_commitment_ : nullptr; }
private:
    bool valid_ = false, recovered_ = false;
    uint8_t original_phase_ = 3;
    context_store_bootstrap_material_synthetic_proof material_;
    context_store_bootstrap_anchor_synthetic_policy policy_;
    context_store_bootstrap_anchor_synthetic_id anchor_attempt_ {}, reconciliation_attempt_ {};
    context_store_format_digest root_policy_ {}, material_source_ {}, create_operation_ {}, close_ {}, reconciliation_commitment_ {}, fence_confirmation_ {};
    std::array<uint8_t, context_store_anchor_max_bytes> observed_ {};
    size_t observed_size_ = 0;
    friend class context_store_bootstrap_anchor_synthetic_backend;
};

struct context_store_bootstrap_anchor_synthetic_result {
    context_store_bootstrap_anchor_synthetic_status status = context_store_bootstrap_anchor_synthetic_status::invalid_source;
    context_store_bootstrap_anchor_synthetic_proof proof;
    context_store_bootstrap_anchor_synthetic_uncertain_handle uncertain_handle;
    bool has_proof() const noexcept { return status == context_store_bootstrap_anchor_synthetic_status::created_backend_claim && proof.valid(); }
    bool has_uncertain_handle() const noexcept { return status == context_store_bootstrap_anchor_synthetic_status::visibility_uncertain && uncertain_handle.valid(); }
};

struct context_store_bootstrap_anchor_synthetic_reconciliation_request {
    context_store_bootstrap_anchor_synthetic_id reconciliation_attempt_id {};
};

enum class context_store_bootstrap_anchor_synthetic_reconciliation_backend_outcome : uint8_t {
    exact_present, absent, other_present, unreadable, malformed_response,
    unconfirmed_fence, synchronization_error, unconfirmed_close, uncertain, late_completion_risk,
};

struct context_store_bootstrap_anchor_synthetic_reconciliation_operation {
    context_store_bootstrap_anchor_synthetic_id reconciliation_attempt_id {}, original_anchor_attempt_id {};
    context_store_format_digest anchor_root_identity {}, material_root_identity {}, registry_root_identity {};
    context_store_format_digest reconciliation_commitment {}, original_create_operation_commitment {};
    context_store_format_digest root_policy_commitment {}, material_source_commitment {};
    uint8_t original_phase = 3;
    context_store_authenticated_anchor proposed_anchor;
};

class context_store_bootstrap_anchor_synthetic_reconciliation_witness {
public:
    context_store_bootstrap_anchor_synthetic_reconciliation_witness() = default;
    context_store_bootstrap_anchor_synthetic_reconciliation_witness(const context_store_bootstrap_anchor_synthetic_reconciliation_witness &) = delete;
    context_store_bootstrap_anchor_synthetic_reconciliation_witness & operator=(const context_store_bootstrap_anchor_synthetic_reconciliation_witness &) = delete;
    context_store_bootstrap_anchor_synthetic_reconciliation_witness(context_store_bootstrap_anchor_synthetic_reconciliation_witness &&) noexcept = default;
    context_store_bootstrap_anchor_synthetic_reconciliation_witness & operator=(context_store_bootstrap_anchor_synthetic_reconciliation_witness &&) noexcept = default;
    context_store_bootstrap_anchor_synthetic_reconciliation_backend_outcome outcome = context_store_bootstrap_anchor_synthetic_reconciliation_backend_outcome::uncertain;
    context_store_bootstrap_anchor_synthetic_reconciliation_operation echo;
    std::array<uint8_t, context_store_anchor_max_bytes> observed_current {};
    size_t observed_current_size = 0;
    uint8_t classification = 0;
    context_store_format_digest fence_confirmation {}, durable_close_confirmation {};
};

enum class context_store_bootstrap_anchor_synthetic_reconciliation_status : uint8_t {
    created_same_recovered_backend_claim,
    already_present_fenced_no_retry,
    definitely_not_created_fenced_no_retry,
    anchor_conflict,
    invalid_handle,
    invalid_request,
    attempt_replayed,
    not_reconcilable,
    anchor_root_quarantined,
    visibility_uncertain,
};

struct context_store_bootstrap_anchor_synthetic_reconciliation_result {
    context_store_bootstrap_anchor_synthetic_reconciliation_status status = context_store_bootstrap_anchor_synthetic_reconciliation_status::invalid_handle;
    context_store_bootstrap_anchor_synthetic_proof proof;
    bool has_proof() const noexcept { return status == context_store_bootstrap_anchor_synthetic_reconciliation_status::created_same_recovered_backend_claim && proof.valid(); }
};

class context_store_bootstrap_anchor_synthetic_backend {
public:
    explicit context_store_bootstrap_anchor_synthetic_backend(const context_store_bootstrap_anchor_synthetic_policy &) noexcept;
    virtual ~context_store_bootstrap_anchor_synthetic_backend();
    context_store_bootstrap_anchor_synthetic_backend(const context_store_bootstrap_anchor_synthetic_backend &) = delete;
    context_store_bootstrap_anchor_synthetic_backend & operator=(const context_store_bootstrap_anchor_synthetic_backend &) = delete;
    bool valid() const noexcept { return valid_; }
    bool quarantined() const noexcept { return quarantined_.load(std::memory_order_acquire); }
    const context_store_bootstrap_anchor_synthetic_policy & policy() const noexcept { return policy_; }
protected:
    void record_conclusive_pre_create_inspection(const context_store_bootstrap_anchor_synthetic_id &) noexcept;
    void record_create_linearized(const context_store_bootstrap_anchor_synthetic_id &) noexcept;
    virtual context_store_bootstrap_anchor_synthetic_witness inspect_create_if_absent_synchronize_and_durable_close(
        const context_store_bootstrap_anchor_synthetic_operation &) = 0;
    virtual context_store_bootstrap_anchor_synthetic_reconciliation_witness fence_original_observe_current_synchronize_and_durable_close(
        const context_store_bootstrap_anchor_synthetic_reconciliation_operation &) = 0;
private:
    enum class pending_state : uint8_t { none, create_active, positive_ready, uncertain, reconciled };
    context_store_bootstrap_anchor_synthetic_status execute_create(
        context_store_bootstrap_material_synthetic_proof &&,
        const context_store_bootstrap_anchor_synthetic_operation &,
        context_store_bootstrap_anchor_synthetic_witness &) noexcept;
    bool claim_positive(const context_store_bootstrap_anchor_synthetic_operation &,
        const context_store_bootstrap_anchor_synthetic_witness &,
        context_store_bootstrap_anchor_synthetic_proof &) noexcept;
    void make_uncertain_handle(context_store_bootstrap_anchor_synthetic_uncertain_handle &) noexcept;
    context_store_bootstrap_anchor_synthetic_reconciliation_status execute_reconciliation(
        context_store_bootstrap_anchor_synthetic_uncertain_handle &&,
        const context_store_bootstrap_anchor_synthetic_id &,
        context_store_bootstrap_anchor_synthetic_proof &) noexcept;
    context_store_bootstrap_anchor_synthetic_policy policy_;
    bool valid_ = false;
    std::mutex mutex_;
    std::atomic<bool> quarantined_ { false };
    static constexpr size_t maximum_terminal_attempts_ = 512;
    static bool fail_history_allocation_test_;
    std::unique_ptr<std::array<context_store_bootstrap_anchor_synthetic_id, maximum_terminal_attempts_>> terminal_attempts_;
    size_t terminal_attempt_count_ = 0;
    pending_state pending_state_ = pending_state::none;
    uint8_t original_phase_ = 3;
    context_store_bootstrap_anchor_synthetic_operation pending_operation_;
    context_store_bootstrap_material_synthetic_proof pending_material_;
    bool reconciliation_registered_ = false;
    context_store_bootstrap_anchor_synthetic_id reconciliation_attempt_ {};
    friend class context_store_bootstrap_anchor_synthetic_coordinator;
    friend class context_store_bootstrap_anchor_synthetic_reconciliation_coordinator;
    friend class context_store_bootstrap_anchor_synthetic_test_access;
};

class context_store_bootstrap_anchor_synthetic_coordinator {
public:
    explicit context_store_bootstrap_anchor_synthetic_coordinator(context_store_bootstrap_anchor_synthetic_backend & b) noexcept : backend_(b) {}
    context_store_bootstrap_anchor_synthetic_result create(context_store_bootstrap_material_synthetic_proof &&,
        const context_store_bootstrap_anchor_synthetic_request &) noexcept;
private:
    static void (*post_positive_test_hook_)();
    friend class context_store_bootstrap_anchor_synthetic_test_access;
    context_store_bootstrap_anchor_synthetic_backend & backend_;
};

class context_store_bootstrap_anchor_synthetic_reconciliation_coordinator {
public:
    explicit context_store_bootstrap_anchor_synthetic_reconciliation_coordinator(context_store_bootstrap_anchor_synthetic_backend & b) noexcept : backend_(b) {}
    context_store_bootstrap_anchor_synthetic_reconciliation_result reconcile(
        context_store_bootstrap_anchor_synthetic_uncertain_handle &&,
        const context_store_bootstrap_anchor_synthetic_reconciliation_request &) noexcept;
private:
    context_store_bootstrap_anchor_synthetic_backend & backend_;
};

const char * context_store_bootstrap_anchor_synthetic_status_name(context_store_bootstrap_anchor_synthetic_status) noexcept;
const char * context_store_bootstrap_anchor_synthetic_reconciliation_status_name(context_store_bootstrap_anchor_synthetic_reconciliation_status) noexcept;

} // namespace halofpx
