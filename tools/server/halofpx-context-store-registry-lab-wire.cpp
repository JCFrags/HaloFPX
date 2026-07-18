#include "halofpx-context-store-registry-lab-wire.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cstring>
#include <utility>

namespace halofpx { namespace {

constexpr char credential_magic[16] = "HaloFPXRegKey01";
constexpr char key_domain[] = "halofpx.registry-lab-key.v1";
// Internal admission binding only. This domain never authenticates a wire object.
constexpr char witness_credential_domain[] = "halofpx.registry-lab-witness-credential.v1";
constexpr char registry_domain[] = "halofpx.registry-lab-registry-envelope.v1";
constexpr char operation_domain[] = "halofpx.registry-lab-operation.v1";
constexpr char path_domain[] = "halofpx.registry-lab-path-policy.v1";
constexpr const char * auth_domains[] = {
    "halofpx.registry-lab-root-auth.v1", "halofpx.registry-lab-head-auth.v1",
    "halofpx.registry-lab-prepare-auth.v1", "halofpx.registry-lab-close-auth.v1",
    "halofpx.registry-lab-abort-auth.v1", "halofpx.registry-lab-quarantine-auth.v1",
};
constexpr const char * content_domains[] = {
    "halofpx.registry-lab-root.v1", "halofpx.registry-lab-head.v1",
    "halofpx.registry-lab-prepare.v1", "halofpx.registry-lab-close.v1",
    "halofpx.registry-lab-abort.v1", "halofpx.registry-lab-quarantine.v1",
};

void wipe(void * p, size_t n) noexcept { volatile uint8_t * b = static_cast<volatile uint8_t *>(p); while (n--) *b++ = 0; }
bool equal(const uint8_t * a, const uint8_t * b, size_t n) noexcept { uint8_t x=0; for(size_t i=0;i<n;++i)x|=a[i]^b[i]; return x==0; }
template<size_t N> bool nonzero(const std::array<uint8_t,N>&v) noexcept { uint8_t x=0;for(auto b:v)x|=b;return x!=0; }
bool valid_id(const context_store_registered_id & id) noexcept { if(!id.size||id.size>128)return false;for(size_t i=0;i<id.size;++i)if(id.bytes[i]<0x21||id.bytes[i]>0x7e)return false;return true; }
bool same_id(const context_store_registered_id&a,const context_store_registered_id&b)noexcept{return a.size==b.size&&equal(reinterpret_cast<const uint8_t *>(a.bytes.data()),reinterpret_cast<const uint8_t *>(b.bytes.data()),a.size);}
bool credential_valid(const context_store_registry_lab_credential & c) noexcept { return valid_id(c.key_id)&&c.generation&&nonzero(c.secret); }

struct writer {
    uint8_t * p; size_t cap,n=0;
    bool raw(const void*x,size_t z){if((!x&&z)||!p||z>cap-n)return false;std::memcpy(p+n,x,z);n+=z;return true;}
    bool head(uint8_t m,uint64_t v){if(v<24){uint8_t b=uint8_t(m<<5|v);return raw(&b,1);}size_t z=v<=UINT8_MAX?1:v<=UINT16_MAX?2:v<=UINT32_MAX?4:8;uint8_t b[9]={uint8_t(m<<5|(z==1?24:z==2?25:z==4?26:27))};for(size_t i=0;i<z;++i)b[z-i]=uint8_t(v>>(8*i));return raw(b,z+1);}
    bool u64(uint64_t v){return head(0,v);}bool tstr(const void*x,size_t z){return head(3,z)&&raw(x,z);}bool bstr(const void*x,size_t z){return head(2,z)&&raw(x,z);}
    bool be(uint64_t v,size_t z){uint8_t b[8]{};for(size_t i=0;i<z;++i)b[z-1-i]=uint8_t(v>>(8*i));return raw(b,z);}
};
struct reader {
    const uint8_t*p;size_t size,off=0;
    bool head(uint8_t m,uint64_t&v){if(off>=size)return false;uint8_t f=p[off++];if((f>>5)!=m)return false;uint8_t a=f&31;if(a<24){v=a;return true;}size_t z=a==24?1:a==25?2:a==26?4:a==27?8:0;if(!z||z>size-off)return false;v=0;for(size_t i=0;i<z;++i)v=(v<<8)|p[off++];return !((z==1&&v<24)||(z==2&&v<=UINT8_MAX)||(z==4&&v<=UINT16_MAX)||(z==8&&v<=UINT32_MAX));}
    bool exact(uint64_t v){uint64_t x=0;return head(0,x)&&x==v;}bool u64(uint64_t&v){return head(0,v);}
    bool bytes(uint8_t m,const uint8_t*&x,size_t&z){uint64_t n=0;if(!head(m,n)||n>size-off)return false;x=p+off;z=size_t(n);off+=z;return true;}
    bool digest(context_store_format_digest&v){const uint8_t*x;size_t z;if(!bytes(2,x,z)||z!=32)return false;std::copy_n(x,32,v.begin());return true;}
    bool optional_digest(context_store_format_digest&v,bool&has){if(off<size&&p[off]==0xf6){++off;has=false;return true;}has=true;return digest(v);}
    bool id(context_store_registered_id&v){const uint8_t*x;size_t z;if(!bytes(3,x,z)||!z||z>128)return false;v.size=uint8_t(z);std::copy_n(x,z,v.bytes.begin());return valid_id(v);}
};

bool hash_domain(const char*domain,const uint8_t*p,size_t n,context_store_format_digest&out){std::array<uint8_t,4160>b{};size_t d=std::strlen(domain)+1;if(d+n>b.size())return false;writer w{b.data(),b.size()};bool ok=w.raw(domain,d)&&w.raw(p,n)&&context_store_sha256_bounded(b.data(),w.n,b.size(),out);wipe(b.data(),b.size());return ok;}
bool derive(const context_store_registry_lab_credential&c,context_store_format_digest&out){std::array<uint8_t,256>b{};writer w{b.data(),b.size()};bool ok=credential_valid(c)&&w.raw(key_domain,sizeof(key_domain))&&w.tstr(c.key_id.bytes.data(),c.key_id.size)&&w.u64(c.generation)&&context_store_hmac_sha256(c.secret.data(),c.secret.size(),b.data(),w.n,out);wipe(b.data(),b.size());return ok;}
bool witness_credential_commitment(const context_store_registry_lab_credential&c,context_store_format_digest&out){std::array<uint8_t,256>b{};writer w{b.data(),b.size()};bool ok=credential_valid(c)&&w.raw(witness_credential_domain,sizeof(witness_credential_domain))&&w.tstr(c.key_id.bytes.data(),c.key_id.size)&&w.u64(c.generation)&&context_store_hmac_sha256(c.secret.data(),c.secret.size(),b.data(),w.n,out);wipe(b.data(),b.size());return ok;}
bool witness_credential_matches(const context_store_registry_lab_credential&c,const context_store_format_digest&expected){context_store_format_digest actual{};bool ok=witness_credential_commitment(c,actual)&&equal(actual.data(),expected.data(),actual.size());wipe(actual.data(),actual.size());return ok;}
bool registry_digest(const uint8_t*p,size_t n,context_store_format_digest&out){return p&&n&&n<=1024&&hash_domain(registry_domain,p,n,out);}

struct body_view { std::array<uint64_t,18> u{}; std::array<context_store_format_digest,18>d{}; std::array<bool,18>has{}; std::array<const uint8_t*,18> bytes{}; std::array<size_t,18> sizes{}; context_store_registered_id id5,id10,id11; };

bool digest_eq(const context_store_format_digest&a,const context_store_format_digest&b){return equal(a.data(),b.data(),32);}
bool parse_outer(reader&r,context_store_registry_lab_kind kind,const context_store_registry_lab_credential&c,size_t&auth_off,size_t&auth_size,const uint8_t*&tag){uint64_t n=0,alg=0,gen=0;context_store_registered_id id;if(!r.head(5,n)||n!=2||!r.exact(0))return false;auth_off=r.off;if(!r.head(5,n)||n!=4||!r.exact(0))return false;/* body parsed by caller */(void)kind;(void)c;(void)auth_size;(void)tag;(void)alg;(void)gen;(void)id;return true;}

bool parse_body(reader&r,context_store_registry_lab_kind kind,body_view&b){uint64_t n=0;const size_t counts[]={18,13,17,14,14,13};size_t count=counts[size_t(kind)];if(!r.head(5,n)||n!=count)return false;for(size_t k=0;k<count;++k){if(!r.exact(k))return false;switch(kind){
case context_store_registry_lab_kind::root:
 if(k==3||k==13||k==15){if(k==15){bool h=false;if(!r.optional_digest(b.d[k],h))return false;b.has[k]=h;}else if(!r.digest(b.d[k]))return false;}else if(k==4||k==7){const uint8_t*x;size_t z;if(!r.bytes(2,x,z)||z!=16)return false;b.bytes[k]=x;b.sizes[k]=z;}else if(k==5){if(!r.id(b.id5))return false;}else if(k==10){if(!r.id(b.id10))return false;}else if(!r.u64(b.u[k]))return false;break;
case context_store_registry_lab_kind::head:
 if(k>=3&&k<=5){if(!r.digest(b.d[k]))return false;}else if(k==7){if(!r.id(b.id5))return false;}else if(k==11){if(!r.id(b.id11))return false;}else if(!r.u64(b.u[k]))return false;break;
case context_store_registry_lab_kind::prepare:
 if(k==3||k==4||k==5||k==7||k==10||k==13||k==15||k==16){if(!r.digest(b.d[k]))return false;}else if(k==9||k==12){const uint8_t*x;size_t z;if(!r.bytes(2,x,z)||!z||z>1024)return false;b.bytes[k]=x;b.sizes[k]=z;}else if(!r.u64(b.u[k]))return false;break;
case context_store_registry_lab_kind::close: case context_store_registry_lab_kind::abort_record:
 if(k==3||k==4||k==5||k==7||k==8||k==9||k==10||k==13){if(!r.digest(b.d[k]))return false;}else if(!r.u64(b.u[k]))return false;break;
case context_store_registry_lab_kind::quarantine:
 if(k==3||k==4||k==5){if(!r.digest(b.d[k]))return false;}else if(k==6||k==10||k==11||k==12){bool h=false;if(!r.optional_digest(b.d[k],h))return false;b.has[k]=h;}else if(k==7){if(r.off<r.size&&r.p[r.off]==0xf6){++r.off;b.has[k]=false;}else{b.has[k]=true;if(!r.u64(b.u[k]))return false;}}else if(!r.u64(b.u[k]))return false;break;
 }}return true;}

bool operation(const context_store_registry_lab_expectation&e,context_store_format_digest&out){std::array<uint8_t,512>b{};writer w{b.data(),b.size()};bool ok=w.head(5,8)&&w.u64(0)&&w.bstr(e.root_id.data(),32)&&w.u64(1)&&w.bstr(e.path_policy_commitment.data(),32)&&w.u64(2)&&w.bstr(e.attempt_id.data(),32)&&w.u64(3)&&w.u64(e.slot)&&w.u64(4)&&w.bstr(e.predecessor_envelope_digest.data(),32)&&w.u64(5)&&w.bstr(e.successor_envelope_digest.data(),32)&&w.u64(6)&&w.u64(e.predecessor_size)&&w.u64(7)&&w.u64(e.successor_size)&&hash_domain(operation_domain,b.data(),w.n,out);wipe(b.data(),b.size());return ok;}

bool base_expectation(const context_store_registry_lab_expectation&e) noexcept {
    return nonzero(e.root_id)&&nonzero(e.path_policy_commitment);
}

bool root_expectation(const context_store_registry_lab_expectation&e) noexcept {
    return base_expectation(e)&&valid_id(e.registry_id)&&e.registry_epoch&&e.mount_id&&
      e.lock_st_dev&&e.lock_st_ino;
}

bool expected_predecessor(const context_store_registry_lab_credential&c,const context_store_registry_lab_expectation&e){
    if(!credential_valid(c)||!root_expectation(e)||!e.predecessor_selector_generation||
       !nonzero(e.predecessor_envelope_digest)||!e.predecessor||!e.predecessor_size||e.predecessor_size>1024)return false;
    context_store_format_digest digest_value{};
    if(!registry_digest(e.predecessor,e.predecessor_size,digest_value)||
       !digest_eq(digest_value,e.predecessor_envelope_digest))return false;
    context_store_protected_registry_key_record key{context_store_key_disposition::active,c.key_id,c.generation,{c.secret.data(),c.secret.size()}};
    auto verified=context_store_verify_protected_registry_v1(e.predecessor,e.predecessor_size,key);
    auto*carrier=verified.authenticated_carrier();auto*body=carrier?carrier->body():nullptr;
    return body&&same_id(body->registry_id,e.registry_id)&&body->registry_epoch==e.registry_epoch&&
      body->last_consumed_sequence==e.predecessor_high_water;
}
bool expected_successor_pair(const context_store_registry_lab_credential&c,const context_store_registry_lab_expectation&e){
    if(!expected_predecessor(c,e)||e.predecessor_high_water==UINT64_MAX||!e.successor||!e.successor_size||e.successor_size>1024||
       !nonzero(e.successor_envelope_digest)||e.predecessor_selector_generation==UINT64_MAX||
       e.successor_selector_generation!=e.predecessor_selector_generation+1)return false;
    context_store_format_digest digest_value{};if(!registry_digest(e.successor,e.successor_size,digest_value)||!digest_eq(digest_value,e.successor_envelope_digest))return false;
    context_store_protected_registry_key_record key{context_store_key_disposition::active,c.key_id,c.generation,{c.secret.data(),c.secret.size()}};
    auto predecessor=context_store_verify_protected_registry_v1(e.predecessor,e.predecessor_size,key);auto successor=context_store_verify_protected_registry_successor_v1(e.successor,e.successor_size,key);
    auto*pc=predecessor.authenticated_carrier();auto*sc=successor.authenticated_carrier();auto*pb=pc?pc->body():nullptr;auto*sb=sc?sc->body():nullptr;
    return pb&&sb&&pc->envelope_digest()&&pc->key_continuity_commitment()&&sc->key_continuity_commitment()&&same_id(sb->registry_id,e.registry_id)&&
      sb->registry_epoch==e.registry_epoch&&sb->authority_base_scope_commitment==pb->authority_base_scope_commitment&&sb->policy_commitment==pb->policy_commitment&&
      sb->consumed_authorization_high_water==e.predecessor_high_water+1&&sb->receipt.authorization_sequence==sb->consumed_authorization_high_water&&
      sb->predecessor_snapshot_envelope_digest==*pc->envelope_digest()&&*pc->key_continuity_commitment()==*sc->key_continuity_commitment();
}

bool expected_transition(const context_store_registry_lab_credential&c,const context_store_registry_lab_expectation&e){
    if(!credential_valid(c)||!root_expectation(e)||e.predecessor_high_water==UINT64_MAX||
       !e.predecessor_selector_generation||
       e.predecessor_selector_generation==UINT64_MAX||
       e.successor_selector_generation!=e.predecessor_selector_generation+1||
       !nonzero(e.initial_head_digest)||
       !nonzero(e.predecessor_envelope_digest)||!nonzero(e.successor_envelope_digest)||
       !nonzero(e.prepare_digest)||!nonzero(e.operation_commitment)||!nonzero(e.attempt_id)||
       e.slot>511||!e.predecessor||!e.successor||!e.predecessor_size||!e.successor_size||
       e.predecessor_size>1024||e.successor_size>1024)return false;
    context_store_format_digest pd{},sd{},op{};
    if(!registry_digest(e.predecessor,e.predecessor_size,pd)||
       !registry_digest(e.successor,e.successor_size,sd)||
       !digest_eq(pd,e.predecessor_envelope_digest)||
       !digest_eq(sd,e.successor_envelope_digest)||
       !operation(e,op)||!digest_eq(op,e.operation_commitment))return false;
    context_store_protected_registry_key_record key{context_store_key_disposition::active,c.key_id,c.generation,{c.secret.data(),c.secret.size()}};
    auto p=context_store_verify_protected_registry_v1(e.predecessor,e.predecessor_size,key);
    auto s=context_store_verify_protected_registry_successor_v1(e.successor,e.successor_size,key);
    auto*pc=p.authenticated_carrier();auto*sc=s.authenticated_carrier();
    if(!pc||!sc||!pc->body()||!sc->body()||!pc->envelope_digest()||!sc->envelope_digest()||
       !pc->key_continuity_commitment()||!sc->key_continuity_commitment())return false;
    auto*pb=pc->body();auto*sb=sc->body();
    return same_id(pb->registry_id,e.registry_id)&&same_id(sb->registry_id,e.registry_id)&&
      pb->registry_epoch==e.registry_epoch&&sb->registry_epoch==e.registry_epoch&&
      pb->authority_base_scope_commitment==sb->authority_base_scope_commitment&&
      pb->policy_commitment==sb->policy_commitment&&
      pb->last_consumed_sequence==e.predecessor_high_water&&
      sb->consumed_authorization_high_water==e.predecessor_high_water+1&&
      sb->receipt.authorization_sequence==sb->consumed_authorization_high_water&&
      sb->predecessor_snapshot_envelope_digest==*pc->envelope_digest()&&
      *pc->key_continuity_commitment()==*sc->key_continuity_commitment();
}

bool head_expectation(const body_view&b,const context_store_registry_lab_credential&c,const context_store_registry_lab_expectation&e){
    if(!base_expectation(e)||!valid_id(e.registry_id)||!e.registry_epoch||
       !same_id(b.id5,e.registry_id)||b.u[8]!=e.registry_epoch||
       !same_id(b.id11,c.key_id)||b.u[12]!=c.generation)return false;
    bool pred=nonzero(e.predecessor_envelope_digest)&&digest_eq(b.d[5],e.predecessor_envelope_digest);
    bool succ=nonzero(e.successor_envelope_digest)&&digest_eq(b.d[5],e.successor_envelope_digest);
    if(pred==succ)return false;
    const uint8_t*x=pred?e.predecessor:e.successor;
    size_t z=pred?e.predecessor_size:e.successor_size;
    uint64_t high=pred?e.predecessor_high_water:e.predecessor_high_water+1;
    uint64_t generation=pred?e.predecessor_selector_generation:e.successor_selector_generation;
    if(!x||!z||z>1024||e.predecessor_high_water==UINT64_MAX||!generation||b.u[9]!=high||b.u[10]!=generation||b.u[6]!=z)return false;
    context_store_format_digest d{};
    if(!registry_digest(x,z,d)||!digest_eq(d,b.d[5]))return false;
    context_store_protected_registry_key_record key{context_store_key_disposition::active,c.key_id,c.generation,{c.secret.data(),c.secret.size()}};
    if(pred){
        auto verified=context_store_verify_protected_registry_v1(x,z,key);auto*carrier=verified.authenticated_carrier();
        auto*body=carrier?carrier->body():nullptr;
        return body&&same_id(body->registry_id,e.registry_id)&&body->registry_epoch==e.registry_epoch&&body->last_consumed_sequence==high;
    }
    auto verified=context_store_verify_protected_registry_successor_v1(x,z,key);auto*carrier=verified.authenticated_carrier();
    auto*body=carrier?carrier->body():nullptr;
    return body&&same_id(body->registry_id,e.registry_id)&&body->registry_epoch==e.registry_epoch&&
      body->consumed_authorization_high_water==high&&body->receipt.authorization_sequence==high;
}

bool expectation_valid(context_store_registry_lab_kind k,const context_store_registry_lab_credential&c,const context_store_registry_lab_expectation&e){
    if(!credential_valid(c)||!base_expectation(e))return false;
    if(k==context_store_registry_lab_kind::root)return root_expectation(e);
    if(k==context_store_registry_lab_kind::head)return valid_id(e.registry_id)&&e.registry_epoch;
    if(k==context_store_registry_lab_kind::prepare||k==context_store_registry_lab_kind::close||k==context_store_registry_lab_kind::abort_record)return expected_transition(c,e);
    return nonzero(e.quarantine_event_id)&&e.quarantine_reason<=15&&e.quarantine_phase<=2&&
      (!e.quarantine_has_previous_record||nonzero(e.quarantine_previous_record_digest))&&
      (!e.quarantine_has_head||nonzero(e.quarantine_head_digest))&&
      (!e.quarantine_attributable||(nonzero(e.attempt_id)&&e.slot<=511&&nonzero(e.operation_commitment)));
}

bool inner_transition(const body_view&b,const context_store_registry_lab_expectation&e){
    return b.sizes[9]==e.predecessor_size&&b.sizes[12]==e.successor_size&&
      equal(b.bytes[9],e.predecessor,e.predecessor_size)&&
      equal(b.bytes[12],e.successor,e.successor_size);
}

bool semantics(context_store_registry_lab_kind k,const body_view&b,const context_store_registry_lab_credential&c,const context_store_registry_lab_expectation&e){if(b.u[0]!=1||b.u[1]!=0||b.u[2]!=uint64_t(k))return false;if(!digest_eq(b.d[3],e.root_id))return false;if(k==context_store_registry_lab_kind::root){return b.sizes[4]==16&&equal(b.bytes[4],e.store_uuid.data(),16)&&same_id(b.id5,e.registry_id)&&b.u[6]==e.registry_epoch&&b.sizes[7]==16&&equal(b.bytes[7],e.filesystem_uuid.data(),16)&&b.u[8]==e.mount_id&&b.u[9]==e.owner_uid&&same_id(b.id10,c.key_id)&&b.u[11]==c.generation&&b.u[12]==512&&digest_eq(b.d[13],e.path_policy_commitment)&&((b.u[14]==0&&!b.has[15])||(b.u[14]==1&&b.has[15]&&digest_eq(b.d[15],e.initial_head_digest)))&&b.u[16]==e.lock_st_dev&&b.u[17]==e.lock_st_ino;}
if(!digest_eq(b.d[4],e.path_policy_commitment))return false;
if(k==context_store_registry_lab_kind::head){return head_expectation(b,c,e);}
if(k==context_store_registry_lab_kind::prepare){context_store_format_digest pd{},sd{},op{};return nonzero(b.d[5])&&b.u[6]<=511&&b.u[6]==e.slot&&b.sizes[9]==b.u[8]&&b.sizes[12]==b.u[11]&&registry_digest(b.bytes[9],b.sizes[9],pd)&&registry_digest(b.bytes[12],b.sizes[12],sd)&&digest_eq(pd,b.d[10])&&digest_eq(sd,b.d[13])&&digest_eq(b.d[5],e.attempt_id)&&operation(e,op)&&digest_eq(b.d[7],op)&&digest_eq(op,e.operation_commitment)&&b.u[14]==1&&digest_eq(b.d[15],e.initial_head_digest)&&digest_eq(b.d[16],e.initial_head_digest)&&inner_transition(b,e);}
if(k==context_store_registry_lab_kind::close||k==context_store_registry_lab_kind::abort_record){bool close=k==context_store_registry_lab_kind::close;return digest_eq(b.d[5],e.attempt_id)&&b.u[6]==e.slot&&digest_eq(b.d[7],e.operation_commitment)&&digest_eq(b.d[8],e.predecessor_envelope_digest)&&digest_eq(b.d[9],e.successor_envelope_digest)&&digest_eq(b.d[10],e.prepare_digest)&&b.u[11]==(close?2:1)&&b.u[12]<=1&&digest_eq(b.d[13],close?e.successor_head_digest:e.initial_head_digest);}
return digest_eq(b.d[5],e.quarantine_event_id)&&
  b.has[6]==e.quarantine_attributable&&b.has[7]==e.quarantine_attributable&&
  b.has[12]==e.quarantine_attributable&&
  (!e.quarantine_attributable||(digest_eq(b.d[6],e.attempt_id)&&b.u[7]==e.slot&&digest_eq(b.d[12],e.operation_commitment)))&&
  b.u[8]==e.quarantine_reason&&b.u[9]==e.quarantine_phase&&
  b.has[10]==e.quarantine_has_previous_record&&
  (!b.has[10]||digest_eq(b.d[10],e.quarantine_previous_record_digest))&&
  b.has[11]==e.quarantine_has_head&&
  (!b.has[11]||digest_eq(b.d[11],e.quarantine_head_digest));
}

bool range_ok(const void * p,size_t n) noexcept {
    if(!p||!n)return false;
    const uintptr_t a=reinterpret_cast<uintptr_t>(p);
    return a<=UINTPTR_MAX-n;
}
bool ranges_overlap(const void*a,size_t an,const void*b,size_t bn) noexcept {
    if(!an||!bn)return false;
    if(!range_ok(a,an)||!range_ok(b,bn))return true;
    const uintptr_t ab=reinterpret_cast<uintptr_t>(a),bb=reinterpret_cast<uintptr_t>(b);
    return ab<bb+bn&&bb<ab+an;
}
bool output_admitted(uint8_t*out,size_t cap,size_t maximum,const void*encoded_size,const void*credential,size_t credential_size,const void*witness,size_t witness_size) noexcept {
    return cap&&cap<=maximum&&range_ok(out,cap)&&
      !ranges_overlap(out,cap,encoded_size,sizeof(size_t))&&
      !ranges_overlap(out,cap,credential,credential_size)&&!ranges_overlap(out,cap,witness,witness_size)&&
      !ranges_overlap(encoded_size,sizeof(size_t),credential,credential_size)&&
      !ranges_overlap(encoded_size,sizeof(size_t),witness,witness_size);
}
bool null_value(writer&w){const uint8_t n=0xf6;return w.raw(&n,1);}

bool encode_body(context_store_registry_lab_kind k,const context_store_registry_lab_expectation&e,
                 const context_store_registry_lab_credential&c,bool choice,writer&w) {
    const size_t counts[]={18,13,17,14,14,13};
    if(!w.head(5,counts[size_t(k)]))return false;
    auto key=[&](uint64_t v){return w.u64(v);};
    auto field_u=[&](uint64_t key_number,uint64_t value){return key(key_number)&&w.u64(value);};
    auto field_d=[&](uint64_t key_number,const context_store_format_digest&value){return key(key_number)&&w.bstr(value.data(),value.size());};
    auto field_id=[&](uint64_t key_number,const context_store_registered_id&value){return key(key_number)&&w.tstr(value.bytes.data(),value.size);};
    if(!field_u(0,1)||!field_u(1,0)||!field_u(2,uint64_t(k))||!field_d(3,e.root_id))return false;
    if(k==context_store_registry_lab_kind::root){
        return key(4)&&w.bstr(e.store_uuid.data(),e.store_uuid.size())&&field_id(5,e.registry_id)&&
          field_u(6,e.registry_epoch)&&key(7)&&w.bstr(e.filesystem_uuid.data(),e.filesystem_uuid.size())&&
          field_u(8,e.mount_id)&&field_u(9,e.owner_uid)&&field_id(10,c.key_id)&&field_u(11,c.generation)&&
          field_u(12,512)&&field_d(13,e.path_policy_commitment)&&field_u(14,choice)&&key(15)&&
          (choice?w.bstr(e.initial_head_digest.data(),32):null_value(w))&&field_u(16,e.lock_st_dev)&&field_u(17,e.lock_st_ino);
    }
    if(!field_d(4,e.path_policy_commitment))return false;
    if(k==context_store_registry_lab_kind::head){
        const auto&d=choice?e.successor_envelope_digest:e.predecessor_envelope_digest;
        const size_t z=choice?e.successor_size:e.predecessor_size;
        return field_d(5,d)&&field_u(6,z)&&field_id(7,e.registry_id)&&field_u(8,e.registry_epoch)&&
          field_u(9,e.predecessor_high_water+(choice?1:0))&&
          field_u(10,choice?e.successor_selector_generation:e.predecessor_selector_generation)&&
          field_id(11,c.key_id)&&field_u(12,c.generation);
    }
    if(k==context_store_registry_lab_kind::prepare){
        return field_d(5,e.attempt_id)&&field_u(6,e.slot)&&field_d(7,e.operation_commitment)&&
          field_u(8,e.predecessor_size)&&key(9)&&w.bstr(e.predecessor,e.predecessor_size)&&
          field_d(10,e.predecessor_envelope_digest)&&field_u(11,e.successor_size)&&
          key(12)&&w.bstr(e.successor,e.successor_size)&&field_d(13,e.successor_envelope_digest)&&
          field_u(14,1)&&field_d(15,e.initial_head_digest)&&field_d(16,e.initial_head_digest);
    }
    if(k==context_store_registry_lab_kind::close||k==context_store_registry_lab_kind::abort_record){
        const bool close=k==context_store_registry_lab_kind::close;
        return field_d(5,e.attempt_id)&&field_u(6,e.slot)&&field_d(7,e.operation_commitment)&&
          field_d(8,e.predecessor_envelope_digest)&&field_d(9,e.successor_envelope_digest)&&
          field_d(10,e.prepare_digest)&&field_u(11,close?2:1)&&field_u(12,choice?1:0)&&
          field_d(13,close?e.successor_head_digest:e.initial_head_digest);
    }
    return field_d(5,e.quarantine_event_id)&&key(6)&&
      (e.quarantine_attributable?w.bstr(e.attempt_id.data(),32):null_value(w))&&key(7)&&
      (e.quarantine_attributable?w.u64(e.slot):null_value(w))&&field_u(8,e.quarantine_reason)&&
      field_u(9,e.quarantine_phase)&&key(10)&&
      (e.quarantine_has_previous_record?w.bstr(e.quarantine_previous_record_digest.data(),32):null_value(w))&&key(11)&&
      (e.quarantine_has_head?w.bstr(e.quarantine_head_digest.data(),32):null_value(w))&&key(12)&&
      (e.quarantine_attributable?w.bstr(e.operation_commitment.data(),32):null_value(w));
}

context_store_registry_lab_wire_result encode_private(
        context_store_registry_lab_kind k,bool choice,const context_store_registry_lab_credential&c,
        const context_store_registry_lab_expectation&e,uint8_t*out,size_t cap,size_t&encoded,
        bool derive_prepare_digest=false) noexcept {
    context_store_registry_lab_wire_result failure;
    std::array<uint8_t,context_store_registry_lab_wire_max_bytes> body{},scratch{};
    std::array<uint8_t,context_store_registry_lab_wire_max_bytes+64> message{};
    context_store_format_digest derived{},tag{};
    writer bw{body.data(),body.size()};
    writer sw{scratch.data(),scratch.size()};
    bool ok=encode_body(k,e,c,choice,bw)&&sw.head(5,2)&&sw.u64(0);
    const size_t auth_off=sw.n;
    ok=ok&&sw.head(5,4)&&sw.u64(0)&&sw.raw(body.data(),bw.n)&&sw.u64(1)&&
      sw.tstr(c.key_id.bytes.data(),c.key_id.size)&&sw.u64(2)&&sw.u64(1)&&sw.u64(3)&&sw.u64(c.generation);
    const size_t auth_size=sw.n-auth_off;
    writer mw{message.data(),message.size()};
    ok=ok&&derive(c,derived)&&mw.raw(auth_domains[size_t(k)],std::strlen(auth_domains[size_t(k)])+1)&&
      mw.raw(scratch.data()+auth_off,auth_size)&&
      context_store_hmac_sha256(derived.data(),derived.size(),message.data(),mw.n,tag)&&
      sw.u64(1)&&sw.bstr(tag.data(),tag.size())&&sw.n<=cap;
    context_store_registry_lab_expectation verification=e;
    if(ok&&derive_prepare_digest&&k==context_store_registry_lab_kind::prepare&&!nonzero(verification.prepare_digest)) {
        ok=hash_domain(content_domains[size_t(k)],scratch.data(),sw.n,verification.prepare_digest);
    }
    context_store_registry_lab_wire_result verified;
    if(ok)verified=context_store_registry_lab_verify_v1(k,scratch.data(),sw.n,c,verification);
    if(verified.authenticated()&&k==context_store_registry_lab_kind::head){
        const auto&expected=choice?e.successor_head_digest:e.initial_head_digest;
        if(nonzero(expected)&&!digest_eq(verified.content_digest,expected))verified=failure;
    }
    if(verified.authenticated()&&k==context_store_registry_lab_kind::prepare&&
       !digest_eq(verified.content_digest,verification.prepare_digest))verified=failure;
    if(!ok||!verified.authenticated())verified=failure;
    if(verified.authenticated()){
        std::memcpy(out,scratch.data(),sw.n);
        encoded=sw.n;
    }
    wipe(derived.data(),derived.size());wipe(tag.data(),tag.size());
    wipe(message.data(),message.size());wipe(body.data(),body.size());wipe(scratch.data(),scratch.size());
    return verified;
}
}

void context_store_registry_lab_credential::clear() noexcept {
    wipe(secret.data(), secret.size());
    wipe(key_id.bytes.data(), key_id.bytes.size());
    key_id.size = 0;
    generation = 0;
}

context_store_registry_lab_credential::~context_store_registry_lab_credential() noexcept { clear(); }

context_store_registry_lab_credential::context_store_registry_lab_credential(
        context_store_registry_lab_credential && other) noexcept {
    key_id = other.key_id;
    generation = other.generation;
    secret = other.secret;
    other.clear();
}

context_store_registry_lab_credential & context_store_registry_lab_credential::operator=(
        context_store_registry_lab_credential && other) noexcept {
    if (this != &other) {
        clear();
        key_id = other.key_id;
        generation = other.generation;
        secret = other.secret;
        other.clear();
    }
    return *this;
}

void context_store_registry_lab_lifecycle_witness::clear() noexcept {
    wipe(predecessor_.data(),predecessor_.size());wipe(successor_.data(),successor_.size());
    wipe(predecessor_head_.data(),predecessor_head_.size());wipe(successor_head_.data(),successor_head_.size());
    wipe(observed_head_.data(),observed_head_.size());wipe(observed_resolved_.data(),observed_resolved_.size());
    wipe(prepare_.data(),prepare_.size());
    expectation_={};wipe(credential_commitment_.data(),credential_commitment_.size());predecessor_size_=successor_size_=0;
    predecessor_head_size_=successor_head_size_=prepare_size_=0;observed_head_size_=observed_resolved_size_=0;
    admitted_=predecessor_ready_=successor_ready_=false;kind_tag_=variant_=0xff;
}
context_store_registry_lab_lifecycle_witness::~context_store_registry_lab_lifecycle_witness() noexcept { clear(); }
context_store_registry_lab_lifecycle_witness::context_store_registry_lab_lifecycle_witness(
        context_store_registry_lab_lifecycle_witness&&other) noexcept { *this=std::move(other); }
context_store_registry_lab_lifecycle_witness&context_store_registry_lab_lifecycle_witness::operator=(
        context_store_registry_lab_lifecycle_witness&&other) noexcept {
    if(this!=&other){
        clear();expectation_=other.expectation_;credential_commitment_=other.credential_commitment_;predecessor_=other.predecessor_;successor_=other.successor_;
        predecessor_head_=other.predecessor_head_;successor_head_=other.successor_head_;observed_head_=other.observed_head_;observed_resolved_=other.observed_resolved_;prepare_=other.prepare_;
        predecessor_size_=other.predecessor_size_;successor_size_=other.successor_size_;
        predecessor_head_size_=other.predecessor_head_size_;successor_head_size_=other.successor_head_size_;
        prepare_size_=other.prepare_size_;observed_head_size_=other.observed_head_size_;observed_resolved_size_=other.observed_resolved_size_;admitted_=other.admitted_;predecessor_ready_=other.predecessor_ready_;successor_ready_=other.successor_ready_;kind_tag_=other.kind_tag_;variant_=other.variant_;
        expectation_.predecessor=predecessor_size_?predecessor_.data():nullptr;
        expectation_.successor=successor_size_?successor_.data():nullptr;
        other.clear();
    }
    return *this;
}

bool context_store_registry_lab_lifecycle_witness::admit(
        const context_store_registry_lab_credential&c,const context_store_registry_lab_expectation&e,
        const evidence&v,context_store_registry_lab_lifecycle_witness&o) noexcept {
    const bool aliases_output=
      ranges_overlap(e.predecessor,e.predecessor_size,&o,sizeof(o))||
      ranges_overlap(e.successor,e.successor_size,&o,sizeof(o))||
      ranges_overlap(v.predecessor_head,v.predecessor_head_size,&o,sizeof(o))||
      ranges_overlap(v.successor_head,v.successor_head_size,&o,sizeof(o))||
      ranges_overlap(v.prepare,v.prepare_size,&o,sizeof(o))||ranges_overlap(v.observed_head,v.observed_head_size,&o,sizeof(o))||
      ranges_overlap(v.observed_resolved,v.observed_resolved_size,&o,sizeof(o));
    o.clear();
    auto evidence_shape=[](const uint8_t*p,size_t n,size_t maximum){return (!p&&!n)||(p&&n&&n<=maximum);};
    if(aliases_output||!credential_valid(c)||!root_expectation(e)||
       !evidence_shape(v.predecessor_head,v.predecessor_head_size,1024)||
       !evidence_shape(v.successor_head,v.successor_head_size,1024)||
       !evidence_shape(v.prepare,v.prepare_size,context_store_registry_lab_wire_max_bytes)||
       !evidence_shape(v.observed_head,v.observed_head_size,1024)||!evidence_shape(v.observed_resolved,v.observed_resolved_size,1024))return false;
    if((e.predecessor==nullptr)!=(e.predecessor_size==0)||(e.successor==nullptr)!=(e.successor_size==0)||
       e.predecessor_size>1024||e.successor_size>1024)return false;
    o.expectation_=e;
    if(e.predecessor_size){std::copy_n(e.predecessor,e.predecessor_size,o.predecessor_.begin());o.predecessor_size_=e.predecessor_size;}
    if(e.successor_size){std::copy_n(e.successor,e.successor_size,o.successor_.begin());o.successor_size_=e.successor_size;}
    o.expectation_.predecessor=o.predecessor_size_?o.predecessor_.data():nullptr;
    o.expectation_.successor=o.successor_size_?o.successor_.data():nullptr;
    if(!witness_credential_commitment(c,o.credential_commitment_))return o.clear(),false;
    o.predecessor_ready_=expected_predecessor(c,o.expectation_);
    o.successor_ready_=expected_transition(c,o.expectation_);
    auto admit_object=[&](context_store_registry_lab_kind k,const uint8_t*p,size_t n,const context_store_format_digest&expected){
        if(!p)return true;
        auto r=context_store_registry_lab_verify_v1(k,p,n,c,o.expectation_);
        return r.authenticated()&&digest_eq(r.content_digest,expected);
    };
    if(v.predecessor_head&&!admit_object(context_store_registry_lab_kind::head,v.predecessor_head,v.predecessor_head_size,e.initial_head_digest))return o.clear(),false;
    if(v.successor_head&&!admit_object(context_store_registry_lab_kind::head,v.successor_head,v.successor_head_size,e.successor_head_digest))return o.clear(),false;
    if(v.prepare&&(!o.successor_ready_||!admit_object(context_store_registry_lab_kind::prepare,v.prepare,v.prepare_size,e.prepare_digest)))return o.clear(),false;
    if(v.predecessor_head){std::copy_n(v.predecessor_head,v.predecessor_head_size,o.predecessor_head_.begin());o.predecessor_head_size_=v.predecessor_head_size;}
    if(v.successor_head){std::copy_n(v.successor_head,v.successor_head_size,o.successor_head_.begin());o.successor_head_size_=v.successor_head_size;}
    if(v.prepare){std::copy_n(v.prepare,v.prepare_size,o.prepare_.begin());o.prepare_size_=v.prepare_size;}
    if(v.observed_head){std::copy_n(v.observed_head,v.observed_head_size,o.observed_head_.begin());o.observed_head_size_=v.observed_head_size;}
    if(v.observed_resolved){std::copy_n(v.observed_resolved,v.observed_resolved_size,o.observed_resolved_.begin());o.observed_resolved_size_=v.observed_resolved_size;}
    o.admitted_=true;return true;
}

bool context_store_registry_lab_lifecycle_witness::admit_root_initializing(const context_store_registry_lab_credential&c,const context_store_registry_lab_expectation&e,context_store_registry_lab_lifecycle_witness&o) noexcept {
    if(!admit(c,e,{},o)) {
        return false;
    }
    o.kind_tag_=uint8_t(context_store_registry_lab_kind::root);
    o.variant_=0;
    return true;
}
bool context_store_registry_lab_lifecycle_witness::admit_root_initialized(const context_store_registry_lab_credential&c,const context_store_registry_lab_expectation&e,const evidence&v,context_store_registry_lab_lifecycle_witness&o) noexcept {
    if(!admit(c,e,v,o)||!o.predecessor_head_size_||o.successor_head_size_||o.prepare_size_||o.observed_head_size_||o.observed_resolved_size_||!nonzero(e.initial_head_digest)) {
        o.clear();
        return false;
    }
    o.kind_tag_=uint8_t(context_store_registry_lab_kind::root);
    o.variant_=1;
    return true;
}
bool context_store_registry_lab_lifecycle_witness::admit_head_predecessor(const context_store_registry_lab_credential&c,const context_store_registry_lab_expectation&e,context_store_registry_lab_lifecycle_witness&o) noexcept {
    if(!admit(c,e,{},o)||!o.predecessor_ready_) {
        o.clear();
        return false;
    }
    o.kind_tag_=uint8_t(context_store_registry_lab_kind::head);
    o.variant_=0;
    return true;
}
bool context_store_registry_lab_lifecycle_witness::admit_head_successor(const context_store_registry_lab_credential&c,const context_store_registry_lab_expectation&e,context_store_registry_lab_lifecycle_witness&o) noexcept {
    if(!admit(c,e,{},o)) {
        return false;
    }
    o.successor_ready_=expected_successor_pair(c,o.expectation_);
    if(!o.successor_ready_) {
        o.clear();
        return false;
    }
    o.kind_tag_=uint8_t(context_store_registry_lab_kind::head);
    o.variant_=1;
    return true;
}
bool context_store_registry_lab_lifecycle_witness::admit_prepare(const context_store_registry_lab_credential&c,const context_store_registry_lab_expectation&e,const evidence&v,context_store_registry_lab_lifecycle_witness&o) noexcept {
    if(!admit(c,e,v,o)||!o.successor_ready_||!o.predecessor_head_size_||o.successor_head_size_||o.prepare_size_||o.observed_head_size_||o.observed_resolved_size_) {
        o.clear();
        return false;
    }
    o.kind_tag_=uint8_t(context_store_registry_lab_kind::prepare);
    o.variant_=0;
    return true;
}
bool context_store_registry_lab_lifecycle_witness::admit_close(const context_store_registry_lab_credential&c,const context_store_registry_lab_expectation&e,const evidence&v,uint8_t variant,context_store_registry_lab_lifecycle_witness&o) noexcept {
    if(variant>1||!admit(c,e,v,o)||!o.successor_ready_||!o.predecessor_head_size_||!o.successor_head_size_||!o.prepare_size_||o.observed_head_size_||o.observed_resolved_size_) {
        o.clear();
        return false;
    }
    o.kind_tag_=uint8_t(context_store_registry_lab_kind::close);
    o.variant_=variant;
    return true;
}
bool context_store_registry_lab_lifecycle_witness::abort_mismatch_evidence(const context_store_registry_lab_credential&c,const context_store_registry_lab_expectation&e,const evidence&v) noexcept {
    if(!v.observed_head||!v.observed_resolved||!v.observed_head_size||!v.observed_resolved_size||v.observed_head_size>1024||v.observed_resolved_size>1024||
       (v.observed_resolved_size==e.predecessor_size&&equal(v.observed_resolved,e.predecessor,e.predecessor_size)))return false;
    context_store_protected_registry_key_record key{context_store_key_disposition::active,c.key_id,c.generation,{c.secret.data(),c.secret.size()}};
    auto inner=context_store_verify_protected_registry_v1(v.observed_resolved,v.observed_resolved_size,key);auto*carrier=inner.authenticated_carrier();auto*body=carrier?carrier->body():nullptr;
    if(!body||!same_id(body->registry_id,e.registry_id)||body->registry_epoch!=e.registry_epoch)return false;
    context_store_registry_lab_expectation observed=e;observed.predecessor=v.observed_resolved;observed.predecessor_size=v.observed_resolved_size;observed.predecessor_high_water=body->last_consumed_sequence;
    if(!registry_digest(v.observed_resolved,v.observed_resolved_size,observed.predecessor_envelope_digest))return false;
    auto head=context_store_registry_lab_verify_v1(context_store_registry_lab_kind::head,v.observed_head,v.observed_head_size,c,observed);
    return head.authenticated()&&digest_eq(head.content_digest,e.initial_head_digest);
}
bool context_store_registry_lab_lifecycle_witness::admit_abort(const context_store_registry_lab_credential&c,const context_store_registry_lab_expectation&e,const evidence&v,uint8_t variant,context_store_registry_lab_lifecycle_witness&o) noexcept{
    if(variant>1||!admit(c,e,v,o)||!o.successor_ready_||o.successor_head_size_||!o.prepare_size_||
       (variant?(!o.predecessor_head_size_||o.observed_head_size_||o.observed_resolved_size_):(o.predecessor_head_size_||!abort_mismatch_evidence(c,e,v)))) {
        o.clear();
        return false;
    }
    o.kind_tag_=uint8_t(context_store_registry_lab_kind::abort_record);
    o.variant_=variant;
    return true;
}
bool context_store_registry_lab_lifecycle_witness::admit_quarantine(const context_store_registry_lab_credential&c,const context_store_registry_lab_expectation&e,const evidence&v,context_store_registry_lab_lifecycle_witness&o) noexcept {
    if(!admit(c,e,v,o)||!expectation_valid(context_store_registry_lab_kind::quarantine,c,o.expectation_)||o.observed_head_size_||o.observed_resolved_size_)return o.clear(),false;
    if(e.quarantine_attributable&&(!o.successor_ready_||!o.prepare_size_||!o.predecessor_head_size_))return o.clear(),false;
    if(e.quarantine_has_previous_record&&(!o.prepare_size_||!o.predecessor_head_size_||!digest_eq(e.quarantine_previous_record_digest,e.prepare_digest)))return o.clear(),false;
    if(e.quarantine_has_head){bool pred=o.predecessor_head_size_&&digest_eq(e.quarantine_head_digest,e.initial_head_digest);bool succ=o.successor_head_size_&&digest_eq(e.quarantine_head_digest,e.successor_head_digest);if(pred==succ)return o.clear(),false;}
    if(!e.quarantine_has_previous_record&&o.prepare_size_&&!e.quarantine_attributable)return o.clear(),false;
    if(!e.quarantine_has_head&&(o.successor_head_size_||(o.predecessor_head_size_&&!o.prepare_size_)))return o.clear(),false;
    o.kind_tag_=uint8_t(context_store_registry_lab_kind::quarantine);
    o.variant_=0;
    return true;
}

context_store_registry_lab_wire_result context_store_registry_lab_lifecycle_witness::encode(
        context_store_registry_lab_kind kind,const context_store_registry_lab_credential&c,
        const context_store_registry_lab_lifecycle_witness&w,uint8_t*out,size_t cap,size_t&encoded) noexcept {
    if(!w.admitted_||w.kind_tag_!=uint8_t(kind)||!witness_credential_matches(c,w.credential_commitment_)) {
        return {};
    }
    size_t maximum=1024;
    bool variant=w.variant_==1;
    switch(kind){
    case context_store_registry_lab_kind::root:
        if(w.variant_>1||(variant&&(!w.predecessor_head_size_||!nonzero(w.expectation_.initial_head_digest)))) {
            return {};
        }
        break;
    case context_store_registry_lab_kind::head:
        if(w.variant_>1||!(variant?w.successor_ready_:w.predecessor_ready_)) {
            return {};
        }
        break;
    case context_store_registry_lab_kind::prepare:
        maximum=context_store_registry_lab_wire_max_bytes;
        if(w.variant_||!w.successor_ready_||!w.predecessor_head_size_) {
            return {};
        }
        break;
    case context_store_registry_lab_kind::close:
        if(w.variant_>1||!w.successor_ready_||!w.predecessor_head_size_||!w.prepare_size_||!w.successor_head_size_) {
            return {};
        }
        break;
    case context_store_registry_lab_kind::abort_record:
        if(w.variant_>1||!w.successor_ready_||!w.prepare_size_||(variant?(!w.predecessor_head_size_||w.observed_head_size_||w.observed_resolved_size_):(!w.observed_head_size_||!w.observed_resolved_size_||w.predecessor_head_size_))) {
            return {};
        }
        break;
    case context_store_registry_lab_kind::quarantine:
        if(w.variant_||!expectation_valid(kind,c,w.expectation_)||
           (w.expectation_.quarantine_has_previous_record&&digest_eq(w.expectation_.quarantine_previous_record_digest,w.expectation_.prepare_digest)&&!w.prepare_size_)||
           (w.expectation_.quarantine_has_head&&digest_eq(w.expectation_.quarantine_head_digest,w.expectation_.initial_head_digest)&&!w.predecessor_head_size_)||
           (w.expectation_.quarantine_has_head&&digest_eq(w.expectation_.quarantine_head_digest,w.expectation_.successor_head_digest)&&!w.successor_head_size_)) {
            return {};
        }
        break;
    }
    if(!output_admitted(out,cap,maximum,&encoded,&c,sizeof(c),&w,sizeof(w))) {
        return {};
    }
    return encode_private(kind,variant,c,w.expectation_,out,cap,encoded);
}

namespace {
bool same_scope_value(const context_store_registry_lab_scope_value_v1&a,const context_store_registry_lab_scope_value_v1&b){return a.root_id==b.root_id&&a.path_policy_commitment==b.path_policy_commitment&&same_id(a.registry_id,b.registry_id)&&a.registry_epoch==b.registry_epoch;}
template<size_t N>bool same_bounded(const context_store_registry_lab_bounded_bytes_v1<N>&a,const context_store_registry_lab_bounded_bytes_v1<N>&b){return a.size==b.size&&a.size<=N&&equal(a.bytes.data(),b.bytes.data(),a.size);}
bool same_root_value(const context_store_registry_lab_root_value_v1&a,const context_store_registry_lab_root_value_v1&b){return same_scope_value(a.scope,b.scope)&&a.store_uuid==b.store_uuid&&a.filesystem_uuid==b.filesystem_uuid&&a.mount_id==b.mount_id&&a.owner_uid==b.owner_uid&&a.lock_st_dev==b.lock_st_dev&&a.lock_st_ino==b.lock_st_ino&&a.state==b.state&&a.initial_head_digest==b.initial_head_digest;}
bool same_head_value(const context_store_registry_lab_head_value_v1&a,const context_store_registry_lab_head_value_v1&b){return same_scope_value(a.scope,b.scope)&&a.selection==b.selection&&same_bounded(a.selected_envelope,b.selected_envelope)&&same_bounded(a.predecessor_envelope,b.predecessor_envelope)&&a.selected_envelope_digest==b.selected_envelope_digest&&a.predecessor_envelope_digest==b.predecessor_envelope_digest&&a.expected_head_digest==b.expected_head_digest&&a.selected_high_water==b.selected_high_water&&a.selector_generation==b.selector_generation&&a.predecessor_high_water==b.predecessor_high_water&&a.predecessor_selector_generation==b.predecessor_selector_generation;}
bool same_prepare_value(const context_store_registry_lab_prepare_value_v1&a,const context_store_registry_lab_prepare_value_v1&b){return same_scope_value(a.scope,b.scope)&&a.attempt_id==b.attempt_id&&a.operation_commitment==b.operation_commitment&&a.predecessor_envelope_digest==b.predecessor_envelope_digest&&a.successor_envelope_digest==b.successor_envelope_digest&&a.initial_head_digest==b.initial_head_digest&&a.slot==b.slot&&a.predecessor_high_water==b.predecessor_high_water&&a.predecessor_selector_generation==b.predecessor_selector_generation&&a.successor_selector_generation==b.successor_selector_generation&&same_bounded(a.predecessor,b.predecessor)&&same_bounded(a.successor,b.successor);}
template<class T>bool same_terminal_value(const T&a,const T&b){return same_scope_value(a.scope,b.scope)&&a.terminal_class==b.terminal_class&&a.attempt_id==b.attempt_id&&a.operation_commitment==b.operation_commitment&&a.predecessor_envelope_digest==b.predecessor_envelope_digest&&a.successor_envelope_digest==b.successor_envelope_digest&&a.prepare_digest==b.prepare_digest&&a.head_digest==b.head_digest&&a.slot==b.slot;}
bool same_quarantine_value(const context_store_registry_lab_quarantine_value_v1&a,const context_store_registry_lab_quarantine_value_v1&b){return same_scope_value(a.scope,b.scope)&&a.event_id==b.event_id&&a.attempt_id==b.attempt_id&&a.previous_record_digest==b.previous_record_digest&&a.head_digest==b.head_digest&&a.operation_commitment==b.operation_commitment&&a.slot==b.slot&&a.reason==b.reason&&a.phase==b.phase&&a.attributable==b.attributable&&a.has_previous_record==b.has_previous_record&&a.has_head==b.has_head;}
void map_scope(const context_store_registry_lab_scope_value_v1&s,context_store_registry_lab_expectation&e){e.root_id=s.root_id;e.path_policy_commitment=s.path_policy_commitment;e.registry_id=s.registry_id;e.registry_epoch=s.registry_epoch;e.mount_id=1;e.lock_st_dev=1;e.lock_st_ino=1;}
bool map_root(const context_store_registry_lab_root_value_v1&v,context_store_registry_lab_expectation&e){if(size_t(v.state)>1||(v.state==context_store_registry_lab_root_state_v1::initializing&&nonzero(v.initial_head_digest)))return false;map_scope(v.scope,e);e.store_uuid=v.store_uuid;e.filesystem_uuid=v.filesystem_uuid;e.mount_id=v.mount_id;e.owner_uid=v.owner_uid;e.lock_st_dev=v.lock_st_dev;e.lock_st_ino=v.lock_st_ino;e.initial_head_digest=v.initial_head_digest;return true;}
bool map_head(const context_store_registry_lab_head_value_v1&v,context_store_registry_lab_expectation&e){if(size_t(v.selection)>1||!v.selected_envelope.size||v.selected_envelope.size>1024)return false;map_scope(v.scope,e);if(v.selection==context_store_registry_lab_head_selection_v1::predecessor){if(v.predecessor_envelope.size||std::any_of(v.predecessor_envelope.bytes.begin(),v.predecessor_envelope.bytes.end(),[](uint8_t b){return b!=0;})||nonzero(v.predecessor_envelope_digest)||v.predecessor_high_water||v.predecessor_selector_generation)return false;e.predecessor=v.selected_envelope.bytes.data();e.predecessor_size=v.selected_envelope.size;e.predecessor_envelope_digest=v.selected_envelope_digest;e.predecessor_high_water=v.selected_high_water;e.predecessor_selector_generation=v.selector_generation;e.initial_head_digest=v.expected_head_digest;}else{if(!v.predecessor_envelope.size||v.predecessor_envelope.size>1024)return false;e.predecessor=v.predecessor_envelope.bytes.data();e.predecessor_size=v.predecessor_envelope.size;e.predecessor_envelope_digest=v.predecessor_envelope_digest;e.predecessor_high_water=v.predecessor_high_water;e.predecessor_selector_generation=v.predecessor_selector_generation;e.successor=v.selected_envelope.bytes.data();e.successor_size=v.selected_envelope.size;e.successor_envelope_digest=v.selected_envelope_digest;e.successor_selector_generation=v.selector_generation;e.successor_head_digest=v.expected_head_digest;}return true;}
bool map_prepare(const context_store_registry_lab_prepare_value_v1&v,context_store_registry_lab_expectation&e){if(!v.predecessor.size||!v.successor.size||v.predecessor.size>1024||v.successor.size>1024)return false;map_scope(v.scope,e);e.attempt_id=v.attempt_id;e.operation_commitment=v.operation_commitment;e.predecessor_envelope_digest=v.predecessor_envelope_digest;e.successor_envelope_digest=v.successor_envelope_digest;e.initial_head_digest=v.initial_head_digest;e.slot=v.slot;e.predecessor_high_water=v.predecessor_high_water;e.predecessor_selector_generation=v.predecessor_selector_generation;e.successor_selector_generation=v.successor_selector_generation;e.predecessor=v.predecessor.bytes.data();e.predecessor_size=v.predecessor.size;e.successor=v.successor.bytes.data();e.successor_size=v.successor.size;return true;}
template<class T>bool terminal_binds_prepare(const T&v,const context_store_registry_lab_prepare_value_v1&p){return same_scope_value(v.scope,p.scope)&&v.attempt_id==p.attempt_id&&v.operation_commitment==p.operation_commitment&&v.predecessor_envelope_digest==p.predecessor_envelope_digest&&v.successor_envelope_digest==p.successor_envelope_digest&&v.slot==p.slot;}
template<class V,class W>bool typed_output_admitted(const V&v,const W&w,uint8_t*out,size_t cap,size_t&n){return !ranges_overlap(out,cap,&v,sizeof(v))&&!ranges_overlap(out,cap,&w,sizeof(w))&&!ranges_overlap(&n,sizeof(n),&v,sizeof(v))&&!ranges_overlap(&n,sizeof(n),&w,sizeof(w));}
}

bool context_store_registry_lab_root_witness::matches(const context_store_registry_lab_root_value_v1&v)const noexcept{return same_root_value(value_,v);}
bool context_store_registry_lab_head_witness::matches(const context_store_registry_lab_head_value_v1&v)const noexcept{return same_head_value(value_,v);}
bool context_store_registry_lab_prepare_witness::matches(const context_store_registry_lab_prepare_value_v1&v)const noexcept{return same_prepare_value(value_,v);}
bool context_store_registry_lab_close_witness::matches(const context_store_registry_lab_close_value_v1&v)const noexcept{return same_terminal_value(value_,v);}
bool context_store_registry_lab_abort_witness::matches(const context_store_registry_lab_abort_value_v1&v)const noexcept{return same_terminal_value(value_,v);}
bool context_store_registry_lab_quarantine_witness::matches(const context_store_registry_lab_quarantine_value_v1&v)const noexcept{return same_quarantine_value(value_,v);}

bool context_store_registry_lab_admit_root_v1(const context_store_registry_lab_root_value_v1&v,const context_store_registry_lab_credential&c,const context_store_registry_lab_root_evidence_v1&x,context_store_registry_lab_root_witness&o)noexcept{context_store_registry_lab_expectation e{};if(!map_root(v,e))return false;context_store_registry_lab_lifecycle_witness::evidence z{x.authenticated_head,x.authenticated_head_size};if(v.state==context_store_registry_lab_root_state_v1::initialized){if(!x.selected_envelope.size||x.selected_envelope.size>1024)return false;e.predecessor=x.selected_envelope.bytes.data();e.predecessor_size=x.selected_envelope.size;e.predecessor_envelope_digest=x.selected_envelope_digest;e.predecessor_high_water=x.selected_high_water;e.predecessor_selector_generation=x.selector_generation;}else if(x.authenticated_head||x.authenticated_head_size||x.selected_envelope.size||nonzero(x.selected_envelope_digest)||x.selected_high_water||x.selector_generation)return false;bool ok=v.state==context_store_registry_lab_root_state_v1::initializing?context_store_registry_lab_lifecycle_witness::admit_root_initializing(c,e,o.storage_):context_store_registry_lab_lifecycle_witness::admit_root_initialized(c,e,z,o.storage_);if(ok)o.value_=v;return ok;}
bool context_store_registry_lab_admit_head_v1(const context_store_registry_lab_head_value_v1&v,const context_store_registry_lab_credential&c,context_store_registry_lab_head_witness&o)noexcept{context_store_registry_lab_expectation e{};if(!map_head(v,e))return false;bool ok=v.selection==context_store_registry_lab_head_selection_v1::predecessor?context_store_registry_lab_lifecycle_witness::admit_head_predecessor(c,e,o.storage_):context_store_registry_lab_lifecycle_witness::admit_head_successor(c,e,o.storage_);if(ok)o.value_=v;return ok;}
bool context_store_registry_lab_admit_prepare_v1(const context_store_registry_lab_prepare_value_v1&v,const context_store_registry_lab_credential&c,const context_store_registry_lab_prepare_evidence_v1&x,context_store_registry_lab_prepare_witness&o)noexcept{context_store_registry_lab_expectation e{};if(!map_prepare(v,e))return false;std::array<uint8_t,context_store_registry_lab_wire_max_bytes> derived_bytes{};size_t derived_size=0;auto derived=encode_private(context_store_registry_lab_kind::prepare,false,c,e,derived_bytes.data(),derived_bytes.size(),derived_size,true);wipe(derived_bytes.data(),derived_bytes.size());if(!derived.authenticated())return false;e.prepare_digest=derived.content_digest;context_store_registry_lab_lifecycle_witness::evidence z{x.predecessor_head,x.predecessor_head_size};bool ok=context_store_registry_lab_lifecycle_witness::admit_prepare(c,e,z,o.storage_);if(ok)o.value_=v;return ok;}
bool context_store_registry_lab_admit_close_v1(const context_store_registry_lab_close_value_v1&v,const context_store_registry_lab_credential&c,const context_store_registry_lab_close_evidence_v1&x,context_store_registry_lab_close_witness&o)noexcept{if(size_t(v.terminal_class)>1||!terminal_binds_prepare(v,x.transition))return false;context_store_registry_lab_expectation e{};if(!map_prepare(x.transition,e))return false;e.prepare_digest=v.prepare_digest;e.successor_head_digest=v.head_digest;context_store_registry_lab_lifecycle_witness::evidence z{x.predecessor_head,x.predecessor_head_size,x.successor_head,x.successor_head_size,x.prepare,x.prepare_size};bool ok=context_store_registry_lab_lifecycle_witness::admit_close(c,e,z,uint8_t(v.terminal_class),o.storage_);if(ok)o.value_=v;return ok;}
bool context_store_registry_lab_admit_abort_v1(const context_store_registry_lab_abort_value_v1&v,const context_store_registry_lab_credential&c,const context_store_registry_lab_abort_evidence_v1&x,context_store_registry_lab_abort_witness&o)noexcept{if(size_t(v.terminal_class)>1||!terminal_binds_prepare(v,x.transition))return false;context_store_registry_lab_expectation e{};if(!map_prepare(x.transition,e))return false;e.prepare_digest=v.prepare_digest;e.initial_head_digest=v.head_digest;context_store_registry_lab_lifecycle_witness::evidence z{x.predecessor_head,x.predecessor_head_size,nullptr,0,x.prepare,x.prepare_size,x.observed_head,x.observed_head_size,x.observed_resolved,x.observed_resolved_size};bool ok=context_store_registry_lab_lifecycle_witness::admit_abort(c,e,z,uint8_t(v.terminal_class),o.storage_);if(ok)o.value_=v;return ok;}
bool context_store_registry_lab_admit_quarantine_v1(const context_store_registry_lab_quarantine_value_v1&v,const context_store_registry_lab_credential&c,const context_store_registry_lab_quarantine_evidence_v1&x,context_store_registry_lab_quarantine_witness&o)noexcept{if((!v.attributable&&(nonzero(v.attempt_id)||v.slot||nonzero(v.operation_commitment)))||(!v.has_previous_record&&nonzero(v.previous_record_digest))||(!v.has_head&&nonzero(v.head_digest)))return false;context_store_registry_lab_expectation e{};if(x.transition){if(!map_prepare(*x.transition,e))return false;}else map_scope(v.scope,e);if(x.prepare&&!hash_domain(content_domains[size_t(context_store_registry_lab_kind::prepare)],x.prepare,x.prepare_size,e.prepare_digest))return false;if(x.successor_head)e.successor_head_digest=v.head_digest;else if(x.predecessor_head&&v.has_head)e.initial_head_digest=v.head_digest;e.quarantine_event_id=v.event_id;e.attempt_id=v.attempt_id;e.quarantine_previous_record_digest=v.previous_record_digest;e.quarantine_head_digest=v.head_digest;e.operation_commitment=v.operation_commitment;e.slot=v.slot;e.quarantine_reason=v.reason;e.quarantine_phase=v.phase;e.quarantine_attributable=v.attributable;e.quarantine_has_previous_record=v.has_previous_record;e.quarantine_has_head=v.has_head;context_store_registry_lab_lifecycle_witness::evidence z{x.predecessor_head,x.predecessor_head_size,x.successor_head,x.successor_head_size,x.prepare,x.prepare_size};bool ok=context_store_registry_lab_lifecycle_witness::admit_quarantine(c,e,z,o.storage_);if(ok)o.value_=v;return ok;}

context_store_registry_lab_wire_result context_store_registry_lab_encode_root_v1(const context_store_registry_lab_root_value_v1&v,const context_store_registry_lab_credential&c,const context_store_registry_lab_root_witness&w,uint8_t*out,size_t cap,size_t&n)noexcept{if(!w.matches(v)||!typed_output_admitted(v,w,out,cap,n))return {};return context_store_registry_lab_lifecycle_witness::encode(context_store_registry_lab_kind::root,c,w.storage_,out,cap,n);}
context_store_registry_lab_wire_result context_store_registry_lab_encode_head_v1(const context_store_registry_lab_head_value_v1&v,const context_store_registry_lab_credential&c,const context_store_registry_lab_head_witness&w,uint8_t*out,size_t cap,size_t&n)noexcept{if(!w.matches(v)||!typed_output_admitted(v,w,out,cap,n))return {};return context_store_registry_lab_lifecycle_witness::encode(context_store_registry_lab_kind::head,c,w.storage_,out,cap,n);}
context_store_registry_lab_wire_result context_store_registry_lab_encode_prepare_v1(const context_store_registry_lab_prepare_value_v1&v,const context_store_registry_lab_credential&c,const context_store_registry_lab_prepare_witness&w,uint8_t*out,size_t cap,size_t&n)noexcept{if(!w.matches(v)||!typed_output_admitted(v,w,out,cap,n))return {};return context_store_registry_lab_lifecycle_witness::encode(context_store_registry_lab_kind::prepare,c,w.storage_,out,cap,n);}
context_store_registry_lab_wire_result context_store_registry_lab_encode_close_v1(const context_store_registry_lab_close_value_v1&v,const context_store_registry_lab_credential&c,const context_store_registry_lab_close_witness&w,uint8_t*out,size_t cap,size_t&n)noexcept{if(!w.matches(v)||!typed_output_admitted(v,w,out,cap,n))return {};return context_store_registry_lab_lifecycle_witness::encode(context_store_registry_lab_kind::close,c,w.storage_,out,cap,n);}
context_store_registry_lab_wire_result context_store_registry_lab_encode_abort_v1(const context_store_registry_lab_abort_value_v1&v,const context_store_registry_lab_credential&c,const context_store_registry_lab_abort_witness&w,uint8_t*out,size_t cap,size_t&n)noexcept{if(!w.matches(v)||!typed_output_admitted(v,w,out,cap,n))return {};return context_store_registry_lab_lifecycle_witness::encode(context_store_registry_lab_kind::abort_record,c,w.storage_,out,cap,n);}
context_store_registry_lab_wire_result context_store_registry_lab_encode_quarantine_v1(const context_store_registry_lab_quarantine_value_v1&v,const context_store_registry_lab_credential&c,const context_store_registry_lab_quarantine_witness&w,uint8_t*out,size_t cap,size_t&n)noexcept{if(!w.matches(v)||!typed_output_admitted(v,w,out,cap,n))return {};return context_store_registry_lab_lifecycle_witness::encode(context_store_registry_lab_kind::quarantine,c,w.storage_,out,cap,n);}

bool context_store_registry_lab_parse_credential_v1(const uint8_t*d,size_t n,context_store_registry_lab_credential&o)noexcept{
    o.clear();
    if(!d||n<60||n>context_store_registry_lab_credential_max_bytes||std::memcmp(d,credential_magic,16)!=0)return false;
    size_t p=16;uint16_t z=uint16_t(d[p]<<8|d[p+1]);p+=2;
    if(!z||z>128||p+z+8+2+32!=n)return false;
    context_store_registry_lab_credential parsed;
    parsed.key_id.size=uint8_t(z);std::copy_n(d+p,z,parsed.key_id.bytes.begin());p+=z;
    for(size_t i=0;i<8;++i)parsed.generation=(parsed.generation<<8)|d[p++];
    uint16_t secret=uint16_t(d[p]<<8|d[p+1]);p+=2;
    if(secret!=32||!parsed.generation||!valid_id(parsed.key_id))return false;
    std::copy_n(d+p,32,parsed.secret.begin());
    if(!nonzero(parsed.secret))return false;
    o=std::move(parsed);
    return true;
}

bool context_store_registry_lab_path_policy_v1(const uint8_t*parent,size_t pn,const uint8_t*root,size_t rn,const std::array<uint8_t,16>&fs,const std::array<uint8_t,16>&sub,uint64_t mount,uint64_t dev,uint64_t uid,context_store_format_digest&out)noexcept{auto valid=[](const uint8_t*p,size_t n){if(!p||!n||n>4096||p[0]!='/')return false;for(size_t i=0;i<n;++i)if(p[i]<0x21||p[i]>0x7e)return false;return true;};if(!valid(parent,pn)||!valid(root,rn)||!mount||!dev)return false;std::array<uint8_t,8400>b{};writer w{b.data(),b.size()};bool ok=w.raw(path_domain,sizeof(path_domain))&&w.be(pn,8)&&w.raw(parent,pn)&&w.be(rn,8)&&w.raw(root,rn)&&w.raw(fs.data(),16)&&w.raw(sub.data(),16)&&w.be(mount,8)&&w.be(dev,8)&&w.be(uid,8)&&w.be(448,4)&&w.be(384,4)&&w.be(512,8)&&w.be(16777216,8)&&w.be(1073741824,8)&&w.be(68719476736ULL,8)&&w.be(268435456,8)&&context_store_sha256_bounded(b.data(),w.n,b.size(),out);wipe(b.data(),b.size());return ok;}

static context_store_registry_lab_wire_result verify_authenticated_structure_v1(context_store_registry_lab_kind k,const uint8_t*d,size_t n,const context_store_registry_lab_credential&c,const context_store_registry_lab_expectation&e)noexcept{context_store_registry_lab_wire_result out;size_t max=k==context_store_registry_lab_kind::prepare?4096:1024;if(!d||!n||n>max||size_t(k)>5)return out;context_store_format_digest dk{};if(!derive(c,dk)){out.status=context_store_registry_lab_wire_status::invalid_credential;return out;}if(!expectation_valid(k,c,e)){wipe(dk.data(),32);out.status=context_store_registry_lab_wire_status::semantic_rejection;return out;}reader r{d,n};uint64_t count=0;if(!r.head(5,count)||count!=2||!r.exact(0)){wipe(dk.data(),32);return out;}size_t auth_off=r.off;if(!r.head(5,count)||count!=4||!r.exact(0)){wipe(dk.data(),32);return out;}body_view b;if(!parse_body(r,k,b)||!r.exact(1)){wipe(dk.data(),32);return out;}context_store_registered_id id;uint64_t alg=0,gen=0;if(!r.id(id)||!r.exact(2)||!r.u64(alg)||!r.exact(3)||!r.u64(gen)){wipe(dk.data(),32);return out;}size_t auth_size=r.off-auth_off;const uint8_t*tagp;size_t tagz;if(!r.exact(1)||!r.bytes(2,tagp,tagz)||tagz!=32||r.off!=n){wipe(dk.data(),32);return out;}if(!same_id(id,c.key_id)||alg!=1||gen!=c.generation){wipe(dk.data(),32);out.status=context_store_registry_lab_wire_status::semantic_rejection;return out;}std::array<uint8_t,4160>msg{};writer w{msg.data(),msg.size()};context_store_format_digest tag{};bool ok=w.raw(auth_domains[size_t(k)],std::strlen(auth_domains[size_t(k)])+1)&&w.raw(d+auth_off,auth_size)&&context_store_hmac_sha256(dk.data(),dk.size(),msg.data(),w.n,tag);wipe(dk.data(),32);wipe(msg.data(),msg.size());if(!ok||!equal(tag.data(),tagp,32)){wipe(tag.data(),32);out.status=context_store_registry_lab_wire_status::authentication_failed;return out;}wipe(tag.data(),32);if(!semantics(k,b,c,e)){out.status=context_store_registry_lab_wire_status::semantic_rejection;return out;}if(!hash_domain(content_domains[size_t(k)],d,n,out.content_digest)){out.status=context_store_registry_lab_wire_status::semantic_rejection;return out;}out.status=context_store_registry_lab_wire_status::authenticated_semantic_only;return out;}

context_store_registry_lab_wire_result context_store_registry_lab_verify_v1(
        context_store_registry_lab_kind kind,const uint8_t*data,size_t size,
        const context_store_registry_lab_credential&credential,
        const context_store_registry_lab_expectation&expectation) noexcept {
    auto result=verify_authenticated_structure_v1(kind,data,size,credential,expectation);
    if(result.authenticated()&&kind==context_store_registry_lab_kind::prepare&&
       !digest_eq(result.content_digest,expectation.prepare_digest)) {
        result.status=context_store_registry_lab_wire_status::semantic_rejection;
        result.content_digest={};
    }
    return result;
}

const char*context_store_registry_lab_wire_status_name(context_store_registry_lab_wire_status s)noexcept{switch(s){case context_store_registry_lab_wire_status::authenticated_semantic_only:return "authenticated-semantic-only";case context_store_registry_lab_wire_status::structural_rejection:return "structural-rejection";case context_store_registry_lab_wire_status::invalid_credential:return "invalid-credential";case context_store_registry_lab_wire_status::authentication_failed:return "authentication-failed";case context_store_registry_lab_wire_status::semantic_rejection:return "semantic-rejection";}return "unknown";}
} // namespace halofpx
