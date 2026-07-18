#pragma once

#include "halofpx-context-store-bootstrap-consumption.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace halofpx {

using context_store_bootstrap_material_synthetic_id = std::array<uint8_t, 32>;

struct context_store_bootstrap_material_synthetic_policy {
    context_store_format_digest material_root_identity {};
    context_store_format_digest registry_root_identity {};
    std::array<uint8_t, 16> store_uuid {};
    context_store_format_digest namespace_id {};
    context_store_format_digest checkpoint_lineage_id {};
    uint64_t policy_epoch = 0;
    uint64_t manifest_key_generation = 0;
    uint64_t writer_authority_epoch = 0;
    context_store_registered_id durability_policy_id;
    uint8_t manifest_durability_mode = 0;
    uint64_t maximum_source_object_count = 0;
    uint64_t maximum_frame_bytes = 0;
    uint64_t maximum_aggregate_frame_bytes = 0;
};

struct context_store_bootstrap_material_synthetic_request {
    context_store_bootstrap_material_synthetic_id material_attempt_id {};
    const uint8_t * manifest_envelope_data = nullptr;
    size_t manifest_envelope_size = 0;
};

enum class context_store_bootstrap_material_synthetic_backend_outcome : uint8_t {
    prepared_backend_claim,
    already_same_backend_claim,
    definitely_aborted,
    source_conflict,
    policy_conflict,
    writer_busy,
    object_collision,
    manifest_collision,
    no_space,
    quota_exhausted,
    reserve_exhausted,
    read_only,
    storage_error,
    synchronization_error,
    malformed_response,
    incomplete,
    unconfirmed_close,
    uncertain,
    late_completion_risk,
};

struct context_store_bootstrap_material_synthetic_operation {
    context_store_bootstrap_material_synthetic_id material_attempt_id {};
    context_store_format_digest material_root_identity {}, registry_root_identity {};
    context_store_format_digest root_policy_commitment {}, authority_source_commitment {};
    context_store_format_digest source_set_commitment {}, material_set_commitment {};
    context_store_format_digest operation_commitment {}, selected_manifest_digest {};
    context_store_format_digest proposed_anchor_envelope_digest {};
    std::array<char, 23> provenance_tag {};
    size_t provenance_tag_size = 0;
    context_store_bootstrap_consumption_id original_consumption_attempt {};
    context_store_format_digest original_consumption_operation {}, command_id {}, authorization_token_digest {};
    context_store_format_digest plan_commitment {}, authority_snapshot_commitment {};
    uint64_t authorization_sequence = 0;
    context_store_authenticated_protected_registry_successor authenticated_successor;
    context_store_authenticated_anchor authenticated_proposed_anchor;
    bool reconciled_source = false;
    context_store_bootstrap_reconciliation_id reconciliation_attempt {};
    context_store_format_digest reconciliation_commitment {};
    std::vector<uint8_t> observed_successor;
    std::vector<uint8_t> manifest_envelope;
};

class context_store_bootstrap_material_synthetic_witness {
public:
    context_store_bootstrap_material_synthetic_witness() = default;
    context_store_bootstrap_material_synthetic_witness(const context_store_bootstrap_material_synthetic_witness &) = delete;
    context_store_bootstrap_material_synthetic_witness & operator=(const context_store_bootstrap_material_synthetic_witness &) = delete;
    context_store_bootstrap_material_synthetic_witness(context_store_bootstrap_material_synthetic_witness &&) noexcept = default;
    context_store_bootstrap_material_synthetic_witness & operator=(context_store_bootstrap_material_synthetic_witness &&) noexcept = default;

    context_store_bootstrap_material_synthetic_backend_outcome outcome =
        context_store_bootstrap_material_synthetic_backend_outcome::uncertain;
    context_store_bootstrap_material_synthetic_id material_attempt_id {};
    context_store_format_digest material_root_identity {}, registry_root_identity {};
    context_store_format_digest root_policy_commitment {}, authority_source_commitment {};
    context_store_format_digest source_set_commitment {}, material_set_commitment {};
    context_store_format_digest operation_commitment {};
    std::vector<std::vector<uint8_t>> observed_frames;
    std::vector<uint8_t> observed_manifest_envelope;
    context_store_format_digest durable_close_confirmation {};
};

