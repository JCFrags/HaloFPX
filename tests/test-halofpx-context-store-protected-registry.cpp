#include "halofpx-context-store-protected-registry.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
#include <array>
#include <cassert>
#include <climits>
#include <string>
#include <type_traits>
#include <vector>

namespace {
using namespace halofpx;
context_store_registered_id id(const std::string&s){context_store_registered_id x;x.size=uint8_t(s.size());std::copy(s.begin(),s.end(),x.bytes.begin());return x;}
struct fixture{std::array<uint8_t,32>secret{};context_store_protected_registry_key_record key;fixture(){secret.fill(0x44);key={context_store_key_disposition::active,id("registry-auth-v1"),13,{secret.data(),secret.size()}};}};
context_store_protected_registry_body body(uint64_t high=40){context_store_protected_registry_body b;b.registry_id=id("registry-v1");b.registry_epoch=9;b.authority_base_scope_commitment.fill(0xaa);b.policy_commitment.fill(0xbb);b.last_consumed_sequence=high;return b;}
std::array<uint8_t,32> hex(const std::string&s){std::array<uint8_t,32>x{};for(size_t i=0;i<x.size();++i)x[i]=uint8_t(std::stoul(s.substr(i*2,2),nullptr,16));return x;}
void retag(std::array<uint8_t,context_store_protected_registry_max_bytes>&bytes,size_t size,const fixture&f){
    constexpr char key_domain[]="halofpx.registry-snapshot-key.v1",auth_domain[]="halofpx.registry-snapshot-auth.v1";
    std::vector<uint8_t> kdf(reinterpret_cast<const uint8_t*>(key_domain),reinterpret_cast<const uint8_t*>(key_domain)+sizeof(key_domain));
    kdf.push_back(uint8_t(0x60|f.key.key_id.size));kdf.insert(kdf.end(),f.key.key_id.bytes.begin(),f.key.key_id.bytes.begin()+f.key.key_id.size);kdf.push_back(uint8_t(f.key.generation));
    context_store_format_digest derived{},tag{};assert(context_store_hmac_sha256(f.secret.data(),f.secret.size(),kdf.data(),kdf.size(),derived));
    std::vector<uint8_t> message(reinterpret_cast<const uint8_t*>(auth_domain),reinterpret_cast<const uint8_t*>(auth_domain)+sizeof(auth_domain));
    message.insert(message.end(),bytes.begin()+2,bytes.begin()+size-35);assert(context_store_hmac_sha256(derived.data(),derived.size(),message.data(),message.size(),tag));
    std::copy(tag.begin(),tag.end(),bytes.begin()+size-32);
}
}
int main(){
    static_assert(context_store_protected_registry_max_bytes==1024);
    static_assert(!std::is_aggregate_v<context_store_authenticated_protected_registry>);
    context_store_protected_registry_result forged;forged.status=context_store_protected_registry_status::authenticated_unadmitted;assert(forged.authenticated_carrier()==nullptr);
    fixture f;std::array<uint8_t,context_store_protected_registry_max_bytes>bytes{};
    auto encoded=context_store_encode_protected_registry_v1(body(),f.key,bytes.data(),bytes.size());
    assert(encoded.status==context_store_protected_registry_status::authenticated_unadmitted&&encoded.encoded_size==156);
    auto verified=context_store_verify_protected_registry_v1(bytes.data(),encoded.encoded_size,f.key);auto*c=verified.authenticated_carrier();
    assert(c&&c->body()&&c->body()->last_consumed_sequence==40&&c->envelope_digest()&&c->authority_binding());
    assert(*c->envelope_digest()==hex("a7b731bccfdea83a4595d5257ffa34ef9248bb61499b40a37874895cff6bc1ec"));
    assert(*c->authority_binding()==hex("f88b0080d50222b31e66879ecd4c14789279b9d15b786f6b22f32240d2ea5f7a"));
    const auto owned_body=*c->body();const auto owned_digest=*c->envelope_digest();const auto owned_binding=*c->authority_binding();
    auto exact=bytes;bytes.fill(0);f.secret.fill(0);assert(c->body()->registry_id.size==owned_body.registry_id.size&&std::equal(c->body()->registry_id.bytes.begin(),c->body()->registry_id.bytes.begin()+c->body()->registry_id.size,owned_body.registry_id.bytes.begin())&&*c->envelope_digest()==owned_digest&&*c->authority_binding()==owned_binding);
    f=fixture{};bytes=exact;
    assert(context_store_encode_protected_registry_v1(body(),f.key,bytes.data(),1).status==context_store_protected_registry_status::output_too_small);
    for(size_t n=0;n<encoded.encoded_size;++n)assert(context_store_verify_protected_registry_v1(bytes.data(),n,f.key).status!=context_store_protected_registry_status::authenticated_unadmitted);
    for(size_t i=0;i<encoded.encoded_size;++i){auto corrupt=bytes;corrupt[i]^=1;assert(context_store_verify_protected_registry_v1(corrupt.data(),encoded.encoded_size,f.key).status!=context_store_protected_registry_status::authenticated_unadmitted);}
    auto extra=bytes;extra[encoded.encoded_size]=0;assert(context_store_verify_protected_registry_v1(extra.data(),encoded.encoded_size+1,f.key).status==context_store_protected_registry_status::structural_rejection);
    auto malformed=bytes;malformed[0]=0xa3;assert(context_store_verify_protected_registry_v1(malformed.data(),encoded.encoded_size,f.key).status==context_store_protected_registry_status::structural_rejection);
    malformed=bytes;malformed[0]=0xa5;assert(context_store_verify_protected_registry_v1(malformed.data(),encoded.encoded_size,f.key).status==context_store_protected_registry_status::structural_rejection);
    malformed=bytes;malformed[4]=0xa9;assert(context_store_verify_protected_registry_v1(malformed.data(),encoded.encoded_size,f.key).status==context_store_protected_registry_status::structural_rejection); // unexpected body field/count
    malformed=bytes;malformed[7]=0;assert(context_store_verify_protected_registry_v1(malformed.data(),encoded.encoded_size,f.key).status==context_store_protected_registry_status::structural_rejection); // duplicate body key zero
    for(size_t version_offset:{size_t(6),size_t(8),size_t(10)}){malformed=bytes;malformed[version_offset]=2;assert(context_store_verify_protected_registry_v1(malformed.data(),encoded.encoded_size,f.key).status==context_store_protected_registry_status::structural_rejection);}
    std::vector<uint8_t>noncanonical(encoded.encoded_size+1);noncanonical[0]=0xb8;noncanonical[1]=2;std::copy_n(bytes.begin()+1,encoded.encoded_size-1,noncanonical.begin()+2);assert(context_store_verify_protected_registry_v1(noncanonical.data(),noncanonical.size(),f.key).status==context_store_protected_registry_status::structural_rejection);
    auto other=f.key;other.key_id=id("other");assert(context_store_verify_protected_registry_v1(bytes.data(),encoded.encoded_size,other).status==context_store_protected_registry_status::unknown_key);
    other=f.key;++other.generation;assert(context_store_verify_protected_registry_v1(bytes.data(),encoded.encoded_size,other).status==context_store_protected_registry_status::key_generation_mismatch);
    other=f.key;other.disposition=context_store_key_disposition::revoked;assert(context_store_verify_protected_registry_v1(bytes.data(),encoded.encoded_size,other).status==context_store_protected_registry_status::revoked_key);
    other=f.key;other.disposition=context_store_key_disposition::read_disabled;assert(context_store_verify_protected_registry_v1(bytes.data(),encoded.encoded_size,other).status==context_store_protected_registry_status::read_disabled_key);
    auto b=body();b.registry_id={};assert(context_store_encode_protected_registry_v1(b,f.key,bytes.data(),bytes.size()).status==context_store_protected_registry_status::invalid_policy);
    b=body();b.registry_epoch=0;assert(context_store_encode_protected_registry_v1(b,f.key,bytes.data(),bytes.size()).status==context_store_protected_registry_status::invalid_policy);
    b=body();b.authority_base_scope_commitment.fill(0);assert(context_store_encode_protected_registry_v1(b,f.key,bytes.data(),bytes.size()).status==context_store_protected_registry_status::invalid_policy);
    b=body();b.policy_commitment.fill(0);assert(context_store_encode_protected_registry_v1(b,f.key,bytes.data(),bytes.size()).status==context_store_protected_registry_status::invalid_policy);
    b=body(UINT64_MAX);assert(context_store_encode_protected_registry_v1(b,f.key,bytes.data(),bytes.size()).status==context_store_protected_registry_status::invalid_policy);
    for(const uint8_t invalid:{uint8_t(0x80),uint8_t(0xff)}){b=body();b.registry_id.bytes[0]=static_cast<char>(invalid);assert(context_store_encode_protected_registry_v1(b,f.key,bytes.data(),bytes.size()).status==context_store_protected_registry_status::invalid_policy);
        auto authenticated_invalid=exact;const std::array<uint8_t,11>needle={'r','e','g','i','s','t','r','y','-','v','1'};auto where=std::search(authenticated_invalid.begin(),authenticated_invalid.begin()+encoded.encoded_size,needle.begin(),needle.end());assert(where!=authenticated_invalid.begin()+encoded.encoded_size);*where=invalid;retag(authenticated_invalid,encoded.encoded_size,f);assert(context_store_verify_protected_registry_v1(authenticated_invalid.data(),encoded.encoded_size,f.key).status==context_store_protected_registry_status::structural_rejection);}
    assert(std::string(context_store_protected_registry_status_name(context_store_protected_registry_status::authenticated_unadmitted))=="authenticated-unadmitted");
}
