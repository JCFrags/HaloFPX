#pragma once

#include "halofpx-context-store-authority.h"
#include "halofpx-context-store-protected-registry-successor.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>

namespace halofpx {
using context_store_bootstrap_consumption_id = std::array<uint8_t,32>;

struct context_store_bootstrap_consumption_operation {
    context_store_format_digest root_identity {};
    context_store_bootstrap_consumption_id attempt_id {};
    context_store_authenticated_protected_registry predecessor;
    context_store_authenticated_protected_registry_successor successor;
    context_store_authenticated_anchor proposed_anchor;
    uint64_t authorization_sequence = 0;
    context_store_format_digest command_id {}, token_digest {}, plan_commitment {}, authority_snapshot_commitment {};
    context_store_format_digest selected_manifest_digest {}, proposed_anchor_envelope_digest {}, operation_commitment {};
};

enum class context_store_bootstrap_backend_outcome : uint8_t {
    advanced_durable, already_same_durable, definitely_not_applied, stale_predecessor,
    conflict, attempt_replayed, malformed_response, uncertain, late_completion_risk,
};

struct context_store_bootstrap_backend_result {
    context_store_bootstrap_backend_outcome outcome = context_store_bootstrap_backend_outcome::uncertain;
    // Required exact-current witness for both positive outcomes. The wrapper
    // compares every authenticated envelope byte before admitting success.
    context_store_authenticated_protected_registry_successor observed_current;
};

using context_store_bootstrap_reconciliation_id = std::array<uint8_t, 32>;
constexpr size_t context_store_bootstrap_reconciliation_witness_max_bytes = 1024;

struct context_store_bootstrap_reconciliation_operation {
    context_store_format_digest root_identity {};
    context_store_bootstrap_reconciliation_id reconciliation_attempt_id {};
    context_store_bootstrap_consumption_id original_attempt_id {};
    context_store_format_digest original_operation_commitment {};
    context_store_authenticated_protected_registry predecessor;
    context_store_authenticated_protected_registry_successor successor;
    context_store_authenticated_anchor proposed_anchor;
    context_store_format_digest reconciliation_commitment {};
};

enum class context_store_bootstrap_reconciliation_backend_outcome : uint8_t {
    authoritative_present, authoritative_absent, unreadable, incomplete,
    unconfirmed_fence, malformed_response, uncertain, late_completion_risk,
    attempt_replayed, not_reconcilable, root_quarantined,
};

struct context_store_bootstrap_reconciliation_backend_result {
    context_store_bootstrap_reconciliation_backend_outcome outcome =
        context_store_bootstrap_reconciliation_backend_outcome::uncertain;
    context_store_format_digest root_identity {};
    context_store_bootstrap_reconciliation_id reconciliation_attempt_id {};
    context_store_format_digest reconciliation_commitment {};
    context_store_bootstrap_consumption_id original_attempt_id {};
    context_store_format_digest original_operation_commitment {};
    std::array<uint8_t, context_store_bootstrap_reconciliation_witness_max_bytes> observed_current {};
    size_t observed_current_size = 0;
};

// One instance owns one nonzero root and the terminal attempt/quarantine state
// for its whole lifetime. Derived implementations provide only the atomic,
// durable exact-envelope primitive; this wrapper owns fencing classification.
class context_store_bootstrap_consumption_backend {
public:
    explicit context_store_bootstrap_consumption_backend(const context_store_format_digest & root) noexcept;
    virtual ~context_store_bootstrap_consumption_backend();
    context_store_bootstrap_consumption_backend(const context_store_bootstrap_consumption_backend&)=delete;
    context_store_bootstrap_consumption_backend&operator=(const context_store_bootstrap_consumption_backend&)=delete;
    const context_store_format_digest & root_identity() const noexcept { return root_; }
    bool quarantined() const noexcept { return quarantined_.load(std::memory_order_acquire); }
protected:
    virtual context_store_bootstrap_backend_result compare_and_advance(const context_store_bootstrap_consumption_operation &) = 0;
    // This is one atomic root operation: fence/join the exact original attempt,
    // then return the exact current head from the same linearization point.
    virtual context_store_bootstrap_reconciliation_backend_result fence_original_and_read_current(
        const context_store_bootstrap_reconciliation_operation &) {
        return {};
    }
private:
    context_store_bootstrap_backend_outcome execute(const context_store_bootstrap_consumption_operation &) noexcept;
    context_store_bootstrap_reconciliation_backend_result execute_reconciliation(
        const context_store_bootstrap_reconciliation_operation &) noexcept;
    context_store_format_digest root_ {};
    std::mutex mutex_;
    bool valid_root_ = false;
    std::atomic<bool> quarantined_ { false };
    static constexpr size_t max_terminal_attempts_ = 512;
    std::array<context_store_bootstrap_consumption_id, max_terminal_attempts_> terminal_attempts_ {};
    size_t terminal_attempt_count_ = 0;
    context_store_bootstrap_reconciliation_id reconciliation_attempt_ {};
    bool has_reconciliation_attempt_ = false;
    enum class reconciliation_phase : uint8_t {
        none, consumption_uncertain, reconciliation_in_flight,
        successor_recovered_terminal, predecessor_confirmed_terminal_no_retry,
        conflict_terminal, visibility_uncertain_terminal,
    };
    reconciliation_phase reconciliation_phase_ = reconciliation_phase::none;
    context_store_bootstrap_consumption_operation uncertain_operation_;
    friend class context_store_bootstrap_consumption_coordinator;
    friend class context_store_bootstrap_reconciliation_coordinator;
};

enum class context_store_bootstrap_consumption_status : uint8_t {
    advanced_unexecuted, already_consumed_same_unexecuted, invalid_request,
    stale_predecessor, conflict, attempt_replayed, definitely_not_applied,
    root_quarantined, visibility_uncertain,
};

class context_store_bootstrap_consumption_proof {
public:
    context_store_bootstrap_consumption_proof() = default;
    context_store_bootstrap_consumption_proof(const context_store_bootstrap_consumption_proof&)=delete;
    context_store_bootstrap_consumption_proof&operator=(const context_store_bootstrap_consumption_proof&)=delete;
    context_store_bootstrap_consumption_proof(context_store_bootstrap_consumption_proof&&other) noexcept;
    context_store_bootstrap_consumption_proof&operator=(context_store_bootstrap_consumption_proof&&other) noexcept;
    bool valid() const noexcept{return valid_;}
    const context_store_authenticated_protected_registry* predecessor()const noexcept{return valid_?&predecessor_:nullptr;}
    const context_store_authenticated_protected_registry_successor* successor()const noexcept{return valid_?&successor_:nullptr;}
    const context_store_authenticated_anchor* proposed_anchor()const noexcept{return valid_?&anchor_:nullptr;}
    const context_store_format_digest* root_identity()const noexcept{return valid_?&root_:nullptr;}
    const context_store_bootstrap_consumption_id* attempt_id()const noexcept{return valid_?&attempt_:nullptr;}
    const context_store_format_digest* operation_commitment()const noexcept{return valid_?&operation_commitment_:nullptr;}
    const context_store_format_digest* command_id()const noexcept{return valid_?&command_:nullptr;}
    const context_store_format_digest* authorization_token_digest()const noexcept{return valid_?&token_:nullptr;}
    const context_store_format_digest* plan_commitment()const noexcept{return valid_?&plan_:nullptr;}
    const context_store_format_digest* authority_snapshot_commitment()const noexcept{return valid_?&snapshot_:nullptr;}
    const context_store_format_digest* selected_manifest_digest()const noexcept{return valid_?&manifest_:nullptr;}
    uint64_t authorization_sequence()const noexcept{return valid_?sequence_:0;}
    context_store_bootstrap_backend_outcome classified_outcome()const noexcept{return valid_?classified_:context_store_bootstrap_backend_outcome::uncertain;}
private:
    bool valid_=false;
    context_store_authenticated_protected_registry predecessor_;
    context_store_authenticated_protected_registry_successor successor_;
    context_store_authenticated_anchor anchor_;
    context_store_format_digest root_{},attempt_{},operation_commitment_{},command_{},token_{},plan_{},snapshot_{},manifest_{};
    uint64_t sequence_=0;
    context_store_bootstrap_backend_outcome classified_=context_store_bootstrap_backend_outcome::definitely_not_applied;
    friend class context_store_bootstrap_consumption_coordinator;
};

struct context_store_bootstrap_consumption_result {
    context_store_bootstrap_consumption_status status=context_store_bootstrap_consumption_status::invalid_request;
    context_store_bootstrap_consumption_proof proof;
    bool has_proof()const noexcept{return (status==context_store_bootstrap_consumption_status::advanced_unexecuted||status==context_store_bootstrap_consumption_status::already_consumed_same_unexecuted)&&proof.valid();}
};

struct context_store_bootstrap_consumption_request {
    context_store_bootstrap_consumption_id attempt_id {};
    context_store_bootstrap_plan plan;
    context_store_authenticated_protected_registry predecessor;
    // Borrowed synchronously; never retained by coordinator, operation, backend wrapper, or proof.
    context_store_protected_registry_key_record registry_authentication_key;
};

class context_store_bootstrap_consumption_coordinator {
public:
    explicit context_store_bootstrap_consumption_coordinator(context_store_bootstrap_consumption_backend & backend) noexcept:backend_(backend){}
    context_store_bootstrap_consumption_result consume(const context_store_bootstrap_consumption_request &) noexcept;
private: context_store_bootstrap_consumption_backend&backend_;
};

enum class context_store_bootstrap_reconciliation_status : uint8_t {
    consumed_same_recovered_unexecuted,
    definitely_unconsumed_fenced_no_retry,
    conflict,
    invalid_request,
    attempt_replayed,
    not_reconcilable,
    root_quarantined,
    visibility_uncertain,
};

class context_store_bootstrap_recovered_consumption_proof {
public:
    context_store_bootstrap_recovered_consumption_proof() = default;
    context_store_bootstrap_recovered_consumption_proof(const context_store_bootstrap_recovered_consumption_proof &) = delete;
    context_store_bootstrap_recovered_consumption_proof & operator=(const context_store_bootstrap_recovered_consumption_proof &) = delete;
    context_store_bootstrap_recovered_consumption_proof(context_store_bootstrap_recovered_consumption_proof &&) noexcept;
    context_store_bootstrap_recovered_consumption_proof & operator=(context_store_bootstrap_recovered_consumption_proof &&) noexcept;
    bool valid() const noexcept { return valid_; }
    const context_store_authenticated_protected_registry * predecessor() const noexcept { return valid_ ? &predecessor_ : nullptr; }
    const context_store_authenticated_protected_registry_successor * successor() const noexcept { return valid_ ? &successor_ : nullptr; }
    const context_store_authenticated_anchor * proposed_anchor() const noexcept { return valid_ ? &anchor_ : nullptr; }
    const context_store_format_digest * root_identity() const noexcept { return valid_ ? &root_ : nullptr; }
    const context_store_bootstrap_consumption_id * original_attempt_id() const noexcept { return valid_ ? &original_attempt_ : nullptr; }
    const context_store_format_digest * original_operation_commitment() const noexcept { return valid_ ? &original_operation_ : nullptr; }
    const context_store_bootstrap_reconciliation_id * reconciliation_attempt_id() const noexcept { return valid_ ? &reconciliation_attempt_ : nullptr; }
    const context_store_format_digest * reconciliation_commitment() const noexcept { return valid_ ? &reconciliation_commitment_ : nullptr; }
    const context_store_format_digest * command_id() const noexcept { return valid_ ? &command_ : nullptr; }
    const context_store_format_digest * authorization_token_digest() const noexcept { return valid_ ? &token_ : nullptr; }
    const context_store_format_digest * plan_commitment() const noexcept { return valid_ ? &plan_ : nullptr; }
    const context_store_format_digest * authority_snapshot_commitment() const noexcept { return valid_ ? &snapshot_ : nullptr; }
    const context_store_format_digest * selected_manifest_digest() const noexcept { return valid_ ? &manifest_ : nullptr; }
    uint64_t authorization_sequence() const noexcept { return valid_ ? sequence_ : 0; }
    const uint8_t * observed_successor_data() const noexcept { return valid_ ? observed_.data() : nullptr; }
    size_t observed_successor_size() const noexcept { return valid_ ? observed_size_ : 0; }
    context_store_bootstrap_reconciliation_status classified_outcome() const noexcept {
        return valid_ ? classified_ : context_store_bootstrap_reconciliation_status::visibility_uncertain;
    }
    bool original_consumption_uncertain_confirmed() const noexcept { return valid_; }
private:
    bool valid_ = false;
    context_store_authenticated_protected_registry predecessor_;
    context_store_authenticated_protected_registry_successor successor_;
    context_store_authenticated_anchor anchor_;
    context_store_format_digest root_ {}, original_attempt_ {}, original_operation_ {}, reconciliation_attempt_ {}, reconciliation_commitment_ {};
    context_store_format_digest command_ {}, token_ {}, plan_ {}, snapshot_ {}, manifest_ {};
    uint64_t sequence_ = 0;
    std::array<uint8_t, context_store_bootstrap_reconciliation_witness_max_bytes> observed_ {};
    size_t observed_size_ = 0;
    context_store_bootstrap_reconciliation_status classified_ = context_store_bootstrap_reconciliation_status::visibility_uncertain;
    friend class context_store_bootstrap_reconciliation_coordinator;
};

struct context_store_bootstrap_reconciliation_request {
    context_store_bootstrap_consumption_id original_attempt_id {};
    context_store_bootstrap_reconciliation_id reconciliation_attempt_id {};
    context_store_bootstrap_plan plan;
    context_store_authenticated_protected_registry predecessor;
    // Borrowed synchronously and never retained.
    context_store_protected_registry_key_record registry_authentication_key;
};

struct context_store_bootstrap_reconciliation_result {
    context_store_bootstrap_reconciliation_status status = context_store_bootstrap_reconciliation_status::invalid_request;
    context_store_bootstrap_recovered_consumption_proof proof;
    bool has_proof() const noexcept {
        return status == context_store_bootstrap_reconciliation_status::consumed_same_recovered_unexecuted && proof.valid();
    }
};

class context_store_bootstrap_reconciliation_coordinator {
public:
    explicit context_store_bootstrap_reconciliation_coordinator(context_store_bootstrap_consumption_backend & backend) noexcept : backend_(backend) {}
    context_store_bootstrap_reconciliation_result reconcile(const context_store_bootstrap_reconciliation_request &) noexcept;
private:
    context_store_bootstrap_consumption_backend & backend_;
};

const char * context_store_bootstrap_consumption_status_name(context_store_bootstrap_consumption_status) noexcept;
const char * context_store_bootstrap_reconciliation_status_name(context_store_bootstrap_reconciliation_status) noexcept;
} // namespace halofpx