enum class context_store_bootstrap_material_synthetic_status : uint8_t {
    prepared_backend_claim,
    already_same_backend_claim,
    invalid_source,
    invalid_request,
    source_conflict,
    policy_conflict,
    writer_busy,
    object_collision,
    manifest_collision,
    no_space,
    quota_exhausted,
    reserve_exhausted,
    read_only,
    storage_error,
    synchronization_error,
    definitely_aborted,
    attempt_replayed,
    history_exhausted,
    material_root_quarantined,
    resource_exhausted,
    visibility_uncertain,
};

class context_store_bootstrap_material_synthetic_proof {
public:
    context_store_bootstrap_material_synthetic_proof() = default;
    context_store_bootstrap_material_synthetic_proof(const context_store_bootstrap_material_synthetic_proof &) = delete;
    context_store_bootstrap_material_synthetic_proof & operator=(const context_store_bootstrap_material_synthetic_proof &) = delete;
    context_store_bootstrap_material_synthetic_proof(context_store_bootstrap_material_synthetic_proof &&) noexcept;
    context_store_bootstrap_material_synthetic_proof & operator=(context_store_bootstrap_material_synthetic_proof &&) noexcept;
    bool valid() const noexcept { return valid_; }
    const context_store_bootstrap_material_synthetic_id * material_attempt_id() const noexcept { return valid_ ? &attempt_ : nullptr; }
    const context_store_format_digest * material_root_identity() const noexcept { return valid_ ? &material_root_ : nullptr; }
    const context_store_format_digest * registry_root_identity() const noexcept { return valid_ ? &registry_root_ : nullptr; }
    const context_store_format_digest * root_policy_commitment() const noexcept { return valid_ ? &policy_ : nullptr; }
    const context_store_bootstrap_material_synthetic_policy * fixed_policy() const noexcept { return valid_ ? &fixed_policy_ : nullptr; }
    const context_store_format_digest * authority_source_commitment() const noexcept { return valid_ ? &authority_ : nullptr; }
    const context_store_format_digest * source_set_commitment() const noexcept { return valid_ ? &source_set_ : nullptr; }
    const context_store_format_digest * material_set_commitment() const noexcept { return valid_ ? &material_set_ : nullptr; }
    const context_store_format_digest * operation_commitment() const noexcept { return valid_ ? &operation_ : nullptr; }
    const context_store_authenticated_anchor * proposed_anchor() const noexcept { return valid_ ? &anchor_ : nullptr; }
    const context_store_bootstrap_consumption_proof * direct_source_proof() const noexcept { return valid_ && direct_source_.valid() ? &direct_source_ : nullptr; }
    const context_store_bootstrap_recovered_consumption_proof * recovered_source_proof() const noexcept { return valid_ && recovered_source_.valid() ? &recovered_source_ : nullptr; }
    const uint8_t * manifest_envelope_data() const noexcept { return valid_ ? manifest_.data() : nullptr; }
    size_t manifest_envelope_size() const noexcept { return valid_ ? manifest_.size() : 0; }
    size_t observed_frame_count() const noexcept { return valid_ ? observed_frames_.size() : 0; }
    size_t descriptor_count() const noexcept { return valid_ ? descriptors_.size() : 0; }
    const context_store_object_reference * descriptor(size_t i) const noexcept { return valid_ && i < descriptors_.size() ? &descriptors_[i] : nullptr; }
    const uint8_t * observed_frame_data(size_t i) const noexcept { return valid_ && i < observed_frames_.size() ? observed_frames_[i].data() : nullptr; }
    size_t observed_frame_size(size_t i) const noexcept { return valid_ && i < observed_frames_.size() ? observed_frames_[i].size() : 0; }
    const context_store_format_digest * durable_close_confirmation() const noexcept { return valid_ ? &close_ : nullptr; }
    context_store_bootstrap_material_synthetic_backend_outcome classified_outcome() const noexcept { return outcome_; }
private:
    bool valid_ = false;
    context_store_bootstrap_consumption_proof direct_source_;
    context_store_bootstrap_recovered_consumption_proof recovered_source_;
    context_store_bootstrap_material_synthetic_id attempt_ {};
    context_store_format_digest material_root_ {}, registry_root_ {}, policy_ {}, authority_ {}, source_set_ {}, material_set_ {}, operation_ {};
    context_store_bootstrap_material_synthetic_policy fixed_policy_;
    context_store_authenticated_anchor anchor_;
    std::vector<uint8_t> manifest_;
    std::vector<std::vector<uint8_t>> observed_frames_;
    std::vector<context_store_object_reference> descriptors_;
    context_store_format_digest close_ {};
    context_store_bootstrap_material_synthetic_backend_outcome outcome_ = context_store_bootstrap_material_synthetic_backend_outcome::uncertain;
    friend class context_store_bootstrap_material_synthetic_coordinator;
};

