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
private:
    context_store_bootstrap_backend_outcome execute(const context_store_bootstrap_consumption_operation &) noexcept;
    context_store_format_digest root_ {};
    std::mutex mutex_;
    bool valid_root_ = false;
    std::atomic<bool> quarantined_ { false };
    static constexpr size_t max_terminal_attempts_ = 512;
    std::array<context_store_bootstrap_consumption_id, max_terminal_attempts_> terminal_attempts_ {};
    size_t terminal_attempt_count_ = 0;
    friend class context_store_bootstrap_consumption_coordinator;
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

const char * context_store_bootstrap_consumption_status_name(context_store_bootstrap_consumption_status) noexcept;
} // namespace halofpx
