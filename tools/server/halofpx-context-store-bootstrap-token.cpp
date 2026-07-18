#include "halofpx-context-store-bootstrap-token.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cstring>

namespace halofpx {
namespace {

constexpr char key_domain[] = "halofpx.bootstrap-token-key.v1";
constexpr char auth_domain[] = "halofpx.bootstrap-token-auth.v1";
constexpr char digest_domain[] = "halofpx.bootstrap-token.v1";
constexpr uint64_t format_major = 1;
constexpr uint64_t format_minor = 0;
constexpr uint64_t operation_bootstrap = 0;
constexpr uint64_t algorithm_hmac_sha256 = 1;

struct writer {
    uint8_t * data = nullptr;
    size_t capacity = 0;
    size_t size = 0;
    bool raw(const void * value, size_t count) noexcept {
        if ((value == nullptr && count != 0) || data == nullptr || count > capacity - size) return false;
        std::memcpy(data + size, value, count); size += count; return true;
    }
    bool head(uint8_t major, uint64_t value) noexcept {
        if (value < 24) { const uint8_t b = static_cast<uint8_t>((major << 5) | value); return raw(&b, 1); }
        if (value <= UINT8_MAX) { const uint8_t b[2] = { static_cast<uint8_t>((major << 5) | 24), static_cast<uint8_t>(value) }; return raw(b, 2); }
        if (value <= UINT16_MAX) { const uint8_t b[3] = { static_cast<uint8_t>((major << 5) | 25), static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value) }; return raw(b, 3); }
        if (value <= UINT32_MAX) { uint8_t b[5] = { static_cast<uint8_t>((major << 5) | 26) }; for (size_t i=0;i<4;++i)b[4-i]=static_cast<uint8_t>(value>>(8*i)); return raw(b,5); }
        uint8_t b[9] = { static_cast<uint8_t>((major << 5) | 27) }; for (size_t i=0;i<8;++i)b[8-i]=static_cast<uint8_t>(value>>(8*i)); return raw(b,9);
    }
    bool u64(uint64_t v) noexcept { return head(0,v); }
    bool bstr(const void * p,size_t n) noexcept { return head(2,n)&&raw(p,n); }
    bool tstr(const void * p,size_t n) noexcept { return head(3,n)&&raw(p,n); }
};

struct reader {
    const uint8_t * data=nullptr; size_t size=0; size_t offset=0;
    bool head(uint8_t major,uint64_t & value) noexcept {
        if(offset>=size)return false; const uint8_t first=data[offset++]; if((first>>5)!=major)return false;
        const uint8_t add=first&31; if(add<24){value=add;return true;} size_t n=0;
        if(add==24)n=1;else if(add==25)n=2;else if(add==26)n=4;else if(add==27)n=8;else return false;
        if(n>size-offset)return false; value=0;for(size_t i=0;i<n;++i)value=(value<<8)|data[offset++];
        return !((n==1&&value<24)||(n==2&&value<=UINT8_MAX)||(n==4&&value<=UINT16_MAX)||(n==8&&value<=UINT32_MAX));
    }
    bool exact_u64(uint64_t expected) noexcept { uint64_t v=0;return head(0,v)&&v==expected; }
    bool u64(uint64_t & v) noexcept { return head(0,v); }
    bool bstr(const uint8_t *& p,size_t & n) noexcept { uint64_t length=0;if(!head(2,length)||length>size-offset)return false;p=data+offset;n=static_cast<size_t>(length);offset+=n;return true; }
    bool tstr(const uint8_t *& p,size_t & n) noexcept { uint64_t length=0;if(!head(3,length)||length>size-offset)return false;p=data+offset;n=static_cast<size_t>(length);offset+=n;return true; }
};

void wipe(void * p,size_t n) noexcept { volatile uint8_t * b=static_cast<volatile uint8_t *>(p);while(n--)*b++=0; }

template<size_t N> bool nonzero(const std::array<uint8_t,N>& v) noexcept { uint8_t x=0;for(auto b:v)x|=b;return x!=0; }
bool valid_id(const context_store_registered_id & id) noexcept { if(id.size==0||id.size>context_store_registered_id_max_bytes)return false;for(size_t i=0;i<id.size;++i){const uint8_t b=id.bytes[i];if(b==0||b>0x7f)return false;}return true; }
bool same_id(const context_store_registered_id&a,const context_store_registered_id&b) noexcept { if(a.size!=b.size)return false;volatile uint8_t x=0;for(size_t i=0;i<a.size;++i)x=static_cast<uint8_t>(x|(a.bytes[i]^b.bytes[i]));return x==0; }
bool same_digest(const context_store_format_digest&a,const context_store_format_digest&b) noexcept { volatile uint8_t x=0;for(size_t i=0;i<a.size();++i)x=static_cast<uint8_t>(x|(a[i]^b[i]));return x==0; }

bool valid_body(const context_store_bootstrap_token_body & b) noexcept {
    return nonzero(b.store_uuid)&&nonzero(b.namespace_id)&&b.policy_epoch!=0&&nonzero(b.checkpoint_lineage_id)&&
        valid_id(b.manifest_key_id)&&b.manifest_key_generation!=0&&nonzero(b.compatibility_root)&&b.authority_epoch!=0&&
        valid_id(b.anchor_key_id)&&b.anchor_key_generation!=0&&nonzero(b.selected_manifest_digest)&&
        nonzero(b.authority_scope_commitment)&&valid_id(b.protected_registry_id)&&b.protected_registry_epoch!=0&&
        nonzero(b.protected_registry_snapshot_digest)&&nonzero(b.protected_registry_policy_digest)&&
        b.authorization_sequence!=0&&nonzero(b.command_id);
}

bool encode_body(const context_store_bootstrap_token_body & b,uint8_t * out,size_t cap,size_t & used) noexcept {
    writer w{out,cap};
    bool ok=w.head(5,23);
    auto key=[&](uint64_t k){return w.u64(k);};
    ok=ok&&key(0)&&w.u64(format_major)&&key(1)&&w.u64(format_minor)&&key(2)&&w.u64(operation_bootstrap)&&
       key(3)&&w.bstr(b.store_uuid.data(),b.store_uuid.size())&&key(4)&&w.bstr(b.namespace_id.data(),b.namespace_id.size())&&
       key(5)&&w.u64(b.policy_epoch)&&key(6)&&w.bstr(b.checkpoint_lineage_id.data(),b.checkpoint_lineage_id.size())&&
       key(7)&&w.tstr(b.manifest_key_id.bytes.data(),b.manifest_key_id.size)&&key(8)&&w.u64(b.manifest_key_generation)&&
       key(9)&&w.bstr(b.compatibility_root.data(),b.compatibility_root.size())&&key(10)&&w.u64(b.authority_epoch)&&
       key(11)&&w.tstr(b.anchor_key_id.bytes.data(),b.anchor_key_id.size)&&key(12)&&w.u64(b.anchor_key_generation)&&
       key(13)&&w.u64(1)&&key(14)&&w.raw("\xf6",1)&&
       key(15)&&w.bstr(b.selected_manifest_digest.data(),b.selected_manifest_digest.size())&&
       key(16)&&w.bstr(b.authority_scope_commitment.data(),b.authority_scope_commitment.size())&&
       key(17)&&w.tstr(b.protected_registry_id.bytes.data(),b.protected_registry_id.size)&&
       key(18)&&w.u64(b.protected_registry_epoch)&&key(19)&&w.bstr(b.protected_registry_snapshot_digest.data(),b.protected_registry_snapshot_digest.size())&&
       key(20)&&w.bstr(b.protected_registry_policy_digest.data(),b.protected_registry_policy_digest.size())&&
       key(21)&&w.u64(b.authorization_sequence)&&key(22)&&w.bstr(b.command_id.data(),b.command_id.size());
    used=w.size;return ok;
}

bool parse_id(reader&r,context_store_registered_id&out) noexcept {const uint8_t*p;size_t n;if(!r.tstr(p,n)||n==0||n>context_store_registered_id_max_bytes)return false;out.size=static_cast<uint8_t>(n);std::copy_n(p,n,out.bytes.begin());return valid_id(out);}
template<size_t N> bool parse_array(reader&r,std::array<uint8_t,N>&out) noexcept {const uint8_t*p;size_t n;if(!r.bstr(p,n)||n!=N)return false;std::copy_n(p,n,out.begin());return true;}
bool parse_body(reader & r,context_store_bootstrap_token_body&b) noexcept {
    uint64_t count=0;if(!r.head(5,count)||count!=23)return false;
    auto key=[&](uint64_t k){return r.exact_u64(k);}; uint64_t target=0;
    bool ok=key(0)&&r.exact_u64(format_major)&&key(1)&&r.exact_u64(format_minor)&&key(2)&&r.exact_u64(operation_bootstrap)&&
      key(3)&&parse_array(r,b.store_uuid)&&key(4)&&parse_array(r,b.namespace_id)&&key(5)&&r.u64(b.policy_epoch)&&
      key(6)&&parse_array(r,b.checkpoint_lineage_id)&&key(7)&&parse_id(r,b.manifest_key_id)&&key(8)&&r.u64(b.manifest_key_generation)&&
      key(9)&&parse_array(r,b.compatibility_root)&&key(10)&&r.u64(b.authority_epoch)&&key(11)&&parse_id(r,b.anchor_key_id)&&
      key(12)&&r.u64(b.anchor_key_generation)&&key(13)&&r.u64(target)&&target==1&&key(14)&&r.offset<r.size&&r.data[r.offset++]==0xf6&&
      key(15)&&parse_array(r,b.selected_manifest_digest)&&key(16)&&parse_array(r,b.authority_scope_commitment)&&
      key(17)&&parse_id(r,b.protected_registry_id)&&key(18)&&r.u64(b.protected_registry_epoch)&&
      key(19)&&parse_array(r,b.protected_registry_snapshot_digest)&&key(20)&&parse_array(r,b.protected_registry_policy_digest)&&
      key(21)&&r.u64(b.authorization_sequence)&&key(22)&&parse_array(r,b.command_id);
    return ok&&valid_body(b);
}

bool key_valid(const context_store_bootstrap_admin_key_record&k) noexcept {return valid_id(k.key_id)&&k.generation!=0&&k.master_key.data&&k.master_key.size&&k.master_key.size<=context_store_master_key_max_bytes;}
bool derive_key(const context_store_bootstrap_admin_key_record&k,context_store_format_digest&derived) noexcept {
    std::array<uint8_t,256> message{};writer w{message.data(),message.size()};
    const bool ok=w.raw(key_domain,sizeof(key_domain))&&w.tstr(k.key_id.bytes.data(),k.key_id.size)&&w.u64(k.generation)&&
        context_store_hmac_sha256(k.master_key.data,k.master_key.size,message.data(),w.size,derived);wipe(message.data(),message.size());return ok;
}
bool make_tag(const context_store_bootstrap_admin_key_record&k,const uint8_t*auth_input,size_t auth_input_size,context_store_format_digest&tag) noexcept {
    std::array<uint8_t,context_store_bootstrap_token_max_bytes> msg{};writer w{msg.data(),msg.size()};context_store_format_digest derived{};
    const bool ok=derive_key(k,derived)&&w.raw(auth_domain,sizeof(auth_domain))&&w.raw(auth_input,auth_input_size)&&
        context_store_hmac_sha256(derived.data(),derived.size(),msg.data(),w.size,tag);wipe(derived.data(),derived.size());wipe(msg.data(),msg.size());return ok;
}
bool envelope_digest(const uint8_t*data,size_t size,context_store_format_digest&digest) noexcept {std::array<uint8_t,context_store_bootstrap_token_max_bytes+64> msg{};writer w{msg.data(),msg.size()};const bool ok=w.raw(digest_domain,sizeof(digest_domain))&&w.raw(data,size)&&context_store_sha256_bounded(msg.data(),w.size,msg.size(),digest);wipe(msg.data(),msg.size());return ok;}

}

