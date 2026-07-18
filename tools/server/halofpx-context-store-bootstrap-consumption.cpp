#include "halofpx-context-store-bootstrap-consumption.h"

#include <algorithm>
#include <array>
#include <cstring>

namespace halofpx { namespace {
constexpr char operation_domain[]="halofpx.bootstrap-consumption-operation.v1";
constexpr char reconciliation_domain[]="halofpx.bootstrap-consumption-reconciliation.v1";
constexpr char head_witness_domain[]="halofpx.registry-head-witness.v1";
void wipe(void*p,size_t n)noexcept{volatile uint8_t*b=(volatile uint8_t*)p;while(n--)*b++=0;}
template<size_t N>bool nz(const std::array<uint8_t,N>&a)noexcept{uint8_t v=0;for(auto x:a)v|=x;return v;}
bool eq(const context_store_format_digest&a,const context_store_format_digest&b)noexcept{uint8_t v=0;for(size_t i=0;i<32;++i)v|=a[i]^b[i];return !v;}
bool eqid(const context_store_registered_id&a,const context_store_registered_id&b)noexcept{return a.size==b.size&&std::equal(a.bytes.begin(),a.bytes.begin()+a.size,b.bytes.begin());}
bool append(uint8_t*out,size_t cap,size_t&n,const void*p,size_t z)noexcept{if(!p||z>cap-n)return false;std::memcpy(out+n,p,z);n+=z;return true;}
bool be64(uint8_t*out,size_t cap,size_t&n,uint64_t v)noexcept{uint8_t b[8];for(size_t i=0;i<8;++i)b[7-i]=(uint8_t)(v>>(i*8));return append(out,cap,n,b,8);}
bool operation_digest(const context_store_bootstrap_consumption_operation&o,context_store_format_digest&d)noexcept{
    if(!o.predecessor.body()||!o.successor.authenticated()||!o.proposed_anchor.authenticated()||!o.proposed_anchor.envelope_digest()||!eq(*o.proposed_anchor.envelope_digest(),o.proposed_anchor_envelope_digest))return false;
    std::array<uint8_t,2500>m{};size_t n=0;
    bool ok=append(m.data(),m.size(),n,operation_domain,sizeof(operation_domain))&&append(m.data(),m.size(),n,o.root_identity.data(),32)&&append(m.data(),m.size(),n,o.attempt_id.data(),32)&&
        be64(m.data(),m.size(),n,o.predecessor.envelope_size())&&append(m.data(),m.size(),n,o.predecessor.envelope_data(),o.predecessor.envelope_size())&&
        be64(m.data(),m.size(),n,o.successor.envelope_size())&&append(m.data(),m.size(),n,o.successor.envelope_data(),o.successor.envelope_size())&&
        be64(m.data(),m.size(),n,o.authorization_sequence)&&append(m.data(),m.size(),n,o.command_id.data(),32)&&append(m.data(),m.size(),n,o.token_digest.data(),32)&&
        append(m.data(),m.size(),n,o.plan_commitment.data(),32)&&append(m.data(),m.size(),n,o.authority_snapshot_commitment.data(),32)&&
        append(m.data(),m.size(),n,o.selected_manifest_digest.data(),32)&&append(m.data(),m.size(),n,o.proposed_anchor_envelope_digest.data(),32)&&
        context_store_sha256_bounded(m.data(),n,m.size(),d);wipe(m.data(),m.size());return ok;
}
bool same_attempt(const context_store_bootstrap_consumption_id&a,const context_store_bootstrap_consumption_id&b)noexcept{return eq(a,b);}
bool exact_successor(const context_store_authenticated_protected_registry_successor&a,const context_store_authenticated_protected_registry_successor&b)noexcept{return a.authenticated()&&b.authenticated()&&a.envelope_size()==b.envelope_size()&&a.envelope_digest()&&b.envelope_digest()&&eq(*a.envelope_digest(),*b.envelope_digest())&&std::equal(a.envelope_data(),a.envelope_data()+a.envelope_size(),b.envelope_data());}
bool exact_predecessor(const context_store_authenticated_protected_registry&a,const context_store_authenticated_protected_registry&b)noexcept{return a.body()&&b.body()&&a.envelope_size()==b.envelope_size()&&a.envelope_digest()&&b.envelope_digest()&&eq(*a.envelope_digest(),*b.envelope_digest())&&std::equal(a.envelope_data(),a.envelope_data()+a.envelope_size(),b.envelope_data());}
bool exact_anchor(const context_store_authenticated_anchor&a,const context_store_authenticated_anchor&b)noexcept{return a.authenticated()&&b.authenticated()&&a.envelope_size()==b.envelope_size()&&a.envelope_digest()&&b.envelope_digest()&&eq(*a.envelope_digest(),*b.envelope_digest())&&std::equal(a.envelope_data(),a.envelope_data()+a.envelope_size(),b.envelope_data());}
bool valid_key(const context_store_protected_registry_key_record&k)noexcept{return k.disposition==context_store_key_disposition::active&&k.key_id.size&&k.generation&&k.master_key.data&&k.master_key.size&&k.master_key.size<=context_store_master_key_max_bytes;}
bool reconciliation_digest(const context_store_bootstrap_reconciliation_operation&o,context_store_format_digest&d)noexcept{
    if(!o.predecessor.body()||!o.successor.authenticated()||!o.proposed_anchor.authenticated())return false;
    std::array<uint8_t,3500>m{};size_t n=0;
    bool ok=append(m.data(),m.size(),n,reconciliation_domain,sizeof(reconciliation_domain))&&append(m.data(),m.size(),n,o.root_identity.data(),32)&&
        append(m.data(),m.size(),n,o.reconciliation_attempt_id.data(),32)&&append(m.data(),m.size(),n,o.original_attempt_id.data(),32)&&
        append(m.data(),m.size(),n,o.original_operation_commitment.data(),32)&&be64(m.data(),m.size(),n,o.predecessor.envelope_size())&&
        append(m.data(),m.size(),n,o.predecessor.envelope_data(),o.predecessor.envelope_size())&&be64(m.data(),m.size(),n,o.successor.envelope_size())&&
        append(m.data(),m.size(),n,o.successor.envelope_data(),o.successor.envelope_size())&&be64(m.data(),m.size(),n,o.proposed_anchor.envelope_size())&&
        append(m.data(),m.size(),n,o.proposed_anchor.envelope_data(),o.proposed_anchor.envelope_size())&&context_store_sha256_bounded(m.data(),n,m.size(),d);
    wipe(m.data(),m.size());return ok;
}
bool exact_head(const uint8_t*observed,size_t observed_size,const uint8_t*expected,size_t expected_size)noexcept{
    if(!observed||!expected||observed_size!=expected_size||observed_size==0||observed_size>context_store_bootstrap_reconciliation_witness_max_bytes)return false;
    std::array<uint8_t,1100>a{},b{};size_t an=0,bn=0;context_store_format_digest ad{},bd{};
    const bool ok=append(a.data(),a.size(),an,head_witness_domain,sizeof(head_witness_domain))&&be64(a.data(),a.size(),an,observed_size)&&append(a.data(),a.size(),an,observed,observed_size)&&
        append(b.data(),b.size(),bn,head_witness_domain,sizeof(head_witness_domain))&&be64(b.data(),b.size(),bn,expected_size)&&append(b.data(),b.size(),bn,expected,expected_size)&&
        context_store_sha256_bounded(a.data(),an,a.size(),ad)&&context_store_sha256_bounded(b.data(),bn,b.size(),bd)&&eq(ad,bd)&&std::equal(observed,observed+observed_size,expected);
    wipe(a.data(),a.size());wipe(b.data(),b.size());wipe(ad.data(),ad.size());wipe(bd.data(),bd.size());return ok;
}
bool build_operation(const context_store_bootstrap_consumption_request&r,const context_store_format_digest&root,context_store_bootstrap_consumption_operation&op)noexcept{
    const auto*pb=r.predecessor.body();const auto*pd=r.predecessor.envelope_digest();
    if(!r.plan.authorized()||!pb||!pd||!valid_key(r.registry_authentication_key)||!nz(r.attempt_id)||!nz(root)||
       !r.plan.command_id()||!r.plan.authorization_token_digest()||!r.plan.plan_commitment()||!r.plan.authority_snapshot_commitment()||!r.plan.selected_manifest_digest()||!r.plan.expected_registry_snapshot_digest()||!r.plan.anchor()||
       !eq(*pd,*r.plan.expected_registry_snapshot_digest())||pb->last_consumed_sequence==UINT64_MAX||r.plan.authorization_sequence()!=pb->last_consumed_sequence+1||!nz(*r.plan.command_id()))return false;
    context_store_protected_registry_successor_body sb;sb.registry_id=pb->registry_id;sb.registry_epoch=pb->registry_epoch;sb.authority_base_scope_commitment=pb->authority_base_scope_commitment;sb.policy_commitment=pb->policy_commitment;sb.consumed_authorization_high_water=r.plan.authorization_sequence();sb.predecessor_snapshot_envelope_digest=*pd;sb.receipt.authorization_sequence=r.plan.authorization_sequence();sb.receipt.command_id=*r.plan.command_id();sb.receipt.authorization_token_digest=*r.plan.authorization_token_digest();sb.receipt.plan_commitment=*r.plan.plan_commitment();sb.receipt.selected_manifest_digest=*r.plan.selected_manifest_digest();sb.receipt.proposed_anchor_envelope_digest=*r.plan.anchor()->envelope_digest();
    std::array<uint8_t,context_store_protected_registry_successor_max_bytes>bytes{};auto encoded=context_store_encode_protected_registry_successor_v1(sb,r.registry_authentication_key,bytes.data(),bytes.size());wipe(bytes.data(),bytes.size());if(encoded.status!=context_store_protected_registry_successor_status::authenticated_unadmitted)return false;const auto*sc=encoded.authenticated_carrier();
    if(!sc||!r.predecessor.key_id()||!r.predecessor.key_continuity_commitment()||!sc->key_id()||!sc->key_continuity_commitment()||!eqid(*r.predecessor.key_id(),*sc->key_id())||r.predecessor.key_generation()!=sc->key_generation()||!eq(*r.predecessor.key_continuity_commitment(),*sc->key_continuity_commitment()))return false;
    op={};op.root_identity=root;op.attempt_id=r.attempt_id;op.predecessor=r.predecessor;op.successor=*sc;op.proposed_anchor=*r.plan.anchor();op.authorization_sequence=r.plan.authorization_sequence();op.command_id=*r.plan.command_id();op.token_digest=*r.plan.authorization_token_digest();op.plan_commitment=*r.plan.plan_commitment();op.authority_snapshot_commitment=*r.plan.authority_snapshot_commitment();op.selected_manifest_digest=*r.plan.selected_manifest_digest();op.proposed_anchor_envelope_digest=*r.plan.anchor()->envelope_digest();return operation_digest(op,op.operation_commitment);
}
} // namespace

context_store_bootstrap_consumption_backend::context_store_bootstrap_consumption_backend(const context_store_format_digest&r)noexcept:root_(r),valid_root_(nz(r)){}
context_store_bootstrap_consumption_backend::~context_store_bootstrap_consumption_backend()=default;
context_store_bootstrap_backend_outcome context_store_bootstrap_consumption_backend::execute(const context_store_bootstrap_consumption_operation&o)noexcept{
    std::lock_guard<std::mutex>l(mutex_);if(!valid_root_||quarantined())return context_store_bootstrap_backend_outcome::uncertain;
    context_store_format_digest recomputed{};if(!eq(root_,o.root_identity)||!operation_digest(o,recomputed)||!eq(recomputed,o.operation_commitment)){quarantined_.store(true,std::memory_order_release);return context_store_bootstrap_backend_outcome::malformed_response;}
    if(std::any_of(terminal_attempts_.begin(),terminal_attempts_.begin()+terminal_attempt_count_,[&](const auto&i){return same_attempt(i,o.attempt_id);})){return context_store_bootstrap_backend_outcome::attempt_replayed;}
    if(terminal_attempt_count_==terminal_attempts_.size()){quarantined_.store(true,std::memory_order_release);return context_store_bootstrap_backend_outcome::uncertain;}
    terminal_attempts_[terminal_attempt_count_++]=o.attempt_id;
    context_store_bootstrap_backend_result response;
    try{response=compare_and_advance(o);}catch(...){uncertain_operation_=o;reconciliation_phase_=reconciliation_phase::consumption_uncertain;quarantined_.store(true,std::memory_order_release);return context_store_bootstrap_backend_outcome::uncertain;}
    const auto result=response.outcome;
    if(result==context_store_bootstrap_backend_outcome::advanced_durable||result==context_store_bootstrap_backend_outcome::already_same_durable){if(!exact_successor(response.observed_current,o.successor)){uncertain_operation_=o;reconciliation_phase_=reconciliation_phase::consumption_uncertain;quarantined_.store(true,std::memory_order_release);return context_store_bootstrap_backend_outcome::malformed_response;}return result;}
    switch(result){case context_store_bootstrap_backend_outcome::definitely_not_applied:case context_store_bootstrap_backend_outcome::stale_predecessor:case context_store_bootstrap_backend_outcome::conflict:case context_store_bootstrap_backend_outcome::attempt_replayed:return result;default:uncertain_operation_=o;reconciliation_phase_=reconciliation_phase::consumption_uncertain;quarantined_.store(true,std::memory_order_release);return result;}
}

context_store_bootstrap_reconciliation_backend_result context_store_bootstrap_consumption_backend::execute_reconciliation(const context_store_bootstrap_reconciliation_operation&o)noexcept{
    std::lock_guard<std::mutex>l(mutex_);context_store_bootstrap_reconciliation_backend_result fail;
    context_store_format_digest recomputed{};
    if(!valid_root_){fail.outcome=context_store_bootstrap_reconciliation_backend_outcome::not_reconcilable;return fail;}
    if(has_reconciliation_attempt_&&eq(reconciliation_attempt_,o.reconciliation_attempt_id)){fail.outcome=context_store_bootstrap_reconciliation_backend_outcome::attempt_replayed;return fail;}
    if(reconciliation_phase_==reconciliation_phase::visibility_uncertain_terminal){fail.outcome=context_store_bootstrap_reconciliation_backend_outcome::root_quarantined;return fail;}
    if(reconciliation_phase_!=reconciliation_phase::consumption_uncertain){fail.outcome=context_store_bootstrap_reconciliation_backend_outcome::not_reconcilable;return fail;}
    if(!eq(uncertain_operation_.root_identity,o.root_identity)||
       !eq(uncertain_operation_.attempt_id,o.original_attempt_id)||!eq(uncertain_operation_.operation_commitment,o.original_operation_commitment)||
       !exact_predecessor(uncertain_operation_.predecessor,o.predecessor)||!exact_successor(uncertain_operation_.successor,o.successor)||
       !exact_anchor(uncertain_operation_.proposed_anchor,o.proposed_anchor)||
       !nz(o.reconciliation_attempt_id)||!reconciliation_digest(o,recomputed)||!eq(recomputed,o.reconciliation_commitment)){fail.outcome=context_store_bootstrap_reconciliation_backend_outcome::not_reconcilable;return fail;}
    // The aggregate comparison above binds proof-equivalent original fields;
    // the anchor carrier is independently bound to the recorded anchor digest.
    if(!o.proposed_anchor.envelope_digest()||!eq(*o.proposed_anchor.envelope_digest(),uncertain_operation_.proposed_anchor_envelope_digest)){fail.outcome=context_store_bootstrap_reconciliation_backend_outcome::not_reconcilable;return fail;}
    if(has_reconciliation_attempt_){fail.outcome=context_store_bootstrap_reconciliation_backend_outcome::not_reconcilable;return fail;}
    reconciliation_attempt_=o.reconciliation_attempt_id;has_reconciliation_attempt_=true;
    reconciliation_phase_=reconciliation_phase::reconciliation_in_flight;
    context_store_bootstrap_reconciliation_backend_result response;
    try{response=fence_original_and_read_current(o);}catch(...){quarantined_.store(true,std::memory_order_release);reconciliation_phase_=reconciliation_phase::visibility_uncertain_terminal;return fail;}
    const bool confirmations=eq(response.root_identity,o.root_identity)&&eq(response.reconciliation_attempt_id,o.reconciliation_attempt_id)&&
        eq(response.reconciliation_commitment,o.reconciliation_commitment)&&eq(response.original_attempt_id,o.original_attempt_id)&&
        eq(response.original_operation_commitment,o.original_operation_commitment);
    const bool present=response.outcome==context_store_bootstrap_reconciliation_backend_outcome::authoritative_present;
    const bool absent=response.outcome==context_store_bootstrap_reconciliation_backend_outcome::authoritative_absent;
    if(!confirmations||response.observed_current_size>response.observed_current.size()||(present&&(response.observed_current_size==0))||(!present&&response.observed_current_size!=0)){
        quarantined_.store(true,std::memory_order_release);reconciliation_phase_=reconciliation_phase::visibility_uncertain_terminal;response.outcome=context_store_bootstrap_reconciliation_backend_outcome::malformed_response;response.observed_current_size=0;return response;
    }
    if(present){if(exact_head(response.observed_current.data(),response.observed_current_size,uncertain_operation_.successor.envelope_data(),uncertain_operation_.successor.envelope_size()))reconciliation_phase_=reconciliation_phase::successor_recovered_terminal;else if(exact_head(response.observed_current.data(),response.observed_current_size,uncertain_operation_.predecessor.envelope_data(),uncertain_operation_.predecessor.envelope_size()))reconciliation_phase_=reconciliation_phase::predecessor_confirmed_terminal_no_retry;else reconciliation_phase_=reconciliation_phase::conflict_terminal;return response;}
    (void)absent;quarantined_.store(true,std::memory_order_release);reconciliation_phase_=reconciliation_phase::visibility_uncertain_terminal;return response;
}

context_store_bootstrap_consumption_proof::context_store_bootstrap_consumption_proof(context_store_bootstrap_consumption_proof&&o)noexcept{*this=std::move(o);}
context_store_bootstrap_consumption_proof&context_store_bootstrap_consumption_proof::operator=(context_store_bootstrap_consumption_proof&&o)noexcept{if(this!=&o){valid_=o.valid_;predecessor_=o.predecessor_;successor_=o.successor_;anchor_=o.anchor_;root_=o.root_;attempt_=o.attempt_;operation_commitment_=o.operation_commitment_;command_=o.command_;token_=o.token_;plan_=o.plan_;snapshot_=o.snapshot_;manifest_=o.manifest_;sequence_=o.sequence_;classified_=o.classified_;o.valid_=false;o.predecessor_={};o.successor_={};o.anchor_={};wipe(o.root_.data(),32);wipe(o.attempt_.data(),32);wipe(o.operation_commitment_.data(),32);wipe(o.command_.data(),32);wipe(o.token_.data(),32);wipe(o.plan_.data(),32);wipe(o.snapshot_.data(),32);wipe(o.manifest_.data(),32);o.sequence_=0;o.classified_=context_store_bootstrap_backend_outcome::definitely_not_applied;}return *this;}

context_store_bootstrap_consumption_result context_store_bootstrap_consumption_coordinator::consume(const context_store_bootstrap_consumption_request&r)noexcept{
    context_store_bootstrap_consumption_result out;if(backend_.quarantined()){out.status=context_store_bootstrap_consumption_status::root_quarantined;return out;}
    context_store_bootstrap_consumption_operation op;if(!build_operation(r,backend_.root_identity(),op)){out.status=context_store_bootstrap_consumption_status::invalid_request;return out;}
    auto result=backend_.execute(op);if(result==context_store_bootstrap_backend_outcome::advanced_durable||result==context_store_bootstrap_backend_outcome::already_same_durable){out.status=result==context_store_bootstrap_backend_outcome::advanced_durable?context_store_bootstrap_consumption_status::advanced_unexecuted:context_store_bootstrap_consumption_status::already_consumed_same_unexecuted;auto&p=out.proof;p.valid_=true;p.predecessor_=op.predecessor;p.successor_=op.successor;p.anchor_=op.proposed_anchor;p.root_=op.root_identity;p.attempt_=op.attempt_id;p.operation_commitment_=op.operation_commitment;p.command_=op.command_id;p.token_=op.token_digest;p.plan_=op.plan_commitment;p.snapshot_=op.authority_snapshot_commitment;p.manifest_=op.selected_manifest_digest;p.sequence_=op.authorization_sequence;p.classified_=result;return out;}
    switch(result){case context_store_bootstrap_backend_outcome::stale_predecessor:out.status=context_store_bootstrap_consumption_status::stale_predecessor;break;case context_store_bootstrap_backend_outcome::conflict:out.status=context_store_bootstrap_consumption_status::conflict;break;case context_store_bootstrap_backend_outcome::attempt_replayed:out.status=context_store_bootstrap_consumption_status::attempt_replayed;break;case context_store_bootstrap_backend_outcome::definitely_not_applied:out.status=context_store_bootstrap_consumption_status::definitely_not_applied;break;default:out.status=context_store_bootstrap_consumption_status::visibility_uncertain;break;}return out;
}

context_store_bootstrap_recovered_consumption_proof::context_store_bootstrap_recovered_consumption_proof(context_store_bootstrap_recovered_consumption_proof&&o)noexcept{*this=std::move(o);}
context_store_bootstrap_recovered_consumption_proof&context_store_bootstrap_recovered_consumption_proof::operator=(context_store_bootstrap_recovered_consumption_proof&&o)noexcept{
    if(this!=&o){valid_=o.valid_;predecessor_=o.predecessor_;successor_=o.successor_;anchor_=o.anchor_;root_=o.root_;original_attempt_=o.original_attempt_;original_operation_=o.original_operation_;reconciliation_attempt_=o.reconciliation_attempt_;reconciliation_commitment_=o.reconciliation_commitment_;command_=o.command_;token_=o.token_;plan_=o.plan_;snapshot_=o.snapshot_;manifest_=o.manifest_;sequence_=o.sequence_;observed_=o.observed_;observed_size_=o.observed_size_;classified_=o.classified_;
        o.valid_=false;o.predecessor_={};o.successor_={};o.anchor_={};wipe(o.root_.data(),32);wipe(o.original_attempt_.data(),32);wipe(o.original_operation_.data(),32);wipe(o.reconciliation_attempt_.data(),32);wipe(o.reconciliation_commitment_.data(),32);wipe(o.command_.data(),32);wipe(o.token_.data(),32);wipe(o.plan_.data(),32);wipe(o.snapshot_.data(),32);wipe(o.manifest_.data(),32);wipe(o.observed_.data(),o.observed_.size());o.sequence_=0;o.observed_size_=0;o.classified_=context_store_bootstrap_reconciliation_status::visibility_uncertain;}
    return *this;
}

context_store_bootstrap_reconciliation_result context_store_bootstrap_reconciliation_coordinator::reconcile(const context_store_bootstrap_reconciliation_request&r)noexcept{
    context_store_bootstrap_reconciliation_result out;
    if(!nz(r.original_attempt_id)||!nz(r.reconciliation_attempt_id)){out.status=context_store_bootstrap_reconciliation_status::invalid_request;return out;}
    context_store_bootstrap_consumption_request original;original.attempt_id=r.original_attempt_id;original.plan=r.plan;original.predecessor=r.predecessor;original.registry_authentication_key=r.registry_authentication_key;
    context_store_bootstrap_consumption_operation old;if(!build_operation(original,backend_.root_identity(),old)){out.status=context_store_bootstrap_reconciliation_status::invalid_request;return out;}
    context_store_bootstrap_reconciliation_operation op;op.root_identity=old.root_identity;op.reconciliation_attempt_id=r.reconciliation_attempt_id;op.original_attempt_id=old.attempt_id;op.original_operation_commitment=old.operation_commitment;op.predecessor=old.predecessor;op.successor=old.successor;op.proposed_anchor=*r.plan.anchor();
    if(!reconciliation_digest(op,op.reconciliation_commitment)){out.status=context_store_bootstrap_reconciliation_status::invalid_request;return out;}
    const auto response=backend_.execute_reconciliation(op);
    switch(response.outcome){
        case context_store_bootstrap_reconciliation_backend_outcome::attempt_replayed:out.status=context_store_bootstrap_reconciliation_status::attempt_replayed;return out;
        case context_store_bootstrap_reconciliation_backend_outcome::not_reconcilable:out.status=context_store_bootstrap_reconciliation_status::not_reconcilable;return out;
        case context_store_bootstrap_reconciliation_backend_outcome::root_quarantined:out.status=context_store_bootstrap_reconciliation_status::root_quarantined;return out;
        case context_store_bootstrap_reconciliation_backend_outcome::authoritative_present:break;
        default:out.status=context_store_bootstrap_reconciliation_status::visibility_uncertain;return out;
    }
    const uint8_t*w=response.observed_current.data();const size_t ws=response.observed_current_size;
    if(exact_head(w,ws,op.successor.envelope_data(),op.successor.envelope_size())){
        out.status=context_store_bootstrap_reconciliation_status::consumed_same_recovered_unexecuted;auto&p=out.proof;p.valid_=true;p.predecessor_=op.predecessor;p.successor_=op.successor;p.anchor_=op.proposed_anchor;p.root_=op.root_identity;p.original_attempt_=op.original_attempt_id;p.original_operation_=op.original_operation_commitment;p.reconciliation_attempt_=op.reconciliation_attempt_id;p.reconciliation_commitment_=op.reconciliation_commitment;p.command_=old.command_id;p.token_=old.token_digest;p.plan_=old.plan_commitment;p.snapshot_=old.authority_snapshot_commitment;p.manifest_=old.selected_manifest_digest;p.sequence_=old.authorization_sequence;p.observed_=response.observed_current;p.observed_size_=ws;p.classified_=out.status;return out;
    }
    if(exact_head(w,ws,op.predecessor.envelope_data(),op.predecessor.envelope_size())){out.status=context_store_bootstrap_reconciliation_status::definitely_unconsumed_fenced_no_retry;return out;}
    out.status=context_store_bootstrap_reconciliation_status::conflict;return out;
}
const char*context_store_bootstrap_consumption_status_name(context_store_bootstrap_consumption_status s)noexcept{switch(s){case context_store_bootstrap_consumption_status::advanced_unexecuted:return"advanced-unexecuted";case context_store_bootstrap_consumption_status::already_consumed_same_unexecuted:return"already-consumed-same-unexecuted";case context_store_bootstrap_consumption_status::invalid_request:return"invalid-request";case context_store_bootstrap_consumption_status::stale_predecessor:return"stale-predecessor";case context_store_bootstrap_consumption_status::conflict:return"conflict";case context_store_bootstrap_consumption_status::attempt_replayed:return"attempt-replayed";case context_store_bootstrap_consumption_status::definitely_not_applied:return"definitely-not-applied";case context_store_bootstrap_consumption_status::root_quarantined:return"root-quarantined";case context_store_bootstrap_consumption_status::visibility_uncertain:return"visibility-uncertain";}return"unknown";}
const char*context_store_bootstrap_reconciliation_status_name(context_store_bootstrap_reconciliation_status s)noexcept{switch(s){case context_store_bootstrap_reconciliation_status::consumed_same_recovered_unexecuted:return"consumed-same-recovered-unexecuted";case context_store_bootstrap_reconciliation_status::definitely_unconsumed_fenced_no_retry:return"definitely-unconsumed-fenced-no-retry";case context_store_bootstrap_reconciliation_status::conflict:return"conflict";case context_store_bootstrap_reconciliation_status::invalid_request:return"invalid-request";case context_store_bootstrap_reconciliation_status::attempt_replayed:return"attempt-replayed";case context_store_bootstrap_reconciliation_status::not_reconcilable:return"not-reconcilable";case context_store_bootstrap_reconciliation_status::root_quarantined:return"root-quarantined";case context_store_bootstrap_reconciliation_status::visibility_uncertain:return"visibility-uncertain";}return"unknown";}
} // namespace halofpx
