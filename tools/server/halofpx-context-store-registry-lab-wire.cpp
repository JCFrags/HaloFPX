#include "halofpx-context-store-registry-lab-wire.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cstring>
#include <utility>

namespace halofpx { namespace {

constexpr char credential_magic[16] = "HaloFPXRegKey01";
constexpr char key_domain[] = "halofpx.registry-lab-key.v1";
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

bool expected_transition(const context_store_registry_lab_credential&c,const context_store_registry_lab_expectation&e){
    if(!credential_valid(c)||!root_expectation(e)||e.predecessor_high_water==UINT64_MAX||
       !e.predecessor_selector_generation||
       e.predecessor_selector_generation==UINT64_MAX||
       e.successor_selector_generation!=e.predecessor_selector_generation+1||
       !nonzero(e.initial_head_digest)||!nonzero(e.successor_head_digest)||
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

context_store_registry_lab_wire_result context_store_registry_lab_verify_v1(context_store_registry_lab_kind k,const uint8_t*d,size_t n,const context_store_registry_lab_credential&c,const context_store_registry_lab_expectation&e)noexcept{context_store_registry_lab_wire_result out;size_t max=k==context_store_registry_lab_kind::prepare?4096:1024;if(!d||!n||n>max||size_t(k)>5)return out;context_store_format_digest dk{};if(!derive(c,dk)){out.status=context_store_registry_lab_wire_status::invalid_credential;return out;}if(!expectation_valid(k,c,e)){wipe(dk.data(),32);out.status=context_store_registry_lab_wire_status::semantic_rejection;return out;}reader r{d,n};uint64_t count=0;if(!r.head(5,count)||count!=2||!r.exact(0)){wipe(dk.data(),32);return out;}size_t auth_off=r.off;if(!r.head(5,count)||count!=4||!r.exact(0)){wipe(dk.data(),32);return out;}body_view b;if(!parse_body(r,k,b)||!r.exact(1)){wipe(dk.data(),32);return out;}context_store_registered_id id;uint64_t alg=0,gen=0;if(!r.id(id)||!r.exact(2)||!r.u64(alg)||!r.exact(3)||!r.u64(gen)){wipe(dk.data(),32);return out;}size_t auth_size=r.off-auth_off;const uint8_t*tagp;size_t tagz;if(!r.exact(1)||!r.bytes(2,tagp,tagz)||tagz!=32||r.off!=n){wipe(dk.data(),32);return out;}if(!same_id(id,c.key_id)||alg!=1||gen!=c.generation){wipe(dk.data(),32);out.status=context_store_registry_lab_wire_status::semantic_rejection;return out;}std::array<uint8_t,4160>msg{};writer w{msg.data(),msg.size()};context_store_format_digest tag{};bool ok=w.raw(auth_domains[size_t(k)],std::strlen(auth_domains[size_t(k)])+1)&&w.raw(d+auth_off,auth_size)&&context_store_hmac_sha256(dk.data(),dk.size(),msg.data(),w.n,tag);wipe(dk.data(),32);wipe(msg.data(),msg.size());if(!ok||!equal(tag.data(),tagp,32)){wipe(tag.data(),32);out.status=context_store_registry_lab_wire_status::authentication_failed;return out;}wipe(tag.data(),32);if(!semantics(k,b,c,e)){out.status=context_store_registry_lab_wire_status::semantic_rejection;return out;}if(!hash_domain(content_domains[size_t(k)],d,n,out.content_digest)){out.status=context_store_registry_lab_wire_status::semantic_rejection;return out;}out.status=context_store_registry_lab_wire_status::authenticated_semantic_only;return out;}

const char*context_store_registry_lab_wire_status_name(context_store_registry_lab_wire_status s)noexcept{switch(s){case context_store_registry_lab_wire_status::authenticated_semantic_only:return "authenticated-semantic-only";case context_store_registry_lab_wire_status::structural_rejection:return "structural-rejection";case context_store_registry_lab_wire_status::invalid_credential:return "invalid-credential";case context_store_registry_lab_wire_status::authentication_failed:return "authentication-failed";case context_store_registry_lab_wire_status::semantic_rejection:return "semantic-rejection";}return "unknown";}
} // namespace halofpx