context_store_bootstrap_token_result context_store_encode_bootstrap_token_v1(const context_store_bootstrap_token_body&body,const context_store_bootstrap_admin_key_record&key,uint8_t*out,size_t capacity) noexcept {
    context_store_bootstrap_token_result result;if(!valid_body(body)||!key_valid(key)||key.disposition!=context_store_key_disposition::active||!out){result.status=context_store_bootstrap_token_status::invalid_policy;return result;}
    std::array<uint8_t,1024> encoded_body{};std::array<uint8_t,1536> auth_input{};size_t body_size=0;context_store_format_digest tag{},digest{};
    writer auth{auth_input.data(),auth_input.size()};
    const bool auth_encoded=encode_body(body,encoded_body.data(),encoded_body.size(),body_size)&&auth.head(5,4)&&
        auth.u64(0)&&auth.raw(encoded_body.data(),body_size)&&auth.u64(1)&&auth.tstr(key.key_id.bytes.data(),key.key_id.size)&&
        auth.u64(2)&&auth.u64(algorithm_hmac_sha256)&&auth.u64(3)&&auth.u64(key.generation);
    if(!auth_encoded||!make_tag(key,auth_input.data(),auth.size,tag)){wipe(encoded_body.data(),encoded_body.size());wipe(auth_input.data(),auth_input.size());result.status=context_store_bootstrap_token_status::invalid_policy;return result;}
    writer w{out,std::min(capacity,context_store_bootstrap_token_max_bytes)};const bool encoded=w.head(5,2)&&w.u64(0)&&w.raw(auth_input.data(),auth.size)&&w.u64(1)&&w.bstr(tag.data(),tag.size());const bool ok=encoded&&envelope_digest(out,w.size,digest);
    wipe(encoded_body.data(),encoded_body.size());wipe(auth_input.data(),auth_input.size());wipe(tag.data(),tag.size());if(!ok){wipe(digest.data(),digest.size());result.status=encoded?context_store_bootstrap_token_status::invalid_policy:context_store_bootstrap_token_status::output_too_small;return result;}result.encoded_size=w.size;result.set_authenticated(body,key,digest);wipe(digest.data(),digest.size());return result;
}

