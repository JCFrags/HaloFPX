#include "halofpx-context-store-bootstrap-material.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>
#include <string>
#include <type_traits>
#include <utility>

namespace halofpx {
void (*context_store_bootstrap_material_synthetic_coordinator::post_positive_test_hook_)() = nullptr;
namespace {

using bytes = std::vector<uint8_t>;
constexpr uint64_t manifest_limit = context_store_manifest_max_bytes;
constexpr uint64_t absolute_frame_limit = 16777216;
constexpr uint64_t absolute_aggregate_limit = 67108864;
constexpr std::array<uint8_t, 8> object_magic = { 0x48,0x41,0x4c,0x4f,0x4f,0x42,0x4a,0x01 };
constexpr char object_domain[] = "halofpx.object.v1";

bool zero(const context_store_format_digest & d) noexcept {
    uint8_t v = 0; for (auto b : d) v |= b; return v == 0;
}
bool equal(const context_store_format_digest & a, const context_store_format_digest & b) noexcept {
    uint8_t v = 0; for (size_t i=0;i<a.size();++i) v |= static_cast<uint8_t>(a[i]^b[i]); return v == 0;
}
bool equal_bytes(const uint8_t * a, const uint8_t * b, size_t n) noexcept {
    if ((!a || !b) && n) return false; uint8_t v=0; for(size_t i=0;i<n;++i)v|=static_cast<uint8_t>(a[i]^b[i]); return v==0;
}
void put(bytes & out, const void * p, size_t n) { const auto * b=static_cast<const uint8_t *>(p); out.insert(out.end(),b,b+n); }
void u64(bytes & out, uint64_t value) { for(int s=56;s>=0;s-=8) out.push_back(static_cast<uint8_t>(value>>s)); }
void u8(bytes & out, uint8_t value) { out.push_back(value); }
void domain(bytes & out, const char * value) { put(out,value,std::strlen(value)+1); }
bool digest(const bytes & in, context_store_format_digest & out) noexcept {
    return context_store_sha256_bounded(in.data(),in.size(),absolute_aggregate_limit+manifest_limit+65536,out);
}
bool registered(const context_store_registered_id & id) noexcept {
    if(!id.size||id.size>context_store_registered_id_max_bytes)return false;
    for(size_t i=0;i<id.size;++i)if(id.bytes[i]<=0||static_cast<unsigned char>(id.bytes[i])>=0x80)return false;
    return true;
}

struct source_view {
    const context_store_format_digest * root=nullptr,* attempt=nullptr,* operation=nullptr,* command=nullptr,* token=nullptr,* plan=nullptr,* snapshot=nullptr,* manifest=nullptr;
    const context_store_authenticated_protected_registry_successor * successor=nullptr;
    const context_store_authenticated_anchor * anchor=nullptr;
    const context_store_bootstrap_reconciliation_id * reconciliation_attempt=nullptr;
    const context_store_format_digest * reconciliation_commitment=nullptr;
    const uint8_t * observed=nullptr;
    size_t observed_size=0;
    uint64_t sequence=0;
    const char * tag=nullptr;
    size_t tag_size=0;
    bool recovered=false;
};

source_view view(const context_store_bootstrap_consumption_proof & p) noexcept {
    source_view v;
    if(!p.valid()) return v;
    v.root=p.root_identity();v.attempt=p.attempt_id();v.operation=p.operation_commitment();v.command=p.command_id();v.token=p.authorization_token_digest();v.plan=p.plan_commitment();v.snapshot=p.authority_snapshot_commitment();v.manifest=p.selected_manifest_digest();v.successor=p.successor();v.anchor=p.proposed_anchor();v.sequence=p.authorization_sequence();
    if(p.classified_outcome()==context_store_bootstrap_backend_outcome::advanced_durable){v.tag="direct-advanced-v1";v.tag_size=18;}
    else if(p.classified_outcome()==context_store_bootstrap_backend_outcome::already_same_durable){v.tag="direct-already-same-v1";v.tag_size=22;}
    return v;
}
source_view view(const context_store_bootstrap_recovered_consumption_proof & p) noexcept {
    source_view v;
    if(!p.valid()||!p.original_consumption_uncertain_confirmed()||p.classified_outcome()!=context_store_bootstrap_reconciliation_status::consumed_same_recovered_unexecuted)return v;
    v.root=p.root_identity();v.attempt=p.original_attempt_id();v.operation=p.original_operation_commitment();v.command=p.command_id();v.token=p.authorization_token_digest();v.plan=p.plan_commitment();v.snapshot=p.authority_snapshot_commitment();v.manifest=p.selected_manifest_digest();v.successor=p.successor();v.anchor=p.proposed_anchor();v.sequence=p.authorization_sequence();v.reconciliation_attempt=p.reconciliation_attempt_id();v.reconciliation_commitment=p.reconciliation_commitment();v.observed=p.observed_successor_data();v.observed_size=p.observed_successor_size();v.tag="reconciled-successor-v1";v.tag_size=23;v.recovered=true;return v;
}
source_view view(const context_store_bootstrap_material_synthetic_operation & op) noexcept {
    source_view v;v.root=&op.registry_root_identity;v.attempt=&op.original_consumption_attempt;v.operation=&op.original_consumption_operation;v.command=&op.command_id;v.token=&op.authorization_token_digest;v.plan=&op.plan_commitment;v.snapshot=&op.authority_snapshot_commitment;v.manifest=&op.selected_manifest_digest;v.successor=&op.authenticated_successor;v.anchor=&op.authenticated_proposed_anchor;v.sequence=op.authorization_sequence;v.tag=op.provenance_tag.data();v.tag_size=op.provenance_tag_size;v.recovered=op.reconciled_source;if(v.recovered){v.reconciliation_attempt=&op.reconciliation_attempt;v.reconciliation_commitment=&op.reconciliation_commitment;v.observed=op.observed_successor.data();v.observed_size=op.observed_successor.size();}return v;
}
bool complete(const source_view & v) noexcept {
    return v.root&&v.attempt&&v.operation&&v.command&&v.token&&v.plan&&v.snapshot&&v.manifest&&v.successor&&v.successor->authenticated()&&v.anchor&&v.anchor->authenticated()&&v.tag&&(!v.recovered||(v.reconciliation_attempt&&v.reconciliation_commitment&&v.observed&&v.observed_size));
}

bool source_set_commitment(const std::vector<std::vector<uint8_t>> & frames, context_store_format_digest & out) {
    bytes b;domain(b,"halofpx.bootstrap-material-source-set.v1");u64(b,frames.size());
    for(size_t i=0;i<frames.size();++i){u64(b,i);u64(b,frames[i].size());put(b,frames[i].data(),frames[i].size());}
    return digest(b,out);
}
bool policy_commitment(const context_store_bootstrap_material_synthetic_policy & p,const context_store_format_digest & sources,context_store_format_digest & out) {
    bytes b;domain(b,"halofpx.bootstrap-material-root-policy.v1");put(b,p.material_root_identity.data(),32);put(b,p.registry_root_identity.data(),32);put(b,p.store_uuid.data(),16);put(b,p.namespace_id.data(),32);put(b,p.checkpoint_lineage_id.data(),32);u64(b,p.policy_epoch);u64(b,p.manifest_key_generation);u64(b,p.writer_authority_epoch);u64(b,p.durability_policy_id.size);put(b,p.durability_policy_id.bytes.data(),p.durability_policy_id.size);u8(b,p.manifest_durability_mode);u64(b,p.maximum_source_object_count);u64(b,p.maximum_frame_bytes);u64(b,p.maximum_aggregate_frame_bytes);put(b,sources.data(),32);return digest(b,out);
}
bool authority_commitment(const source_view & v,context_store_format_digest & out) {
    if(!complete(v))return false;bytes b;domain(b,"halofpx.bootstrap-material-authority-source.v1");u64(b,v.tag_size);put(b,v.tag,v.tag_size);put(b,v.root->data(),32);put(b,v.attempt->data(),32);put(b,v.operation->data(),32);u64(b,v.successor->envelope_size());put(b,v.successor->envelope_data(),v.successor->envelope_size());u64(b,v.anchor->envelope_size());put(b,v.anchor->envelope_data(),v.anchor->envelope_size());u64(b,v.sequence);put(b,v.command->data(),32);put(b,v.token->data(),32);put(b,v.plan->data(),32);put(b,v.snapshot->data(),32);put(b,v.manifest->data(),32);if(v.recovered){put(b,v.reconciliation_attempt->data(),32);put(b,v.reconciliation_commitment->data(),32);u64(b,v.observed_size);put(b,v.observed,v.observed_size);}return digest(b,out);
}
bool material_commitment(const bytes & envelope,const context_store_parsed_manifest & m,context_store_format_digest & out) {
    bytes b;domain(b,"halofpx.bootstrap-material-set.v1");u64(b,envelope.size());put(b,envelope.data(),envelope.size());u64(b,m.object_count);for(size_t i=0;i<m.object_count;++i){const auto & r=m.object_references[i];u64(b,i);put(b,r.object_id.data(),32);u64(b,r.stream_type.size);put(b,r.stream_type.bytes.data(),r.stream_type.size);u64(b,r.frame_bytes);}return digest(b,out);
}
bool operation_commitment(const context_store_bootstrap_material_synthetic_operation & o,context_store_format_digest & out) {
    bytes b;domain(b,"halofpx.bootstrap-material-preparation.v1");put(b,o.material_root_identity.data(),32);put(b,o.registry_root_identity.data(),32);put(b,o.material_attempt_id.data(),32);put(b,o.root_policy_commitment.data(),32);put(b,o.authority_source_commitment.data(),32);put(b,o.source_set_commitment.data(),32);put(b,o.material_set_commitment.data(),32);put(b,o.selected_manifest_digest.data(),32);put(b,o.proposed_anchor_envelope_digest.data(),32);return digest(b,out);
}
bool close_commitment(const context_store_bootstrap_material_synthetic_operation & o,context_store_format_digest & out) {
    bytes b;domain(b,"halofpx.bootstrap-material-durable-close.v1");put(b,o.material_root_identity.data(),32);put(b,o.registry_root_identity.data(),32);put(b,o.material_attempt_id.data(),32);put(b,o.operation_commitment.data(),32);put(b,o.source_set_commitment.data(),32);put(b,o.material_set_commitment.data(),32);put(b,o.selected_manifest_digest.data(),32);u8(b,1);return digest(b,out);
}
bool frame_matches(const std::vector<uint8_t> & f,const context_store_object_reference & r,uint64_t max) noexcept {
    if(f.empty()||f.size()>max||r.frame_bytes!=f.size())return false;size_t pos=0;
    auto take=[&](size_t n,const uint8_t *& p){if(pos>f.size()||n>f.size()-pos)return false;p=f.data()+pos;pos+=n;return true;};
    const uint8_t * p=nullptr;if(!take(object_magic.size(),p)||!equal_bytes(p,object_magic.data(),object_magic.size()))return false;
    if(!take(2,p))return false;uint16_t dl=static_cast<uint16_t>((p[0]<<8)|p[1]);if(dl!=sizeof(object_domain)-1||!take(dl,p)||!equal_bytes(p,reinterpret_cast<const uint8_t*>(object_domain),dl))return false;
    if(!take(2,p))return false;uint16_t tl=static_cast<uint16_t>((p[0]<<8)|p[1]);if(!tl||tl>context_store_registered_id_max_bytes||tl!=r.stream_type.size||!take(tl,p)||!equal_bytes(p,reinterpret_cast<const uint8_t*>(r.stream_type.bytes.data()),tl))return false;for(size_t i=0;i<tl;++i)if(p[i]==0||p[i]>=0x80)return false;
    if(!take(8,p))return false;uint64_t payload=0;for(size_t i=0;i<8;++i)payload=(payload<<8)|p[i];if(payload!=f.size()-pos)return false;
    context_store_format_digest d{};return context_store_sha256_bounded(f.data(),f.size(),max,d)&&equal(d,r.object_id);
}
bool frames_match(const std::vector<std::vector<uint8_t>> & frames,const context_store_parsed_manifest & m,const context_store_bootstrap_material_synthetic_policy & p) noexcept {
    if(frames.size()!=m.object_count||frames.size()>p.maximum_source_object_count)return false;uint64_t total=0;for(size_t i=0;i<frames.size();++i){if(frames[i].size()>p.maximum_frame_bytes||frames[i].size()>p.maximum_aggregate_frame_bytes||total>p.maximum_aggregate_frame_bytes-frames[i].size()||!frame_matches(frames[i],m.object_references[i],p.maximum_frame_bytes))return false;total+=frames[i].size();}return true;
}
bool policy_valid(const context_store_bootstrap_material_synthetic_policy & p) noexcept {
    return !zero(p.material_root_identity)&&!zero(p.registry_root_identity)&&!equal(p.material_root_identity,p.registry_root_identity)&&registered(p.durability_policy_id)&&p.maximum_source_object_count>=1&&p.maximum_source_object_count<=128&&p.maximum_frame_bytes>=1&&p.maximum_frame_bytes<=absolute_frame_limit&&p.maximum_aggregate_frame_bytes>=1&&p.maximum_aggregate_frame_bytes<=absolute_aggregate_limit;
}
bool manifest_matches(const context_store_parsed_manifest & m,const context_store_bootstrap_material_synthetic_policy & p,const context_store_authenticated_anchor & a) noexcept {
    const auto * ab=a.body();if(!ab)return false;
    return m.store_uuid==p.store_uuid&&equal(m.scope_namespace,p.namespace_id)&&equal(m.checkpoint_lineage_id,p.checkpoint_lineage_id)&&m.policy_epoch==p.policy_epoch&&m.authentication_key_generation==p.manifest_key_generation&&m.generation==1&&!m.has_predecessor&&m.durability_mode==p.manifest_durability_mode&&ab->store_uuid==p.store_uuid&&equal(ab->namespace_id,p.namespace_id)&&equal(ab->checkpoint_lineage_id,p.checkpoint_lineage_id)&&ab->policy_epoch==p.policy_epoch&&ab->manifest_key_generation==p.manifest_key_generation&&ab->authority_epoch==p.writer_authority_epoch&&ab->generation==1&&!ab->has_predecessor;
}
context_store_bootstrap_material_synthetic_status map_definite(context_store_bootstrap_material_synthetic_backend_outcome o) noexcept {
    using O=context_store_bootstrap_material_synthetic_backend_outcome;using S=context_store_bootstrap_material_synthetic_status;
    switch(o){case O::source_conflict:return S::source_conflict;case O::policy_conflict:return S::policy_conflict;case O::writer_busy:return S::writer_busy;case O::object_collision:return S::object_collision;case O::manifest_collision:return S::manifest_collision;case O::no_space:return S::no_space;case O::quota_exhausted:return S::quota_exhausted;case O::reserve_exhausted:return S::reserve_exhausted;case O::read_only:return S::read_only;case O::storage_error:return S::storage_error;case O::synchronization_error:return S::synchronization_error;case O::definitely_aborted:return S::definitely_aborted;default:return S::visibility_uncertain;}
}
bool positive(context_store_bootstrap_material_synthetic_backend_outcome o) noexcept {return o==context_store_bootstrap_material_synthetic_backend_outcome::prepared_backend_claim||o==context_store_bootstrap_material_synthetic_backend_outcome::already_same_backend_claim;}

} // namespace

template<class Proof>
context_store_bootstrap_material_synthetic_result context_store_bootstrap_material_synthetic_coordinator::prepare_owned(Proof && consumed,const context_store_bootstrap_material_synthetic_request & request,bool recovered) noexcept {
    context_store_bootstrap_material_synthetic_result result;
    bool backend_entered = false;
    try {
        const source_view s=view(consumed);
        if(!complete(s)){result.status=context_store_bootstrap_material_synthetic_status::invalid_source;return result;}
        if(!backend_.valid()||zero(request.material_attempt_id)||!request.manifest_envelope_data||!request.manifest_envelope_size||request.manifest_envelope_size>context_store_manifest_max_bytes){result.status=context_store_bootstrap_material_synthetic_status::invalid_request;return result;}
        if(!equal(*s.root,backend_.policy().registry_root_identity)){result.status=context_store_bootstrap_material_synthetic_status::source_conflict;return result;}
        bytes envelope(request.manifest_envelope_data,request.manifest_envelope_data+request.manifest_envelope_size);
        context_store_format_digest selected{};if(!context_store_manifest_digest_v1(envelope.data(),envelope.size(),selected)||!equal(selected,*s.manifest)||!s.anchor->body()||!equal(selected,s.anchor->body()->selected_manifest_digest)){result.status=context_store_bootstrap_material_synthetic_status::invalid_request;return result;}
        auto parsed=context_store_parse_manifest_v1(envelope.data(),envelope.size());if(parsed.status!=context_store_manifest_parse_status::structural_only||!manifest_matches(parsed.manifest,backend_.policy(),*s.anchor)){result.status=context_store_bootstrap_material_synthetic_status::invalid_request;return result;}
        if(!frames_match(backend_.source_frames_,parsed.manifest,backend_.policy())){result.status=context_store_bootstrap_material_synthetic_status::invalid_request;return result;}
        context_store_bootstrap_material_synthetic_operation op;op.material_attempt_id=request.material_attempt_id;op.material_root_identity=backend_.policy().material_root_identity;op.registry_root_identity=backend_.policy().registry_root_identity;op.manifest_envelope=std::move(envelope);op.selected_manifest_digest=selected;op.proposed_anchor_envelope_digest=*s.anchor->envelope_digest();op.provenance_tag_size=s.tag_size;std::copy(s.tag,s.tag+s.tag_size,op.provenance_tag.begin());op.original_consumption_attempt=*s.attempt;op.original_consumption_operation=*s.operation;op.command_id=*s.command;op.authorization_token_digest=*s.token;op.plan_commitment=*s.plan;op.authority_snapshot_commitment=*s.snapshot;op.authorization_sequence=s.sequence;op.authenticated_successor=*s.successor;op.authenticated_proposed_anchor=*s.anchor;op.reconciled_source=s.recovered;if(s.recovered){op.reconciliation_attempt=*s.reconciliation_attempt;op.reconciliation_commitment=*s.reconciliation_commitment;op.observed_successor.assign(s.observed,s.observed+s.observed_size);}
        if(!source_set_commitment(backend_.source_frames_,op.source_set_commitment)||!policy_commitment(backend_.policy(),op.source_set_commitment,op.root_policy_commitment)||!authority_commitment(s,op.authority_source_commitment)||!material_commitment(op.manifest_envelope,parsed.manifest,op.material_set_commitment)||!operation_commitment(op,op.operation_commitment)){result.status=context_store_bootstrap_material_synthetic_status::resource_exhausted;return result;}
        result.proof.descriptors_.assign(parsed.manifest.object_references.begin(),parsed.manifest.object_references.begin()+parsed.manifest.object_count);context_store_bootstrap_material_synthetic_witness witness;backend_entered=true;result.status=backend_.execute(op,parsed.manifest,witness);if(result.status!=context_store_bootstrap_material_synthetic_status::prepared_backend_claim&&result.status!=context_store_bootstrap_material_synthetic_status::already_same_backend_claim)return result;if(post_positive_test_hook_)post_positive_test_hook_();
        context_store_format_digest observed_sources{},observed_close{};if(!frames_match(witness.observed_frames,parsed.manifest,backend_.policy())||!source_set_commitment(witness.observed_frames,observed_sources)||!equal(observed_sources,op.source_set_commitment)||witness.observed_manifest_envelope.size()!=op.manifest_envelope.size()||!equal_bytes(witness.observed_manifest_envelope.data(),op.manifest_envelope.data(),op.manifest_envelope.size())||!close_commitment(op,observed_close)||!equal(observed_close,witness.durable_close_confirmation)){backend_.quarantined_.store(true,std::memory_order_release);result.status=context_store_bootstrap_material_synthetic_status::visibility_uncertain;return result;}
        result.proof.valid_=true;result.proof.anchor_=op.authenticated_proposed_anchor;if constexpr(std::is_same_v<std::remove_reference_t<Proof>,context_store_bootstrap_recovered_consumption_proof>)result.proof.recovered_source_=std::move(consumed);else result.proof.direct_source_=std::move(consumed);result.proof.attempt_=op.material_attempt_id;result.proof.material_root_=op.material_root_identity;result.proof.registry_root_=op.registry_root_identity;result.proof.policy_=op.root_policy_commitment;result.proof.fixed_policy_=backend_.policy();result.proof.authority_=op.authority_source_commitment;result.proof.source_set_=op.source_set_commitment;result.proof.material_set_=op.material_set_commitment;result.proof.operation_=op.operation_commitment;result.proof.manifest_=std::move(op.manifest_envelope);result.proof.observed_frames_=std::move(witness.observed_frames);result.proof.close_=witness.durable_close_confirmation;result.proof.outcome_=witness.outcome;return result;
    } catch(...) {if(backend_entered){backend_.quarantined_.store(true,std::memory_order_release);result.status=context_store_bootstrap_material_synthetic_status::visibility_uncertain;}else result.status=context_store_bootstrap_material_synthetic_status::resource_exhausted;return result;}
}

context_store_bootstrap_material_synthetic_proof::context_store_bootstrap_material_synthetic_proof(context_store_bootstrap_material_synthetic_proof && o) noexcept {*this=std::move(o);}
context_store_bootstrap_material_synthetic_proof & context_store_bootstrap_material_synthetic_proof::operator=(context_store_bootstrap_material_synthetic_proof && o) noexcept {if(this!=&o){valid_=o.valid_;direct_source_=std::move(o.direct_source_);recovered_source_=std::move(o.recovered_source_);attempt_=o.attempt_;material_root_=o.material_root_;registry_root_=o.registry_root_;policy_=o.policy_;fixed_policy_=o.fixed_policy_;authority_=o.authority_;source_set_=o.source_set_;material_set_=o.material_set_;operation_=o.operation_;anchor_=o.anchor_;manifest_=std::move(o.manifest_);observed_frames_=std::move(o.observed_frames_);descriptors_=std::move(o.descriptors_);close_=o.close_;outcome_=o.outcome_;o.valid_=false;o.attempt_={};o.material_root_={};o.registry_root_={};o.policy_={};o.fixed_policy_={};o.authority_={};o.source_set_={};o.material_set_={};o.operation_={};o.anchor_={};o.close_={};o.outcome_=context_store_bootstrap_material_synthetic_backend_outcome::uncertain;}return *this;}

context_store_bootstrap_material_synthetic_backend::context_store_bootstrap_material_synthetic_backend(const context_store_bootstrap_material_synthetic_policy & p,std::vector<std::vector<uint8_t>> && frames) noexcept : policy_(p),source_frames_(std::move(frames)) {try{if(!policy_valid(policy_)||source_frames_.empty()||source_frames_.size()>policy_.maximum_source_object_count)return;uint64_t total=0;for(const auto & f:source_frames_){if(f.empty()||f.size()>policy_.maximum_frame_bytes||f.size()>policy_.maximum_aggregate_frame_bytes||total>policy_.maximum_aggregate_frame_bytes-f.size())return;total+=f.size();}valid_=true;}catch(...){valid_=false;}}
context_store_bootstrap_material_synthetic_backend::~context_store_bootstrap_material_synthetic_backend()=default;

context_store_bootstrap_material_synthetic_status context_store_bootstrap_material_synthetic_backend::execute(const context_store_bootstrap_material_synthetic_operation & op,const context_store_parsed_manifest & manifest,context_store_bootstrap_material_synthetic_witness & witness) noexcept {
    using S=context_store_bootstrap_material_synthetic_status;std::lock_guard<std::mutex> lock(mutex_);if(quarantined())return S::material_root_quarantined;
    for(size_t i=0;i<terminal_attempt_count_;++i)if(terminal_attempts_[i]==op.material_attempt_id)return S::attempt_replayed;
    if(terminal_attempt_count_==maximum_terminal_attempts_)return S::history_exhausted;
    terminal_attempts_[terminal_attempt_count_++]=op.material_attempt_id;if(!has_stable_binding_){stable_authority_source_=op.authority_source_commitment;stable_root_policy_=op.root_policy_commitment;stable_material_set_=op.material_set_commitment;has_stable_binding_=true;}
    try {
        context_store_format_digest ss{},pc{},ac{},mc{},oc{};const auto exact_source=view(op);if(!source_set_commitment(source_frames_,ss)||!policy_commitment(policy_,ss,pc)||!authority_commitment(exact_source,ac)||!material_commitment(op.manifest_envelope,manifest,mc)||!operation_commitment(op,oc)||!equal(ss,op.source_set_commitment)||!equal(pc,op.root_policy_commitment)||!equal(ac,op.authority_source_commitment)||!equal(mc,op.material_set_commitment)||!equal(oc,op.operation_commitment)||!frames_match(source_frames_,manifest,policy_)){quarantined_.store(true,std::memory_order_release);return S::visibility_uncertain;}
        if(terminal_attempt_count_>1&&(!equal(stable_authority_source_,op.authority_source_commitment)||!equal(stable_root_policy_,op.root_policy_commitment)||!equal(stable_material_set_,op.material_set_commitment)))return S::source_conflict;
        witness=prepare_exact_material_set_and_durable_close(op);
    if(!positive(witness.outcome)){
        const auto definite=map_definite(witness.outcome);if(definite==S::visibility_uncertain)quarantined_.store(true,std::memory_order_release);return definite;
    }
    context_store_format_digest readback{},close{};
    const bool echoes=witness.material_attempt_id==op.material_attempt_id&&equal(witness.material_root_identity,op.material_root_identity)&&equal(witness.registry_root_identity,op.registry_root_identity)&&equal(witness.root_policy_commitment,op.root_policy_commitment)&&equal(witness.authority_source_commitment,op.authority_source_commitment)&&equal(witness.source_set_commitment,op.source_set_commitment)&&equal(witness.material_set_commitment,op.material_set_commitment)&&equal(witness.operation_commitment,op.operation_commitment);
    const bool exact_manifest=witness.observed_manifest_envelope.size()==op.manifest_envelope.size()&&equal_bytes(witness.observed_manifest_envelope.data(),op.manifest_envelope.data(),op.manifest_envelope.size());
    const bool ok=echoes&&frames_match(witness.observed_frames,manifest,policy_)&&source_set_commitment(witness.observed_frames,readback)&&equal(readback,op.source_set_commitment)&&exact_manifest&&close_commitment(op,close)&&equal(close,witness.durable_close_confirmation);
    if(!ok){quarantined_.store(true,std::memory_order_release);return S::visibility_uncertain;}
    } catch(...){quarantined_.store(true,std::memory_order_release);return S::visibility_uncertain;}
    return witness.outcome==context_store_bootstrap_material_synthetic_backend_outcome::prepared_backend_claim?S::prepared_backend_claim:S::already_same_backend_claim;
}

context_store_bootstrap_material_synthetic_result context_store_bootstrap_material_synthetic_coordinator::prepare(context_store_bootstrap_consumption_proof && source,const context_store_bootstrap_material_synthetic_request & request) noexcept {context_store_bootstrap_consumption_proof consumed=std::move(source);return prepare_direct(std::move(consumed),request);}
context_store_bootstrap_material_synthetic_result context_store_bootstrap_material_synthetic_coordinator::prepare(context_store_bootstrap_recovered_consumption_proof && source,const context_store_bootstrap_material_synthetic_request & request) noexcept {context_store_bootstrap_recovered_consumption_proof consumed=std::move(source);return prepare_recovered(std::move(consumed),request);}
context_store_bootstrap_material_synthetic_result context_store_bootstrap_material_synthetic_coordinator::prepare_direct(context_store_bootstrap_consumption_proof && source,const context_store_bootstrap_material_synthetic_request & request) noexcept {return prepare_owned(std::move(source),request,false);}
context_store_bootstrap_material_synthetic_result context_store_bootstrap_material_synthetic_coordinator::prepare_recovered(context_store_bootstrap_recovered_consumption_proof && source,const context_store_bootstrap_material_synthetic_request & request) noexcept {return prepare_owned(std::move(source),request,true);}

const char * context_store_bootstrap_material_synthetic_status_name(context_store_bootstrap_material_synthetic_status s) noexcept {using S=context_store_bootstrap_material_synthetic_status;switch(s){case S::prepared_backend_claim:return "prepared-backend-claim";case S::already_same_backend_claim:return "already-same-backend-claim";case S::invalid_source:return "invalid-source";case S::invalid_request:return "invalid-request";case S::source_conflict:return "source-conflict";case S::policy_conflict:return "policy-conflict";case S::writer_busy:return "writer-busy";case S::object_collision:return "object-collision";case S::manifest_collision:return "manifest-collision";case S::no_space:return "no-space";case S::quota_exhausted:return "quota-exhausted";case S::reserve_exhausted:return "reserve-exhausted";case S::read_only:return "read-only";case S::storage_error:return "storage-error";case S::synchronization_error:return "synchronization-error";case S::definitely_aborted:return "definitely-aborted";case S::attempt_replayed:return "attempt-replayed";case S::history_exhausted:return "history-exhausted";case S::material_root_quarantined:return "material-root-quarantined";case S::resource_exhausted:return "resource-exhausted";case S::visibility_uncertain:return "visibility-uncertain";}return "unknown";}
} // namespace halofpx
