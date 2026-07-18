#include "halofpx-context-store-authority.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

static_assert(!std::is_aggregate_v<halofpx::context_store_bootstrap_plan>);
static_assert(!std::is_aggregate_v<halofpx::context_store_bootstrap_result>);
static_assert(!std::is_copy_constructible_v<halofpx::context_store_bootstrap_authority>);
static_assert(!std::is_copy_assignable_v<halofpx::context_store_bootstrap_authority>);
static_assert(!std::is_move_constructible_v<halofpx::context_store_bootstrap_authority>);
static_assert(!std::is_move_assignable_v<halofpx::context_store_bootstrap_authority>);

namespace {
using bytes = std::vector<uint8_t>;
using digest = halofpx::context_store_format_digest;

void head(bytes & out, uint8_t major, uint64_t value) {
    if (value < 24) out.push_back(static_cast<uint8_t>((major << 5) | value));
    else if (value <= 0xff) { out.push_back(static_cast<uint8_t>((major << 5) | 24)); out.push_back(static_cast<uint8_t>(value)); }
    else if (value <= 0xffff) { out.push_back(static_cast<uint8_t>((major << 5) | 25)); out.push_back(static_cast<uint8_t>(value >> 8)); out.push_back(static_cast<uint8_t>(value)); }
    else { out.push_back(static_cast<uint8_t>((major << 5) | 27)); for (int s = 56; s >= 0; s -= 8) out.push_back(static_cast<uint8_t>(value >> s)); }
}
void u(bytes & out, uint64_t value) { head(out, 0, value); }
void map(bytes & out, uint64_t count) { head(out, 5, count); }
void array(bytes & out, uint64_t count) { head(out, 4, count); }
void raw(bytes & out, const uint8_t * data, size_t size) { head(out, 2, size); out.insert(out.end(), data, data + size); }
void fill(bytes & out, size_t size, uint8_t value) { head(out, 2, size); out.insert(out.end(), size, value); }
void d(bytes & out, const digest & value) { raw(out, value.data(), value.size()); }
void text(bytes & out, const std::string & value) { head(out, 3, value.size()); out.insert(out.end(), value.begin(), value.end()); }
bytes domain(const char * value, const bytes & body) {
    bytes out(reinterpret_cast<const uint8_t *>(value), reinterpret_cast<const uint8_t *>(value) + std::strlen(value) + 1);
    out.insert(out.end(), body.begin(), body.end()); return out;
}
digest sha(const bytes & value) { digest out {}; assert(halofpx::context_store_sha256(value.data(), value.size(), out)); return out; }
digest mac(const bytes & key, const bytes & value) { digest out {}; assert(halofpx::context_store_hmac_sha256(key.data(), key.size(), value.data(), value.size(), out)); return out; }
halofpx::context_store_registered_id rid(const std::string & value) {
    halofpx::context_store_registered_id out; out.size = static_cast<uint8_t>(value.size());
    std::copy(value.begin(), value.end(), out.bytes.begin()); return out;
}

struct manifest_options {
    std::string key_id = "manifest-key-v1";
    uint64_t key_generation = 5;
    uint64_t generation = 1;
    bool predecessor = false;
    uint8_t store = 0x02, lineage = 0x03, scope = 0x80;
    uint64_t policy_epoch = 7;
    size_t objects = 1;
    uint8_t compatibility_delta = 0;
};
struct signed_manifest {
    bytes data;
    bytes key = bytes(64, 0x33);
    halofpx::context_store_compatibility_expectation compatibility;
};

signed_manifest make_manifest(const manifest_options & option = {}) {
    signed_manifest result;
    for (size_t i = 0; i < result.compatibility.components.size(); ++i)
        result.compatibility.components[i].fill(static_cast<uint8_t>(0x20 + i));
    result.compatibility.components[0][0] ^= option.compatibility_delta;
    bytes compatibility;
    map(compatibility, 16);
    for (size_t i = 0; i < result.compatibility.components.size(); ++i) { u(compatibility, i); d(compatibility, result.compatibility.components[i]); }
    result.compatibility.root = sha(domain("halofpx.compat.v1", compatibility));

    bytes body;
    map(body, 15);
    u(body,0);u(body,1); u(body,1);u(body,0); u(body,2);fill(body,16,option.store);
    u(body,3);fill(body,32,option.lineage); u(body,4);u(body,option.generation); u(body,5);
    if (option.predecessor) fill(body,32,0x05); else body.push_back(0xf6);
    u(body,6);body.insert(body.end(), compatibility.begin(), compatibility.end());
    u(body,7);d(body,result.compatibility.root); u(body,8);fill(body,32,option.scope); u(body,9);u(body,option.policy_epoch);
    u(body,10); map(body,6); u(body,0);text(body,"plan.synthetic.v1"); u(body,1);text(body,"single");
    u(body,2);u(body,1); u(body,3);fill(body,32,0xa3); u(body,4);u(body,9); u(body,5);array(body,1);
    map(body,3);u(body,0);u(body,0);u(body,1);fill(body,32,0xb0);u(body,2);fill(body,32,0xc0);
    u(body,11);text(body,"profile.synthetic.v1"); u(body,12);array(body,option.objects);
    for (size_t object = 0; object < option.objects; ++object) {
        map(body,13); u(body,0);fill(body,32,static_cast<uint8_t>(0xd0 + object)); u(body,1);text(body,"tokens");
        u(body,2);text(body,"codec.synthetic.v1"); u(body,3);u(body,1); u(body,4);u(body,object);
        u(body,5);body.push_back(0xf5); u(body,6);u(body,64); u(body,7);fill(body,32,0xd7);
        u(body,8);u(body,16); u(body,9);u(body,8); u(body,10);u(body,0); u(body,11);fill(body,32,0xb0);
        u(body,12);d(body,result.compatibility.root);
    }
    u(body,13);fill(body,32,0xe3);u(body,14);u(body,0);

    bytes auth;
    map(auth,4);u(auth,0);auth.insert(auth.end(),body.begin(),body.end());u(auth,1);text(auth,option.key_id);
    u(auth,2);u(auth,1);u(auth,3);u(auth,option.key_generation);
    bytes kdf(reinterpret_cast<const uint8_t *>("halofpx.manifest-key.v1"),
        reinterpret_cast<const uint8_t *>("halofpx.manifest-key.v1") + sizeof("halofpx.manifest-key.v1"));
    kdf.push_back(static_cast<uint8_t>(option.key_id.size() >> 8)); kdf.push_back(static_cast<uint8_t>(option.key_id.size()));
    kdf.insert(kdf.end(),option.key_id.begin(),option.key_id.end()); kdf.insert(kdf.end(),16,option.store);
    kdf.insert(kdf.end(),32,option.scope); for (int s=56;s>=0;s-=8) kdf.push_back(static_cast<uint8_t>(option.key_generation >> s));
    const auto derived = mac(result.key,kdf); const auto tag = mac(bytes(derived.begin(),derived.end()),domain("halofpx.manifest-auth.v1",auth));
    map(result.data,2);u(result.data,0);result.data.insert(result.data.end(),auth.begin(),auth.end());u(result.data,1);d(result.data,tag);
    return result;
}

struct fixture {
    std::array<uint8_t,64> anchor {}, admin {}, manifest {};
    halofpx::context_store_bootstrap_authority_config config;
    signed_manifest signed_data;
};
void bind(fixture & f) {
    f.config.anchor_signing_key.master_key={f.anchor.data(),f.anchor.size()};
    f.config.bootstrap_admin_key.master_key={f.admin.data(),f.admin.size()};
    f.config.manifest_authentication_key.master_key={f.manifest.data(),f.manifest.size()};
}
fixture make_fixture() {
    fixture f; for(size_t i=0;i<64;++i){f.anchor[i]=static_cast<uint8_t>(i+1);f.admin[i]=static_cast<uint8_t>(0xf0-i);f.manifest[i]=0x33;}
    f.signed_data=make_manifest(); f.config.anchor_signing_key={halofpx::context_store_key_disposition::active,rid("anchor-key-v1"),7,{}};
    f.config.bootstrap_admin_key={halofpx::context_store_key_disposition::active,rid("bootstrap-admin-v1"),11,{}};
    f.config.manifest_authentication_key={halofpx::context_store_key_disposition::active,rid("manifest-key-v1"),5,{}};
    f.config.trusted_compatibility=f.signed_data.compatibility; f.config.store_uuid.fill(0x02);f.config.namespace_id.fill(0x80);
    f.config.policy_epoch=7;f.config.checkpoint_lineage_id.fill(0x03);f.config.manifest_key_generation=5;f.config.authority_epoch=6;bind(f);return f;
}
halofpx::context_store_bootstrap_request request(bytes & manifest,uint64_t attempt=91) {
    halofpx::context_store_bootstrap_request out;for(size_t i=0;i<8;++i)out.attempt_id[31-i]=static_cast<uint8_t>(attempt>>(i*8));
    out.manifest_data=manifest.data();out.manifest_size=manifest.size();return out;
}
const halofpx::context_store_bootstrap_plan & plan(const halofpx::context_store_bootstrap_result & result) {
    assert(result.status==halofpx::context_store_bootstrap_status::authorized_unexecuted&&result.has_authorized_plan());return *result.authorized_plan();
}
void rejected(const halofpx::context_store_bootstrap_result & result) { assert(!result.has_authorized_plan()&&result.authorized_plan()==nullptr); }

void test_derived_plan_and_ownership() {
    auto f=make_fixture();halofpx::context_store_bootstrap_authority authority(f.config);assert(authority.valid());
    auto req=request(f.signed_data.data);const auto result=authority.plan(req);const auto & p=plan(result);digest expected{};
    assert(halofpx::context_store_manifest_digest_v1(f.signed_data.data.data(),f.signed_data.data.size(),expected));
    assert(p.object_count()==1&&*p.selected_manifest_digest()==expected&&p.anchor()->body()->selected_manifest_digest==expected);
    const auto authorization=*p.authorization_commitment(), anchor=*p.anchor()->envelope_digest();
    f.anchor.fill(0);f.admin.fill(0);f.manifest.fill(0);f.config.trusted_compatibility={};
    assert(*plan(authority.plan(req)).authorization_commitment()==authorization);
    auto retained=p;f.signed_data.data.assign(f.signed_data.data.size(),0);assert(*retained.anchor()->envelope_digest()==anchor);
    fixture temporary=make_fixture();auto owned=std::make_unique<halofpx::context_store_bootstrap_authority>(temporary.config);
    auto temp_request=request(temporary.signed_data.data);temporary.anchor.fill(0);temporary.admin.fill(0);temporary.manifest.fill(0);bind(temporary);
    assert(plan(owned->plan(temp_request)).object_count()==1);
    halofpx::context_store_bootstrap_plan after_authority;
    {
        auto scoped=make_fixture();halofpx::context_store_bootstrap_authority scoped_authority(scoped.config);
        after_authority=plan(scoped_authority.plan(request(scoped.signed_data.data)));
    }
    assert(after_authority.authorized()&&after_authority.anchor()!=nullptr&&after_authority.anchor()->authenticated());
}

void test_manifest_rejections() {
    auto f=make_fixture();halofpx::context_store_bootstrap_authority authority(f.config);
    auto bad=f.signed_data.data;bad.pop_back();rejected(authority.plan(request(bad)));
    bad=f.signed_data.data;bad.back()^=1;rejected(authority.plan(request(bad)));
    bad=f.signed_data.data;bad[20]^=1;rejected(authority.plan(request(bad)));
    std::vector<manifest_options> options;
    auto option=manifest_options{};option.key_id="wrong-key";options.push_back(option);
    option=manifest_options{};option.key_generation=6;options.push_back(option);
    option=manifest_options{};option.generation=2;options.push_back(option);
    option=manifest_options{};option.predecessor=true;options.push_back(option);
    option=manifest_options{};option.store=0x04;options.push_back(option);
    option=manifest_options{};option.lineage=0x04;options.push_back(option);
    option=manifest_options{};option.scope=0x81;options.push_back(option);
    option=manifest_options{};option.policy_epoch=8;options.push_back(option);
    option=manifest_options{};option.compatibility_delta=1;options.push_back(option);
    for (const auto & value : options) { auto other=make_manifest(value);rejected(authority.plan(request(other.data))); }
    for(size_t count:{size_t(0),size_t(129)}){manifest_options o;o.objects=count;auto other=make_manifest(o);rejected(authority.plan(request(other.data)));}
    auto wrong_root=make_fixture();wrong_root.config.trusted_compatibility.root[0]^=1;halofpx::context_store_bootstrap_authority root_authority(wrong_root.config);
    assert(!root_authority.valid());rejected(root_authority.plan(request(wrong_root.signed_data.data)));
    auto wrong_secret=make_fixture();wrong_secret.manifest[0]^=1;bind(wrong_secret);halofpx::context_store_bootstrap_authority secret_authority(wrong_secret.config);
    assert(secret_authority.valid());rejected(secret_authority.plan(request(wrong_secret.signed_data.data)));
}

void test_invalid_authority_and_separation() {
    auto invalid=[](fixture f){bind(f);halofpx::context_store_bootstrap_authority a(f.config);assert(!a.valid());auto r=request(f.signed_data.data);rejected(a.plan(r));};
    for(const auto disposition:{halofpx::context_store_key_disposition::unknown,halofpx::context_store_key_disposition::revoked,halofpx::context_store_key_disposition::read_disabled}) {
        auto f=make_fixture();f.config.anchor_signing_key.disposition=disposition;invalid(f);
        f=make_fixture();f.config.bootstrap_admin_key.disposition=disposition;invalid(f);
        f=make_fixture();f.config.manifest_authentication_key.disposition=disposition;invalid(f);
    }
    auto f=make_fixture();f.config.anchor_signing_key.generation=0;invalid(f);
    f=make_fixture();f.config.bootstrap_admin_key.generation=0;invalid(f);
    f=make_fixture();f.config.manifest_authentication_key.generation=0;invalid(f);f=make_fixture();f.config.manifest_key_generation=6;invalid(f);
    f=make_fixture();f.config.manifest_authentication_key.key_id=f.config.anchor_signing_key.key_id;f.config.manifest_authentication_key.generation=f.config.anchor_signing_key.generation;invalid(f);
    f=make_fixture();f.config.manifest_authentication_key.key_id=f.config.bootstrap_admin_key.key_id;f.config.manifest_authentication_key.generation=f.config.bootstrap_admin_key.generation;invalid(f);
    f=make_fixture();f.manifest=f.anchor;invalid(f);f=make_fixture();f.manifest=f.admin;invalid(f);
    f=make_fixture();f.admin=f.anchor;invalid(f);
    f=make_fixture();f.config.store_uuid.fill(0);invalid(f);f=make_fixture();f.config.namespace_id.fill(0);invalid(f);
    f=make_fixture();f.config.checkpoint_lineage_id.fill(0);invalid(f);f=make_fixture();f.config.policy_epoch=0;invalid(f);
    f=make_fixture();f.config.manifest_key_generation=0;invalid(f);f=make_fixture();f.config.authority_epoch=0;invalid(f);
}

void test_attempt_snapshot_bindings_and_concurrency() {
    auto f=make_fixture();halofpx::context_store_bootstrap_authority authority(f.config);auto req=request(f.signed_data.data);
    auto zero=req;zero.attempt_id.fill(0);auto zero_result=authority.plan(zero);assert(zero_result.status==halofpx::context_store_bootstrap_status::invalid_request);rejected(zero_result);
    auto null=req;null.manifest_data=nullptr;auto null_result=authority.plan(null);assert(null_result.status==halofpx::context_store_bootstrap_status::invalid_request);rejected(null_result);
    bytes oversized(halofpx::context_store_manifest_max_bytes+1,0);auto too_large=request(oversized);auto large_result=authority.plan(too_large);
    assert(large_result.status==halofpx::context_store_bootstrap_status::invalid_request);rejected(large_result);
    const auto baseline=plan(authority.plan(req));auto second=req;second.attempt_id[0]=1;assert(*plan(authority.plan(second)).authorization_commitment()!=*baseline.authorization_commitment());
    auto changed=make_fixture();manifest_options changed_compatibility;changed_compatibility.compatibility_delta=1;
    changed.signed_data=make_manifest(changed_compatibility);changed.config.trusted_compatibility=changed.signed_data.compatibility;
    bind(changed);halofpx::context_store_bootstrap_authority changed_authority(changed.config);
    assert(*plan(changed_authority.plan(request(changed.signed_data.data))).authority_snapshot_commitment()!=*baseline.authority_snapshot_commitment());
    auto changed_anchor=make_fixture();changed_anchor.anchor[0]^=1;bind(changed_anchor);halofpx::context_store_bootstrap_authority changed_anchor_authority(changed_anchor.config);
    const auto changed_anchor_plan=plan(changed_anchor_authority.plan(request(changed_anchor.signed_data.data)));
    assert(*changed_anchor_plan.authority_snapshot_commitment()!=*baseline.authority_snapshot_commitment());
    assert(*changed_anchor_plan.anchor()->envelope_digest()!=*baseline.anchor()->envelope_digest());
    auto changed_admin=make_fixture();changed_admin.admin[0]^=1;bind(changed_admin);halofpx::context_store_bootstrap_authority changed_admin_authority(changed_admin.config);
    const auto changed_admin_plan=plan(changed_admin_authority.plan(request(changed_admin.signed_data.data)));
    assert(*changed_admin_plan.authority_snapshot_commitment()!=*baseline.authority_snapshot_commitment());
    assert(*changed_admin_plan.anchor()->envelope_digest()==*baseline.anchor()->envelope_digest());
    auto changed_epoch=make_fixture();++changed_epoch.config.authority_epoch;bind(changed_epoch);halofpx::context_store_bootstrap_authority changed_epoch_authority(changed_epoch.config);
    const auto changed_epoch_plan=plan(changed_epoch_authority.plan(request(changed_epoch.signed_data.data)));
    assert(*changed_epoch_plan.authority_snapshot_commitment()!=*baseline.authority_snapshot_commitment());
    assert(*changed_epoch_plan.anchor()->envelope_digest()!=*baseline.anchor()->envelope_digest());
    std::array<digest,32> values{};std::vector<std::thread> threads;for(size_t i=0;i<values.size();++i)threads.emplace_back([&,i]{values[i]=*plan(authority.plan(req)).authorization_commitment();});
    for(auto & thread:threads)thread.join();for(const auto & value:values)assert(value==*baseline.authorization_commitment());
    assert(std::string(halofpx::context_store_bootstrap_status_name(halofpx::context_store_bootstrap_status::manifest_rejected))=="manifest-rejected");
}
}

int main(){test_derived_plan_and_ownership();test_manifest_rejections();test_invalid_authority_and_separation();test_attempt_snapshot_bindings_and_concurrency();}