context_store_bootstrap_token_result context_store_verify_bootstrap_token_v1(const uint8_t*data,size_t size,const context_store_bootstrap_admin_key_record&key) noexcept {
    context_store_bootstrap_token_result result;if(!data||size==0||size>context_store_bootstrap_token_max_bytes)return result;
    if(!key_valid(key)){result.status=context_store_bootstrap_token_status::invalid_policy;return result;}
    reader r{data,size};uint64_t count=0,generation=0,algorithm=0;const uint8_t*id_data=nullptr,*tag_data=nullptr;size_t id_size=0,tag_size=0;
    if(!r.head(5,count)||count!=2||!r.exact_u64(0))return result;const size_t auth_offset=r.offset;
    if(!r.head(5,count)||count!=4||!r.exact_u64(0))return result;context_store_bootstrap_token_body body;if(!parse_body(r,body))return result;
    if(!r.exact_u64(1)||!r.tstr(id_data,id_size)||!r.exact_u64(2)||!r.u64(algorithm)||!r.exact_u64(3)||!r.u64(generation)||algorithm!=algorithm_hmac_sha256)return result;
    const size_t auth_size=r.offset-auth_offset;
    if(!r.exact_u64(1)||!r.bstr(tag_data,tag_size)||r.offset!=size||tag_size!=32)return result;
    context_store_registered_id encoded_id;if(id_size==0||id_size>context_store_registered_id_max_bytes)return result;encoded_id.size=static_cast<uint8_t>(id_size);std::copy_n(id_data,id_size,encoded_id.bytes.begin());if(!valid_id(encoded_id))return result;
    if(!same_id(encoded_id,key.key_id)){result.status=context_store_bootstrap_token_status::unknown_key;return result;}if(key.disposition==context_store_key_disposition::revoked){result.status=context_store_bootstrap_token_status::revoked_key;return result;}if(key.disposition==context_store_key_disposition::read_disabled){result.status=context_store_bootstrap_token_status::read_disabled_key;return result;}if(key.disposition!=context_store_key_disposition::active){result.status=context_store_bootstrap_token_status::invalid_policy;return result;}if(generation!=key.generation){result.status=context_store_bootstrap_token_status::key_generation_mismatch;return result;}
    context_store_format_digest expected{},provided{},digest{};std::copy_n(tag_data,provided.size(),provided.begin());
    if(!make_tag(key,data+auth_offset,auth_size,expected)||!same_digest(expected,provided)){wipe(expected.data(),expected.size());wipe(provided.data(),provided.size());result.status=context_store_bootstrap_token_status::authentication_failed;return result;}wipe(expected.data(),expected.size());wipe(provided.data(),provided.size());if(!envelope_digest(data,size,digest)){result.status=context_store_bootstrap_token_status::invalid_policy;return result;}result.encoded_size=size;result.set_authenticated(body,key,digest);wipe(digest.data(),digest.size());return result;
}

const char*context_store_bootstrap_token_status_name(context_store_bootstrap_token_status s) noexcept {switch(s){case context_store_bootstrap_token_status::authenticated_unconsumed:return "authenticated-unconsumed";case context_store_bootstrap_token_status::structural_rejection:return "structural-rejection";case context_store_bootstrap_token_status::output_too_small:return "output-too-small";case context_store_bootstrap_token_status::invalid_policy:return "invalid-policy";case context_store_bootstrap_token_status::unknown_key:return "unknown-key";case context_store_bootstrap_token_status::revoked_key:return "revoked-key";case context_store_bootstrap_token_status::read_disabled_key:return "read-disabled-key";case context_store_bootstrap_token_status::key_generation_mismatch:return "key-generation-mismatch";case context_store_bootstrap_token_status::authentication_failed:return "authentication-failed";}return "unknown";}
} // namespace halofpx