struct context_store_bootstrap_material_synthetic_result {
    context_store_bootstrap_material_synthetic_status status = context_store_bootstrap_material_synthetic_status::invalid_source;
    context_store_bootstrap_material_synthetic_proof proof;
    bool has_proof() const noexcept {
        return (status == context_store_bootstrap_material_synthetic_status::prepared_backend_claim ||
                status == context_store_bootstrap_material_synthetic_status::already_same_backend_claim) && proof.valid();
    }
};

class context_store_bootstrap_material_synthetic_backend {
public:
    context_store_bootstrap_material_synthetic_backend(
        const context_store_bootstrap_material_synthetic_policy &,
        std::vector<std::vector<uint8_t>> && source_frames) noexcept;
    virtual ~context_store_bootstrap_material_synthetic_backend();
    context_store_bootstrap_material_synthetic_backend(const context_store_bootstrap_material_synthetic_backend &) = delete;
    context_store_bootstrap_material_synthetic_backend & operator=(const context_store_bootstrap_material_synthetic_backend &) = delete;
    bool valid() const noexcept { return valid_; }
    bool quarantined() const noexcept { return quarantined_.load(std::memory_order_acquire); }
    const context_store_bootstrap_material_synthetic_policy & policy() const noexcept { return policy_; }
protected:
    size_t source_frame_count() const noexcept { return source_frames_.size(); }
    const uint8_t * source_frame_data(size_t i) const noexcept { return i < source_frames_.size() ? source_frames_[i].data() : nullptr; }
    size_t source_frame_size(size_t i) const noexcept { return i < source_frames_.size() ? source_frames_[i].size() : 0; }
    virtual context_store_bootstrap_material_synthetic_witness prepare_exact_material_set_and_durable_close(
        const context_store_bootstrap_material_synthetic_operation &) = 0;
private:
    context_store_bootstrap_material_synthetic_status execute(
        const context_store_bootstrap_material_synthetic_operation &,
        const context_store_parsed_manifest &,
        context_store_bootstrap_material_synthetic_witness &) noexcept;
    context_store_bootstrap_material_synthetic_policy policy_;
    const std::vector<std::vector<uint8_t>> source_frames_;
    bool valid_ = false;
    std::mutex mutex_;
    std::atomic<bool> quarantined_ { false };
    static constexpr size_t maximum_terminal_attempts_ = 512;
    std::array<context_store_bootstrap_material_synthetic_id, maximum_terminal_attempts_> terminal_attempts_ {};
    size_t terminal_attempt_count_ = 0;
    context_store_format_digest stable_authority_source_ {}, stable_root_policy_ {}, stable_material_set_ {};
    bool has_stable_binding_ = false;
    friend class context_store_bootstrap_material_synthetic_coordinator;
};

class context_store_bootstrap_material_synthetic_coordinator {
public:
    explicit context_store_bootstrap_material_synthetic_coordinator(context_store_bootstrap_material_synthetic_backend & backend) noexcept : backend_(backend) {}
    context_store_bootstrap_material_synthetic_result prepare(
        context_store_bootstrap_consumption_proof &&,
        const context_store_bootstrap_material_synthetic_request &) noexcept;
    context_store_bootstrap_material_synthetic_result prepare(
        context_store_bootstrap_recovered_consumption_proof &&,
        const context_store_bootstrap_material_synthetic_request &) noexcept;
private:
    static void (*post_positive_test_hook_)();
    friend class context_store_bootstrap_material_synthetic_test_access;
    template<class Proof>
    context_store_bootstrap_material_synthetic_result prepare_owned(
        Proof &&,
        const context_store_bootstrap_material_synthetic_request &,
        bool recovered) noexcept;
    context_store_bootstrap_material_synthetic_result prepare_direct(
        context_store_bootstrap_consumption_proof &&,
        const context_store_bootstrap_material_synthetic_request &) noexcept;
    context_store_bootstrap_material_synthetic_result prepare_recovered(
        context_store_bootstrap_recovered_consumption_proof &&,
        const context_store_bootstrap_material_synthetic_request &) noexcept;
    context_store_bootstrap_material_synthetic_backend & backend_;
};

const char * context_store_bootstrap_material_synthetic_status_name(context_store_bootstrap_material_synthetic_status) noexcept;

} // namespace halofpx
