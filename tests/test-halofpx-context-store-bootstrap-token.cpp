#include "halofpx-context-store-bootstrap-token.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <type_traits>

using namespace halofpx;

namespace {
context_store_registered_id id(const char * text) {
    context_store_registered_id value; value.size=static_cast<uint8_t>(std::strlen(text));
    std::copy_n(reinterpret_cast<const uint8_t *>(text),value.size,value.bytes.begin()); return value;
}
context_store_bootstrap_token_body body() {
    context_store_bootstrap_token_body b;b.store_uuid.fill(1);b.namespace_id.fill(2);b.policy_epoch=3;b.checkpoint_lineage_id.fill(4);
    b.manifest_key_id=id("manifest");b.manifest_key_generation=5;b.compatibility_root.fill(6);b.authority_epoch=7;
    b.anchor_key_id=id("anchor");b.anchor_key_generation=8;b.selected_manifest_digest.fill(9);b.authority_scope_commitment.fill(10);
    b.protected_registry_id=id("registry");b.protected_registry_epoch=11;b.protected_registry_snapshot_digest.fill(12);
    b.protected_registry_policy_digest.fill(13);b.authorization_sequence=14;b.command_id.fill(15);return b;
}
struct fixture { std::array<uint8_t,32> secret{}; context_store_bootstrap_admin_key_record key; fixture(){secret.fill(0x41);key.disposition=context_store_key_disposition::active;key.key_id=id("admin");key.generation=4;key.master_key={secret.data(),secret.size()};} };
}

int main() {
    static_assert(context_store_bootstrap_token_max_bytes==2048);
    static_assert(!std::is_aggregate_v<context_store_authenticated_bootstrap_token>);
    fixture f;auto input=body();std::array<uint8_t,context_store_bootstrap_token_max_bytes> bytes{};
    auto encoded=context_store_encode_bootstrap_token_v1(input,f.key,bytes.data(),bytes.size());
    assert(encoded.status==context_store_bootstrap_token_status::authenticated_unconsumed&&encoded.encoded_size>0);
    auto verified=context_store_verify_bootstrap_token_v1(bytes.data(),encoded.encoded_size,f.key);
    auto * carrier=verified.authenticated_carrier();assert(carrier&&carrier->body()&&carrier->envelope_digest());
    const context_store_format_digest golden_digest={0x3e,0x2b,0x07,0xd2,0x32,0x1d,0xaf,0xe8,0x45,0x10,0xc5,0xb2,0x06,0x24,0x25,0xc4,
        0x15,0x4c,0xeb,0x8e,0x0c,0xf7,0x7a,0x2f,0x81,0xdc,0x74,0xb3,0x93,0xa5,0xe3,0x20};
    assert(encoded.encoded_size==399&&*carrier->envelope_digest()==golden_digest);
    assert(carrier->body()->authorization_sequence==14&&carrier->body()->command_id==input.command_id);
    const auto owned_body=*carrier->body();const auto owned_digest=*carrier->envelope_digest();bytes.fill(0);
    assert(carrier->body()->command_id==owned_body.command_id&&*carrier->envelope_digest()==owned_digest);

    std::array<uint8_t,context_store_bootstrap_token_max_bytes> exact{};encoded=context_store_encode_bootstrap_token_v1(input,f.key,exact.data(),exact.size());
    const auto repeated=context_store_verify_bootstrap_token_v1(exact.data(),encoded.encoded_size,f.key);
    assert(repeated.status==context_store_bootstrap_token_status::authenticated_unconsumed);
    for(size_t truncated=0;truncated<encoded.encoded_size;++truncated)
        assert(context_store_verify_bootstrap_token_v1(exact.data(),truncated,f.key).authenticated_carrier()==nullptr);
    for(size_t offset=0;offset<encoded.encoded_size;++offset){auto changed=exact;changed[offset]^=1;
        assert(context_store_verify_bootstrap_token_v1(changed.data(),encoded.encoded_size,f.key).authenticated_carrier()==nullptr);}
    assert(context_store_encode_bootstrap_token_v1(input,f.key,exact.data(),1).status==context_store_bootstrap_token_status::output_too_small);
    assert(context_store_verify_bootstrap_token_v1(exact.data(),encoded.encoded_size-1,f.key).authenticated_carrier()==nullptr);
    auto extra=exact;extra[encoded.encoded_size]=0;assert(context_store_verify_bootstrap_token_v1(extra.data(),encoded.encoded_size+1,f.key).status==context_store_bootstrap_token_status::structural_rejection);
    std::array<uint8_t,context_store_bootstrap_token_max_bytes> noncanonical{};noncanonical[0]=0xb8;noncanonical[1]=2;
    std::copy_n(exact.data()+1,encoded.encoded_size-1,noncanonical.data()+2);
    assert(context_store_verify_bootstrap_token_v1(noncanonical.data(),encoded.encoded_size+1,f.key).status==context_store_bootstrap_token_status::structural_rejection);
    auto corrupt=exact;corrupt[encoded.encoded_size-1]^=1;assert(context_store_verify_bootstrap_token_v1(corrupt.data(),encoded.encoded_size,f.key).status==context_store_bootstrap_token_status::authentication_failed);
    corrupt=exact;corrupt[0]=0xa4;assert(context_store_verify_bootstrap_token_v1(corrupt.data(),encoded.encoded_size,f.key).status==context_store_bootstrap_token_status::structural_rejection);
    corrupt=exact;corrupt[0]=0xa5;assert(context_store_verify_bootstrap_token_v1(corrupt.data(),encoded.encoded_size,f.key).status==context_store_bootstrap_token_status::structural_rejection); // former five-field envelope
    fixture other;other.key.key_id=id("other");assert(context_store_verify_bootstrap_token_v1(exact.data(),encoded.encoded_size,other.key).status==context_store_bootstrap_token_status::unknown_key);
    auto wrong_generation=f.key;wrong_generation.generation++;assert(context_store_verify_bootstrap_token_v1(exact.data(),encoded.encoded_size,wrong_generation).status==context_store_bootstrap_token_status::key_generation_mismatch);
    auto revoked=f.key;revoked.disposition=context_store_key_disposition::revoked;assert(context_store_verify_bootstrap_token_v1(exact.data(),encoded.encoded_size,revoked).status==context_store_bootstrap_token_status::revoked_key);
    auto disabled=f.key;disabled.disposition=context_store_key_disposition::read_disabled;assert(context_store_verify_bootstrap_token_v1(exact.data(),encoded.encoded_size,disabled).status==context_store_bootstrap_token_status::read_disabled_key);
    input.command_id.fill(0);assert(context_store_encode_bootstrap_token_v1(input,f.key,exact.data(),exact.size()).status==context_store_bootstrap_token_status::invalid_policy);
}
