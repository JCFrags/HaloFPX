#include "halofpx-context-store-authority.h"
#include "halofpx-context-store-bootstrap-consumption.h"
#include "halofpx-context-store-bootstrap-material.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
#include <array>
#include <cassert>
#include <climits>
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

namespace halofpx { class context_store_bootstrap_material_synthetic_test_access { public: static void set(void(*hook)()){context_store_bootstrap_material_synthetic_coordinator::post_positive_test_hook_=hook;} }; }

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
    std::vector<digest> object_ids;
    std::vector<uint64_t> frame_bytes;
    uint8_t durability_mode = 0;
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
        map(body,13); u(body,0);if(object<option.object_ids.size())d(body,option.object_ids[object]);else fill(body,32,static_cast<uint8_t>(0xd0 + object)); u(body,1);text(body,"tokens");
        u(body,2);text(body,"codec.synthetic.v1"); u(body,3);u(body,1); u(body,4);u(body,object);
        u(body,5);body.push_back(0xf5); u(body,6);u(body,object<option.frame_bytes.size()?option.frame_bytes[object]:64); u(body,7);fill(body,32,0xd7);
        u(body,8);u(body,16); u(body,9);u(body,8); u(body,10);u(body,0); u(body,11);fill(body,32,0xb0);
        u(body,12);d(body,result.compatibility.root);
    }
    u(body,13);fill(body,32,0xe3);u(body,14);u(body,option.durability_mode);

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
    std::array<uint8_t,64> anchor {}, admin {}, manifest {}, registry {};
    halofpx::context_store_bootstrap_authority_config config;
    signed_manifest signed_data;
    bytes token;
    bytes registry_snapshot;
};
void bind(fixture & f) {
    f.config.anchor_signing_key.master_key={f.anchor.data(),f.anchor.size()};
    f.config.bootstrap_admin_key.master_key={f.admin.data(),f.admin.size()};
    f.config.manifest_authentication_key.master_key={f.manifest.data(),f.manifest.size()};
    f.config.protected_registry_authentication_key.master_key={f.registry.data(),f.registry.size()};
    f.config.protected_registry_snapshot_data=f.registry_snapshot.data();
    f.config.protected_registry_snapshot_size=f.registry_snapshot.size();
}
halofpx::context_store_bootstrap_token_body token_body(fixture & f, uint64_t command=91) {
    halofpx::context_store_bootstrap_token_body body;
    body.store_uuid=f.config.store_uuid;body.namespace_id=f.config.namespace_id;body.policy_epoch=f.config.policy_epoch;
    body.checkpoint_lineage_id=f.config.checkpoint_lineage_id;body.manifest_key_id=f.config.manifest_authentication_key.key_id;
    body.manifest_key_generation=f.config.manifest_key_generation;body.compatibility_root=f.config.trusted_compatibility.root;
    body.authority_epoch=f.config.authority_epoch;body.anchor_key_id=f.config.anchor_signing_key.key_id;
    body.anchor_key_generation=f.config.anchor_signing_key.generation;assert(halofpx::context_store_manifest_digest_v1(f.signed_data.data.data(),f.signed_data.data.size(),body.selected_manifest_digest));
    assert(halofpx::context_store_bootstrap_authority_scope_commitment(f.config,body.authority_scope_commitment));
    auto rr=halofpx::context_store_verify_protected_registry_v1(f.config.protected_registry_snapshot_data,f.config.protected_registry_snapshot_size,f.config.protected_registry_authentication_key);
    auto rc=rr.authenticated_carrier();assert(rc&&rc->body()&&rc->envelope_digest());
    body.protected_registry_id=rc->body()->registry_id;body.protected_registry_epoch=rc->body()->registry_epoch;
    body.protected_registry_snapshot_digest=*rc->envelope_digest();body.protected_registry_policy_digest=rc->body()->policy_commitment;
    body.authorization_sequence=rc->body()->last_consumed_sequence+1;for(size_t i=0;i<8;++i)body.command_id[31-i]=static_cast<uint8_t>(command>>(i*8));
    return body;
}
void encode_token(fixture & f, const halofpx::context_store_bootstrap_token_body & body) {
    f.token.resize(halofpx::context_store_bootstrap_token_max_bytes);auto encoded=halofpx::context_store_encode_bootstrap_token_v1(body,f.config.bootstrap_admin_key,f.token.data(),f.token.size());
    assert(encoded.status==halofpx::context_store_bootstrap_token_status::authenticated_unconsumed);f.token.resize(encoded.encoded_size);
}
void authorize(fixture & f, uint64_t command=91) { encode_token(f,token_body(f,command)); }
void seal_registry(fixture & f, uint64_t high=40) {
    bind(f);digest base{};assert(halofpx::context_store_bootstrap_authority_base_scope_commitment(f.config,base));
    halofpx::context_store_protected_registry_body rb;rb.registry_id=rid("registry-v1");rb.registry_epoch=9;rb.authority_base_scope_commitment=base;rb.policy_commitment.fill(0x62);rb.last_consumed_sequence=high;
    f.registry_snapshot.resize(halofpx::context_store_protected_registry_max_bytes);auto re=halofpx::context_store_encode_protected_registry_v1(rb,f.config.protected_registry_authentication_key,f.registry_snapshot.data(),f.registry_snapshot.size());assert(re.status==halofpx::context_store_protected_registry_status::authenticated_unadmitted);f.registry_snapshot.resize(re.encoded_size);bind(f);
}
fixture make_fixture() {
    fixture f; for(size_t i=0;i<64;++i){f.anchor[i]=static_cast<uint8_t>(i+1);f.admin[i]=static_cast<uint8_t>(0xf0-i);f.manifest[i]=0x33;f.registry[i]=0x71;}
    f.signed_data=make_manifest(); f.config.anchor_signing_key={halofpx::context_store_key_disposition::active,rid("anchor-key-v1"),7,{}};
    f.config.bootstrap_admin_key={halofpx::context_store_key_disposition::active,rid("bootstrap-admin-v1"),11,{}};
    f.config.manifest_authentication_key={halofpx::context_store_key_disposition::active,rid("manifest-key-v1"),5,{}};
    f.config.protected_registry_authentication_key={halofpx::context_store_key_disposition::active,rid("registry-auth-v1"),13,{}};
    f.config.trusted_compatibility=f.signed_data.compatibility; f.config.store_uuid.fill(0x02);f.config.namespace_id.fill(0x80);
    f.config.policy_epoch=7;f.config.checkpoint_lineage_id.fill(0x03);f.config.manifest_key_generation=5;f.config.authority_epoch=6;
    seal_registry(f);authorize(f);return f;
}
halofpx::context_store_bootstrap_request request(fixture & f, bytes & manifest) {
    halofpx::context_store_bootstrap_request out;out.manifest_data=manifest.data();out.manifest_size=manifest.size();
    out.authorization_token_data=f.token.data();out.authorization_token_size=f.token.size();return out;
}
const halofpx::context_store_bootstrap_plan & plan(const halofpx::context_store_bootstrap_result & result) {
    assert(result.status==halofpx::context_store_bootstrap_status::authorized_unexecuted&&result.has_authorized_plan());return *result.authorized_plan();
}
void rejected(const halofpx::context_store_bootstrap_result & result) { assert(!result.has_authorized_plan()&&result.authorized_plan()==nullptr); }

void test_derived_plan_and_ownership() {
    auto f=make_fixture();halofpx::context_store_bootstrap_authority authority(f.config);assert(authority.valid());
    digest public_scope{};assert(halofpx::context_store_bootstrap_authority_scope_commitment(f.config,public_scope)&&public_scope==*authority.authority_scope_commitment());
    auto public_config=f.config;public_config.anchor_signing_key.master_key={};public_config.bootstrap_admin_key.master_key={};public_config.manifest_authentication_key.master_key={};public_config.protected_registry_authentication_key.master_key={};digest base_a{},base_b{};
    assert(halofpx::context_store_bootstrap_authority_base_scope_commitment(f.config,base_a)&&halofpx::context_store_bootstrap_authority_base_scope_commitment(public_config,base_b)&&base_a==base_b);
    auto req=request(f,f.signed_data.data);const auto result=authority.plan(req);const auto & p=plan(result);digest expected{};
    assert(halofpx::context_store_manifest_digest_v1(f.signed_data.data.data(),f.signed_data.data.size(),expected));
    assert(p.object_count()==1&&*p.selected_manifest_digest()==expected&&p.anchor()->body()->selected_manifest_digest==expected);
    const auto authorization=*p.plan_commitment(), anchor=*p.anchor()->envelope_digest();
    f.anchor.fill(0);f.admin.fill(0);f.manifest.fill(0);f.registry.fill(0);f.registry_snapshot.assign(f.registry_snapshot.size(),0);f.config.trusted_compatibility={};
    assert(*plan(authority.plan(req)).plan_commitment()==authorization);
    auto retained=p;f.signed_data.data.assign(f.signed_data.data.size(),0);assert(*retained.anchor()->envelope_digest()==anchor);
    fixture temporary=make_fixture();auto owned=std::make_unique<halofpx::context_store_bootstrap_authority>(temporary.config);
    auto temp_request=request(temporary,temporary.signed_data.data);temporary.anchor.fill(0);temporary.admin.fill(0);temporary.manifest.fill(0);bind(temporary);
    assert(plan(owned->plan(temp_request)).object_count()==1);
    halofpx::context_store_bootstrap_plan after_authority;
    {
        auto scoped=make_fixture();halofpx::context_store_bootstrap_authority scoped_authority(scoped.config);
        after_authority=plan(scoped_authority.plan(request(scoped,scoped.signed_data.data)));
    }
    assert(after_authority.authorized()&&after_authority.anchor()!=nullptr&&after_authority.anchor()->authenticated());
}

void test_manifest_rejections() {
    auto f=make_fixture();halofpx::context_store_bootstrap_authority authority(f.config);
    auto bad=f.signed_data.data;bad.pop_back();rejected(authority.plan(request(f,bad)));
    bad=f.signed_data.data;bad.back()^=1;rejected(authority.plan(request(f,bad)));
    bad=f.signed_data.data;bad[20]^=1;rejected(authority.plan(request(f,bad)));
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
    for (const auto & value : options) { auto other=make_manifest(value);rejected(authority.plan(request(f,other.data))); }
    for(size_t count:{size_t(0),size_t(129)}){manifest_options o;o.objects=count;auto other=make_manifest(o);rejected(authority.plan(request(f,other.data)));}
    auto wrong_root=make_fixture();wrong_root.config.trusted_compatibility.root[0]^=1;halofpx::context_store_bootstrap_authority root_authority(wrong_root.config);
    assert(!root_authority.valid());rejected(root_authority.plan(request(wrong_root,wrong_root.signed_data.data)));
    auto wrong_secret=make_fixture();wrong_secret.manifest[0]^=1;bind(wrong_secret);halofpx::context_store_bootstrap_authority secret_authority(wrong_secret.config);
    assert(secret_authority.valid());rejected(secret_authority.plan(request(wrong_secret,wrong_secret.signed_data.data)));
}

void test_invalid_authority_and_separation() {
    auto invalid=[](fixture f){bind(f);halofpx::context_store_bootstrap_authority a(f.config);assert(!a.valid());auto r=request(f,f.signed_data.data);rejected(a.plan(r));};
    for(const auto disposition:{halofpx::context_store_key_disposition::unknown,halofpx::context_store_key_disposition::revoked,halofpx::context_store_key_disposition::read_disabled}) {
        auto f=make_fixture();f.config.anchor_signing_key.disposition=disposition;invalid(f);
        f=make_fixture();f.config.bootstrap_admin_key.disposition=disposition;invalid(f);
        f=make_fixture();f.config.manifest_authentication_key.disposition=disposition;invalid(f);
        f=make_fixture();f.config.protected_registry_authentication_key.disposition=disposition;invalid(f);
    }
    auto f=make_fixture();f.config.anchor_signing_key.generation=0;invalid(f);
    f=make_fixture();f.config.bootstrap_admin_key.generation=0;invalid(f);
    f=make_fixture();f.config.manifest_authentication_key.generation=0;invalid(f);f=make_fixture();f.config.manifest_key_generation=6;invalid(f);
    f=make_fixture();f.config.protected_registry_authentication_key.generation=0;invalid(f);
    f=make_fixture();f.config.bootstrap_admin_key.key_id=f.config.anchor_signing_key.key_id;f.config.bootstrap_admin_key.generation=f.config.anchor_signing_key.generation;invalid(f);
    f=make_fixture();f.config.manifest_authentication_key.key_id=f.config.anchor_signing_key.key_id;f.config.manifest_authentication_key.generation=f.config.anchor_signing_key.generation;invalid(f);
    f=make_fixture();f.config.manifest_authentication_key.key_id=f.config.bootstrap_admin_key.key_id;f.config.manifest_authentication_key.generation=f.config.bootstrap_admin_key.generation;invalid(f);
    f=make_fixture();f.config.protected_registry_authentication_key.key_id=f.config.anchor_signing_key.key_id;f.config.protected_registry_authentication_key.generation=f.config.anchor_signing_key.generation;invalid(f);
    f=make_fixture();f.config.protected_registry_authentication_key.key_id=f.config.bootstrap_admin_key.key_id;f.config.protected_registry_authentication_key.generation=f.config.bootstrap_admin_key.generation;invalid(f);
    f=make_fixture();f.config.protected_registry_authentication_key.key_id=f.config.manifest_authentication_key.key_id;f.config.protected_registry_authentication_key.generation=f.config.manifest_authentication_key.generation;invalid(f);
    f=make_fixture();f.manifest=f.anchor;invalid(f);f=make_fixture();f.manifest=f.admin;invalid(f);
    f=make_fixture();f.admin=f.anchor;invalid(f);
    f=make_fixture();f.registry=f.anchor;invalid(f);f=make_fixture();f.registry=f.admin;invalid(f);f=make_fixture();f.registry=f.manifest;invalid(f);
    f=make_fixture();std::copy_n(f.anchor.begin(),63,f.registry.begin());f.registry[63]=0;seal_registry(f);f.config.anchor_signing_key.master_key={f.anchor.data(),63};{halofpx::context_store_bootstrap_authority a(f.config);assert(!a.valid());} // HMAC-equivalent padded secrets
    f=make_fixture();f.config.store_uuid.fill(0);invalid(f);f=make_fixture();f.config.namespace_id.fill(0);invalid(f);
    f=make_fixture();f.config.checkpoint_lineage_id.fill(0);invalid(f);f=make_fixture();f.config.policy_epoch=0;invalid(f);
    f=make_fixture();f.config.manifest_key_generation=0;invalid(f);f=make_fixture();f.config.authority_epoch=0;invalid(f);
    f=make_fixture();bind(f);f.config.protected_registry_snapshot_data=nullptr;{halofpx::context_store_bootstrap_authority a(f.config);assert(!a.valid());}
    f=make_fixture();bind(f);f.config.protected_registry_snapshot_size=0;{halofpx::context_store_bootstrap_authority a(f.config);assert(!a.valid());}
    f=make_fixture();f.registry_snapshot.back()^=1;invalid(f);f=make_fixture();f.config.protected_registry_authentication_key.key_id=rid("wrong-registry-key");invalid(f);
    f=make_fixture();{auto rr=halofpx::context_store_verify_protected_registry_v1(f.registry_snapshot.data(),f.registry_snapshot.size(),f.config.protected_registry_authentication_key);auto rb=*rr.authenticated_carrier()->body();rb.authority_base_scope_commitment[0]^=1;f.registry_snapshot.resize(halofpx::context_store_protected_registry_max_bytes);auto re=halofpx::context_store_encode_protected_registry_v1(rb,f.config.protected_registry_authentication_key,f.registry_snapshot.data(),f.registry_snapshot.size());assert(re.status==halofpx::context_store_protected_registry_status::authenticated_unadmitted);f.registry_snapshot.resize(re.encoded_size);invalid(f);}
}

void test_base_scope_sensitivity() {
    auto base=make_fixture();digest expected{};assert(halofpx::context_store_bootstrap_authority_base_scope_commitment(base.config,expected));
    auto differs=[&](auto mutate){auto f=base;bind(f);mutate(f.config);digest value{};assert(halofpx::context_store_bootstrap_authority_base_scope_commitment(f.config,value)&&value!=expected);};
    differs([](auto&c){c.bootstrap_admin_key.key_id=rid("admin-2");});differs([](auto&c){++c.bootstrap_admin_key.generation;});
    differs([](auto&c){c.anchor_signing_key.key_id=rid("anchor-2");});differs([](auto&c){++c.anchor_signing_key.generation;});
    differs([](auto&c){c.manifest_authentication_key.key_id=rid("manifest-2");});differs([](auto&c){++c.manifest_authentication_key.generation;});
    differs([](auto&c){c.protected_registry_authentication_key.key_id=rid("registry-auth-2");});differs([](auto&c){++c.protected_registry_authentication_key.generation;});
    differs([](auto&c){c.store_uuid[0]^=1;});differs([](auto&c){c.namespace_id[0]^=1;});differs([](auto&c){++c.policy_epoch;});
    differs([](auto&c){c.checkpoint_lineage_id[0]^=1;});differs([](auto&c){++c.manifest_key_generation;});differs([](auto&c){++c.authority_epoch;});
    for(size_t i=0;i<base.config.trusted_compatibility.components.size();++i)differs([i](auto&c){c.trusted_compatibility.components[i][0]^=1;});
    differs([](auto&c){c.trusted_compatibility.root[0]^=1;});
}

void test_public_scope_helpers_reject_malformed_ids() {
    auto f=make_fixture();digest output{};
    auto reject=[&](auto member){
        auto malformed=f.config;
        (malformed.*member).key_id.size=UINT8_MAX;
        assert(!halofpx::context_store_bootstrap_authority_base_scope_commitment(malformed,output));
        assert(!halofpx::context_store_bootstrap_authority_scope_commitment(malformed,output));
    };
    reject(&halofpx::context_store_bootstrap_authority_config::bootstrap_admin_key);
    reject(&halofpx::context_store_bootstrap_authority_config::anchor_signing_key);
    reject(&halofpx::context_store_bootstrap_authority_config::manifest_authentication_key);
    reject(&halofpx::context_store_bootstrap_authority_config::protected_registry_authentication_key);
}

void test_old_authenticated_snapshot_is_still_accepted() {
    auto old=make_fixture();auto old_snapshot=old.registry_snapshot;halofpx::context_store_bootstrap_authority old_authority(old.config);assert(old_authority.valid());
    auto req=request(old,old.signed_data.data);const auto first=plan(old_authority.plan(req));const auto again=plan(old_authority.plan(req));assert(*first.plan_commitment()==*again.plan_commitment());
    auto newer=old;bind(newer);seal_registry(newer,41);authorize(newer);halofpx::context_store_bootstrap_authority newer_authority(newer.config);assert(newer_authority.valid());
    old.registry_snapshot=old_snapshot;bind(old);halofpx::context_store_bootstrap_authority reconstructed_old(old.config);assert(reconstructed_old.valid());
    assert(*plan(reconstructed_old.plan(request(old,old.signed_data.data))).plan_commitment()==*first.plan_commitment());
}

void test_attempt_snapshot_bindings_and_concurrency() {
    auto f=make_fixture();halofpx::context_store_bootstrap_authority authority(f.config);auto req=request(f,f.signed_data.data);
    auto null=req;null.manifest_data=nullptr;auto null_result=authority.plan(null);assert(null_result.status==halofpx::context_store_bootstrap_status::invalid_request);rejected(null_result);
    auto missing_token=req;missing_token.authorization_token_data=nullptr;auto missing_result=authority.plan(missing_token);assert(missing_result.status==halofpx::context_store_bootstrap_status::invalid_request);rejected(missing_result);
    bytes oversized(halofpx::context_store_manifest_max_bytes+1,0);auto too_large=request(f,oversized);auto large_result=authority.plan(too_large);
    assert(large_result.status==halofpx::context_store_bootstrap_status::invalid_request);rejected(large_result);
    const auto baseline=plan(authority.plan(req));assert(baseline.authorization_sequence()==41&&baseline.command_id()!=nullptr&&baseline.authorization_token_digest()!=nullptr);
    assert(*plan(authority.plan(req)).plan_commitment()==*baseline.plan_commitment()); // stateless: not consumed here
    auto corrupt=f.token;corrupt.back()^=1;auto rejected_token=req;rejected_token.authorization_token_data=corrupt.data();
    auto bad_token_result=authority.plan(rejected_token);assert(bad_token_result.status==halofpx::context_store_bootstrap_status::authorization_rejected);rejected(bad_token_result);
    bytes malformed_manifest={0xff};rejected_token.manifest_data=malformed_manifest.data();rejected_token.manifest_size=malformed_manifest.size();
    bad_token_result=authority.plan(rejected_token);assert(bad_token_result.status==halofpx::context_store_bootstrap_status::authorization_rejected); // token first
    auto second_fixture=f;bind(second_fixture);authorize(second_fixture,92);auto second=request(second_fixture,second_fixture.signed_data.data);
    assert(*plan(authority.plan(second)).plan_commitment()!=*baseline.plan_commitment());
    auto changed=make_fixture();manifest_options changed_compatibility;changed_compatibility.compatibility_delta=1;
    changed.signed_data=make_manifest(changed_compatibility);changed.config.trusted_compatibility=changed.signed_data.compatibility;
    seal_registry(changed);authorize(changed);halofpx::context_store_bootstrap_authority changed_authority(changed.config);
    assert(*plan(changed_authority.plan(request(changed,changed.signed_data.data))).authority_snapshot_commitment()!=*baseline.authority_snapshot_commitment());
    auto changed_anchor=make_fixture();changed_anchor.anchor[0]^=1;bind(changed_anchor);authorize(changed_anchor);halofpx::context_store_bootstrap_authority changed_anchor_authority(changed_anchor.config);
    const auto changed_anchor_plan=plan(changed_anchor_authority.plan(request(changed_anchor,changed_anchor.signed_data.data)));
    assert(*changed_anchor_plan.authority_snapshot_commitment()!=*baseline.authority_snapshot_commitment());
    assert(*changed_anchor_plan.anchor()->envelope_digest()!=*baseline.anchor()->envelope_digest());
    auto changed_admin=make_fixture();changed_admin.admin[0]^=1;bind(changed_admin);authorize(changed_admin);halofpx::context_store_bootstrap_authority changed_admin_authority(changed_admin.config);
    const auto changed_admin_plan=plan(changed_admin_authority.plan(request(changed_admin,changed_admin.signed_data.data)));
    assert(*changed_admin_plan.authority_snapshot_commitment()!=*baseline.authority_snapshot_commitment());
    assert(*changed_admin_authority.authority_scope_commitment()==*authority.authority_scope_commitment());
    assert(*changed_admin_plan.anchor()->envelope_digest()==*baseline.anchor()->envelope_digest());
    auto changed_registry=make_fixture();changed_registry.registry[0]^=1;bind(changed_registry);seal_registry(changed_registry);authorize(changed_registry);halofpx::context_store_bootstrap_authority changed_registry_authority(changed_registry.config);
    const auto changed_registry_plan=plan(changed_registry_authority.plan(request(changed_registry,changed_registry.signed_data.data)));
    const auto original_registry=halofpx::context_store_verify_protected_registry_v1(f.registry_snapshot.data(),f.registry_snapshot.size(),f.config.protected_registry_authentication_key);
    const auto replacement_registry=halofpx::context_store_verify_protected_registry_v1(changed_registry.registry_snapshot.data(),changed_registry.registry_snapshot.size(),changed_registry.config.protected_registry_authentication_key);
    assert(*original_registry.authenticated_carrier()->authority_binding()!=*replacement_registry.authenticated_carrier()->authority_binding());
    assert(*changed_registry_plan.authority_snapshot_commitment()!=*baseline.authority_snapshot_commitment());
    auto changed_epoch=make_fixture();++changed_epoch.config.authority_epoch;seal_registry(changed_epoch);authorize(changed_epoch);halofpx::context_store_bootstrap_authority changed_epoch_authority(changed_epoch.config);
    const auto changed_epoch_plan=plan(changed_epoch_authority.plan(request(changed_epoch,changed_epoch.signed_data.data)));
    assert(*changed_epoch_plan.authority_snapshot_commitment()!=*baseline.authority_snapshot_commitment());
    assert(*changed_epoch_plan.anchor()->envelope_digest()!=*baseline.anchor()->envelope_digest());
    std::array<digest,32> values{};std::vector<std::thread> threads;for(size_t i=0;i<values.size();++i)threads.emplace_back([&,i]{values[i]=*plan(authority.plan(req)).plan_commitment();});
    for(auto & thread:threads)thread.join();for(const auto & value:values)assert(value==*baseline.plan_commitment());
    assert(std::string(halofpx::context_store_bootstrap_status_name(halofpx::context_store_bootstrap_status::manifest_rejected))=="manifest-rejected");
}

void test_authenticated_token_semantic_rejections() {
    auto base=make_fixture();halofpx::context_store_bootstrap_authority authority(base.config);
    auto reject=[&](auto mutate){auto candidate=base;bind(candidate);auto body=token_body(candidate);mutate(body);encode_token(candidate,body);
        auto result=authority.plan(request(candidate,candidate.signed_data.data));assert(result.status==halofpx::context_store_bootstrap_status::authorization_rejected);rejected(result);};
    reject([](auto & b){b.store_uuid[0]^=1;});reject([](auto & b){b.namespace_id[0]^=1;});reject([](auto & b){++b.policy_epoch;});
    reject([](auto & b){b.checkpoint_lineage_id[0]^=1;});reject([](auto & b){b.manifest_key_id=rid("other-manifest");});
    reject([](auto & b){++b.manifest_key_generation;});reject([](auto & b){b.compatibility_root[0]^=1;});reject([](auto & b){++b.authority_epoch;});
    reject([](auto & b){b.anchor_key_id=rid("other-anchor");});reject([](auto & b){++b.anchor_key_generation;});
    reject([](auto & b){b.authority_scope_commitment[0]^=1;});reject([](auto & b){b.protected_registry_id=rid("other-registry");});
    reject([](auto & b){++b.protected_registry_epoch;});reject([](auto & b){b.protected_registry_snapshot_digest[0]^=1;});
    reject([](auto & b){b.protected_registry_policy_digest[0]^=1;});reject([](auto & b){++b.authorization_sequence;});
    auto wrong_manifest=base;bind(wrong_manifest);auto body=token_body(wrong_manifest);body.selected_manifest_digest[0]^=1;encode_token(wrong_manifest,body);
    auto result=authority.plan(request(wrong_manifest,wrong_manifest.signed_data.data));assert(result.status==halofpx::context_store_bootstrap_status::manifest_rejected);rejected(result);
}

digest independent_reconciliation_commitment(const halofpx::context_store_bootstrap_reconciliation_operation & op) {
    bytes body;auto add=[&](const auto & value){body.insert(body.end(),value.begin(),value.end());};auto length=[&](size_t value){for(int shift=56;shift>=0;shift-=8)body.push_back(static_cast<uint8_t>(static_cast<uint64_t>(value)>>shift));};
    add(op.root_identity);add(op.reconciliation_attempt_id);add(op.original_attempt_id);add(op.original_operation_commitment);
    length(op.predecessor.envelope_size());body.insert(body.end(),op.predecessor.envelope_data(),op.predecessor.envelope_data()+op.predecessor.envelope_size());
    length(op.successor.envelope_size());body.insert(body.end(),op.successor.envelope_data(),op.successor.envelope_data()+op.successor.envelope_size());
    length(op.proposed_anchor.envelope_size());body.insert(body.end(),op.proposed_anchor.envelope_data(),op.proposed_anchor.envelope_data()+op.proposed_anchor.envelope_size());
    return sha(domain("halofpx.bootstrap-consumption-reconciliation.v1",body));
}

class consumption_backend final : public halofpx::context_store_bootstrap_consumption_backend {
public:
    consumption_backend(const digest & root, halofpx::context_store_bootstrap_backend_outcome result)
        : context_store_bootstrap_consumption_backend(root), result_(result) {}
    halofpx::context_store_bootstrap_backend_outcome result_;
    bool throws_=false,bad_positive_witness_=false,stateful_=false,has_current_=false,check_quarantine_=false;
    halofpx::context_store_bootstrap_reconciliation_backend_outcome reconciliation_outcome_=halofpx::context_store_bootstrap_reconciliation_backend_outcome::authoritative_present;
    int reconciliation_witness_=0; // successor, predecessor, other, empty
    int bad_reconciliation_echo_=0;
    bool reconciliation_throws_=false,oversized_reconciliation_witness_=false,truncate_reconciliation_witness_=false,force_reconciliation_bytes_=false,reconciliation_check_quarantine_=false;
    size_t reconciliation_calls_=0;
    size_t calls_=0;
    halofpx::context_store_authenticated_protected_registry_successor current_;
protected:
    halofpx::context_store_bootstrap_backend_result compare_and_advance(const halofpx::context_store_bootstrap_consumption_operation & op) override {
        ++calls_;assert(op.predecessor.envelope_size()>0&&op.successor.envelope_size()>0);if(check_quarantine_)assert(!quarantined());if(throws_)throw 1;
        halofpx::context_store_bootstrap_backend_result out;
        if(stateful_){if(!has_current_){current_=op.successor;has_current_=true;out.outcome=halofpx::context_store_bootstrap_backend_outcome::advanced_durable;out.observed_current=current_;return out;}
            const bool exact=current_.envelope_size()==op.successor.envelope_size()&&current_.envelope_digest()&&op.successor.envelope_digest()&&*current_.envelope_digest()==*op.successor.envelope_digest()&&std::equal(current_.envelope_data(),current_.envelope_data()+current_.envelope_size(),op.successor.envelope_data());
            if(exact){out.outcome=halofpx::context_store_bootstrap_backend_outcome::already_same_durable;out.observed_current=current_;return out;}out.outcome=halofpx::context_store_bootstrap_backend_outcome::conflict;return out;}
        out.outcome=result_;if(!bad_positive_witness_&&(result_==halofpx::context_store_bootstrap_backend_outcome::advanced_durable||result_==halofpx::context_store_bootstrap_backend_outcome::already_same_durable))out.observed_current=op.successor;return out;
    }
    halofpx::context_store_bootstrap_reconciliation_backend_result fence_original_and_read_current(const halofpx::context_store_bootstrap_reconciliation_operation & op) override {
        ++reconciliation_calls_;assert(independent_reconciliation_commitment(op)==op.reconciliation_commitment);if(reconciliation_check_quarantine_)assert(quarantined());if(reconciliation_throws_)throw 2;halofpx::context_store_bootstrap_reconciliation_backend_result out;out.outcome=reconciliation_outcome_;out.root_identity=op.root_identity;out.reconciliation_attempt_id=op.reconciliation_attempt_id;out.reconciliation_commitment=op.reconciliation_commitment;out.original_attempt_id=op.original_attempt_id;out.original_operation_commitment=op.original_operation_commitment;
        if(bad_reconciliation_echo_==1)out.root_identity[0]^=1;else if(bad_reconciliation_echo_==2)out.reconciliation_attempt_id[0]^=1;else if(bad_reconciliation_echo_==3)out.reconciliation_commitment[0]^=1;else if(bad_reconciliation_echo_==4)out.original_attempt_id[0]^=1;else if(bad_reconciliation_echo_==5)out.original_operation_commitment[0]^=1;
        const uint8_t*data=nullptr;size_t size=0;if(reconciliation_witness_==0){data=op.successor.envelope_data();size=op.successor.envelope_size();}else if(reconciliation_witness_==1){data=op.predecessor.envelope_data();size=op.predecessor.envelope_size();}else if(reconciliation_witness_==2){data=op.successor.envelope_data();size=op.successor.envelope_size();}
        if((out.outcome==halofpx::context_store_bootstrap_reconciliation_backend_outcome::authoritative_present||force_reconciliation_bytes_)&&data&&size){std::copy(data,data+size,out.observed_current.begin());out.observed_current_size=oversized_reconciliation_witness_?out.observed_current.size()+1:(truncate_reconciliation_witness_?size-1:size);if(reconciliation_witness_==2)out.observed_current[0]^=1;}
        return out;
    }
};

halofpx::context_store_bootstrap_consumption_request consumption_request(fixture & f,const halofpx::context_store_bootstrap_plan & p,uint8_t attempt) {
    halofpx::context_store_bootstrap_consumption_request r;r.attempt_id.fill(attempt);r.plan=p;r.registry_authentication_key=f.config.protected_registry_authentication_key;
    auto verified=halofpx::context_store_verify_protected_registry_v1(f.registry_snapshot.data(),f.registry_snapshot.size(),f.config.protected_registry_authentication_key);
    assert(verified.authenticated_carrier());r.predecessor=*verified.authenticated_carrier();return r;
}

void test_consumption_coordinator_backend_fencing() {
    static_assert(!std::is_copy_constructible_v<halofpx::context_store_bootstrap_consumption_proof>);
    auto f=make_fixture();halofpx::context_store_bootstrap_authority authority(f.config);auto p=plan(authority.plan(request(f,f.signed_data.data)));
    halofpx::context_store_bootstrap_consumption_result forged;forged.status=halofpx::context_store_bootstrap_consumption_status::advanced_unexecuted;assert(!forged.has_proof());
    assert(p.expected_registry_snapshot_digest());digest root{};root.fill(0x91);consumption_backend backend(root,halofpx::context_store_bootstrap_backend_outcome::advanced_durable);
    halofpx::context_store_bootstrap_consumption_coordinator first(backend);auto req=consumption_request(f,p,1);auto advanced=first.consume(req);assert(advanced.status==halofpx::context_store_bootstrap_consumption_status::advanced_unexecuted&&advanced.has_proof()&&backend.calls_==1);
    auto moved=std::move(advanced.proof);assert(moved.valid()&&!advanced.proof.valid()&&moved.predecessor()&&moved.successor()&&moved.proposed_anchor()&&moved.root_identity()&&moved.attempt_id()&&moved.operation_commitment()&&moved.command_id()&&moved.authorization_token_digest()&&moved.plan_commitment()&&moved.authority_snapshot_commitment()&&moved.selected_manifest_digest()&&moved.authorization_sequence()==p.authorization_sequence()&&moved.classified_outcome()==halofpx::context_store_bootstrap_backend_outcome::advanced_durable);
    halofpx::context_store_bootstrap_consumption_coordinator second(backend);auto replay=second.consume(req);assert(replay.status==halofpx::context_store_bootstrap_consumption_status::attempt_replayed&&!replay.has_proof()&&backend.calls_==1);
    backend.result_=halofpx::context_store_bootstrap_backend_outcome::already_same_durable;auto retry=consumption_request(f,p,2);auto same=second.consume(retry);assert(same.status==halofpx::context_store_bootstrap_consumption_status::already_consumed_same_unexecuted&&same.has_proof());
    backend.result_=halofpx::context_store_bootstrap_backend_outcome::conflict;auto conflict=second.consume(consumption_request(f,p,3));assert(conflict.status==halofpx::context_store_bootstrap_consumption_status::conflict&&!conflict.has_proof());
    backend.result_=halofpx::context_store_bootstrap_backend_outcome::uncertain;auto uncertain=second.consume(consumption_request(f,p,4));assert(uncertain.status==halofpx::context_store_bootstrap_consumption_status::visibility_uncertain&&backend.quarantined());auto after=first.consume(consumption_request(f,p,5));assert(after.status==halofpx::context_store_bootstrap_consumption_status::root_quarantined);
    consumption_backend fresh(root,halofpx::context_store_bootstrap_backend_outcome::advanced_durable);halofpx::context_store_bootstrap_consumption_coordinator nonpersistent_limit(fresh);assert(nonpersistent_limit.consume(consumption_request(f,p,6)).has_proof());
    consumption_backend exception(root,halofpx::context_store_bootstrap_backend_outcome::advanced_durable);exception.throws_=true;halofpx::context_store_bootstrap_consumption_coordinator exception_coordinator(exception);assert(exception_coordinator.consume(consumption_request(f,p,7)).status==halofpx::context_store_bootstrap_consumption_status::visibility_uncertain&&exception.quarantined());
    consumption_backend late(root,halofpx::context_store_bootstrap_backend_outcome::late_completion_risk);halofpx::context_store_bootstrap_consumption_coordinator late_coordinator(late);assert(late_coordinator.consume(consumption_request(f,p,10)).status==halofpx::context_store_bootstrap_consumption_status::visibility_uncertain&&late.quarantined());
    consumption_backend malformed(root,halofpx::context_store_bootstrap_backend_outcome::malformed_response);halofpx::context_store_bootstrap_consumption_coordinator malformed_coordinator(malformed);assert(malformed_coordinator.consume(consumption_request(f,p,11)).status==halofpx::context_store_bootstrap_consumption_status::visibility_uncertain&&malformed.quarantined());
    consumption_backend missing_witness(root,halofpx::context_store_bootstrap_backend_outcome::advanced_durable);missing_witness.bad_positive_witness_=true;halofpx::context_store_bootstrap_consumption_coordinator witness_coordinator(missing_witness);assert(witness_coordinator.consume(consumption_request(f,p,16)).status==halofpx::context_store_bootstrap_consumption_status::visibility_uncertain&&missing_witness.quarantined());
    consumption_backend reentrant_status(root,halofpx::context_store_bootstrap_backend_outcome::advanced_durable);reentrant_status.check_quarantine_=true;halofpx::context_store_bootstrap_consumption_coordinator reentrant_coordinator(reentrant_status);assert(reentrant_coordinator.consume(consumption_request(f,p,17)).has_proof());
    auto bad=consumption_request(f,p,8);bad.attempt_id.fill(0);assert(nonpersistent_limit.consume(bad).status==halofpx::context_store_bootstrap_consumption_status::invalid_request);
    bad=consumption_request(f,p,9);bad.registry_authentication_key.master_key={};assert(nonpersistent_limit.consume(bad).status==halofpx::context_store_bootstrap_consumption_status::invalid_request);
    auto wrong_secret=consumption_request(f,p,12);auto other_secret=f.registry;other_secret[0]^=1;wrong_secret.registry_authentication_key.master_key={other_secret.data(),other_secret.size()};assert(nonpersistent_limit.consume(wrong_secret).status==halofpx::context_store_bootstrap_consumption_status::invalid_request);
    auto different=f;bind(different);seal_registry(different,39);auto wrong_predecessor=consumption_request(different,p,13);assert(nonpersistent_limit.consume(wrong_predecessor).status==halofpx::context_store_bootstrap_consumption_status::invalid_request);
    auto f2=make_fixture();authorize(f2,92);auto p2=plan(authority.plan(request(f2,f2.signed_data.data)));consumption_backend racing(root,halofpx::context_store_bootstrap_backend_outcome::advanced_durable);racing.stateful_=true;halofpx::context_store_bootstrap_consumption_coordinator racer_a(racing),racer_b(racing);auto race_a=consumption_request(f,p,14),race_b=consumption_request(f2,p2,15);std::array<halofpx::context_store_bootstrap_consumption_status,2> race_status{};
    std::thread ta([&]{race_status[0]=racer_a.consume(race_a).status;}),tb([&]{race_status[1]=racer_b.consume(race_b).status;});ta.join();tb.join();
    assert((race_status[0]==halofpx::context_store_bootstrap_consumption_status::advanced_unexecuted&&race_status[1]==halofpx::context_store_bootstrap_consumption_status::conflict)||(race_status[1]==halofpx::context_store_bootstrap_consumption_status::advanced_unexecuted&&race_status[0]==halofpx::context_store_bootstrap_consumption_status::conflict));assert(racing.calls_==2);
    auto same_retry=race_status[0]==halofpx::context_store_bootstrap_consumption_status::advanced_unexecuted?consumption_request(f,p,18):consumption_request(f2,p2,18);assert(racer_a.consume(same_retry).status==halofpx::context_store_bootstrap_consumption_status::already_consumed_same_unexecuted&&racing.calls_==3);
}

void test_consumption_reconciliation_fencing() {
    static_assert(!std::is_copy_constructible_v<halofpx::context_store_bootstrap_recovered_consumption_proof>);
    auto f=make_fixture();halofpx::context_store_bootstrap_authority authority(f.config);auto p=plan(authority.plan(request(f,f.signed_data.data)));digest root{};root.fill(0x73);
    auto rr=[&](uint8_t original,uint8_t fresh){halofpx::context_store_bootstrap_reconciliation_request r;r.original_attempt_id.fill(original);r.reconciliation_attempt_id.fill(fresh);r.plan=p;r.registry_authentication_key=f.config.protected_registry_authentication_key;auto verified=halofpx::context_store_verify_protected_registry_v1(f.registry_snapshot.data(),f.registry_snapshot.size(),f.config.protected_registry_authentication_key);r.predecessor=*verified.authenticated_carrier();return r;};
    consumption_backend never(root,halofpx::context_store_bootstrap_backend_outcome::definitely_not_applied);halofpx::context_store_bootstrap_reconciliation_coordinator never_rc(never);assert(never_rc.reconcile(rr(1,2)).status==halofpx::context_store_bootstrap_reconciliation_status::not_reconcilable&&never.reconciliation_calls_==0);
    consumption_backend recovered(root,halofpx::context_store_bootstrap_backend_outcome::uncertain);halofpx::context_store_bootstrap_consumption_coordinator cc(recovered);assert(cc.consume(consumption_request(f,p,3)).status==halofpx::context_store_bootstrap_consumption_status::visibility_uncertain&&recovered.quarantined());halofpx::context_store_bootstrap_reconciliation_coordinator rc(recovered);auto result=rc.reconcile(rr(3,4));assert(result.status==halofpx::context_store_bootstrap_reconciliation_status::consumed_same_recovered_unexecuted&&result.has_proof()&&recovered.reconciliation_calls_==1);auto proof=std::move(result.proof);assert(proof.valid()&&!result.proof.valid()&&proof.predecessor()&&proof.successor()&&proof.proposed_anchor()&&proof.root_identity()&&proof.original_attempt_id()&&proof.original_operation_commitment()&&proof.reconciliation_attempt_id()&&proof.reconciliation_commitment()&&proof.command_id()&&proof.authorization_token_digest()&&proof.plan_commitment()&&proof.authority_snapshot_commitment()&&proof.selected_manifest_digest()&&proof.authorization_sequence()==p.authorization_sequence()&&proof.original_consumption_uncertain_confirmed()&&proof.observed_successor_size()==proof.successor()->envelope_size()&&std::equal(proof.observed_successor_data(),proof.observed_successor_data()+proof.observed_successor_size(),proof.successor()->envelope_data()));auto replay=rc.reconcile(rr(3,4));assert(replay.status==halofpx::context_store_bootstrap_reconciliation_status::attempt_replayed&&!replay.has_proof()&&recovered.reconciliation_calls_==1);assert(rc.reconcile(rr(3,5)).status==halofpx::context_store_bootstrap_reconciliation_status::not_reconcilable&&recovered.reconciliation_calls_==1);
    consumption_backend predecessor(root,halofpx::context_store_bootstrap_backend_outcome::late_completion_risk);predecessor.reconciliation_witness_=1;halofpx::context_store_bootstrap_consumption_coordinator pc(predecessor);assert(pc.consume(consumption_request(f,p,6)).status==halofpx::context_store_bootstrap_consumption_status::visibility_uncertain);halofpx::context_store_bootstrap_reconciliation_coordinator prc(predecessor);auto pred=prc.reconcile(rr(6,7));assert(pred.status==halofpx::context_store_bootstrap_reconciliation_status::definitely_unconsumed_fenced_no_retry&&!pred.has_proof());assert(prc.reconcile(rr(6,8)).status==halofpx::context_store_bootstrap_reconciliation_status::not_reconcilable&&predecessor.reconciliation_calls_==1);
    consumption_backend other(root,halofpx::context_store_bootstrap_backend_outcome::uncertain);other.reconciliation_witness_=2;halofpx::context_store_bootstrap_consumption_coordinator oc(other);oc.consume(consumption_request(f,p,9));halofpx::context_store_bootstrap_reconciliation_coordinator orc(other);assert(orc.reconcile(rr(9,10)).status==halofpx::context_store_bootstrap_reconciliation_status::conflict&&!orc.reconcile(rr(9,11)).has_proof());
    consumption_backend absent(root,halofpx::context_store_bootstrap_backend_outcome::uncertain);absent.reconciliation_outcome_=halofpx::context_store_bootstrap_reconciliation_backend_outcome::authoritative_absent;absent.reconciliation_witness_=3;halofpx::context_store_bootstrap_consumption_coordinator ac(absent);ac.consume(consumption_request(f,p,12));halofpx::context_store_bootstrap_reconciliation_coordinator arc(absent);assert(arc.reconcile(rr(12,13)).status==halofpx::context_store_bootstrap_reconciliation_status::visibility_uncertain&&absent.quarantined());
    for(int echo=1;echo<=5;++echo){consumption_backend crossed(root,halofpx::context_store_bootstrap_backend_outcome::uncertain);crossed.bad_reconciliation_echo_=echo;halofpx::context_store_bootstrap_consumption_coordinator xc(crossed);xc.consume(consumption_request(f,p,14));halofpx::context_store_bootstrap_reconciliation_coordinator xrc(crossed);assert(xrc.reconcile(rr(14,15)).status==halofpx::context_store_bootstrap_reconciliation_status::visibility_uncertain&&crossed.quarantined()&&crossed.reconciliation_calls_==1);}
    consumption_backend missing(root,halofpx::context_store_bootstrap_backend_outcome::uncertain);missing.reconciliation_witness_=3;halofpx::context_store_bootstrap_consumption_coordinator mc(missing);mc.consume(consumption_request(f,p,21));halofpx::context_store_bootstrap_reconciliation_coordinator mrc(missing);assert(mrc.reconcile(rr(21,22)).status==halofpx::context_store_bootstrap_reconciliation_status::visibility_uncertain&&missing.quarantined());
    consumption_backend oversized(root,halofpx::context_store_bootstrap_backend_outcome::uncertain);oversized.oversized_reconciliation_witness_=true;halofpx::context_store_bootstrap_consumption_coordinator zc(oversized);zc.consume(consumption_request(f,p,23));halofpx::context_store_bootstrap_reconciliation_coordinator zrc(oversized);assert(zrc.reconcile(rr(23,24)).status==halofpx::context_store_bootstrap_reconciliation_status::visibility_uncertain&&oversized.quarantined());
    consumption_backend thrown(root,halofpx::context_store_bootstrap_backend_outcome::uncertain);thrown.reconciliation_throws_=true;halofpx::context_store_bootstrap_consumption_coordinator tc(thrown);tc.consume(consumption_request(f,p,25));halofpx::context_store_bootstrap_reconciliation_coordinator trc(thrown);assert(trc.reconcile(rr(25,26)).status==halofpx::context_store_bootstrap_reconciliation_status::visibility_uncertain&&thrown.quarantined());
    consumption_backend differently_bound(root,halofpx::context_store_bootstrap_backend_outcome::uncertain);halofpx::context_store_bootstrap_consumption_coordinator dc(differently_bound);dc.consume(consumption_request(f,p,16));halofpx::context_store_bootstrap_reconciliation_coordinator drc(differently_bound);assert(drc.reconcile(rr(17,18)).status==halofpx::context_store_bootstrap_reconciliation_status::not_reconcilable&&differently_bound.reconciliation_calls_==0);
    consumption_backend definite(root,halofpx::context_store_bootstrap_backend_outcome::definitely_not_applied);halofpx::context_store_bootstrap_consumption_coordinator fc(definite);assert(fc.consume(consumption_request(f,p,19)).status==halofpx::context_store_bootstrap_consumption_status::definitely_not_applied);halofpx::context_store_bootstrap_reconciliation_coordinator frc(definite);assert(frc.reconcile(rr(19,20)).status==halofpx::context_store_bootstrap_reconciliation_status::not_reconcilable&&definite.reconciliation_calls_==0);
    consumption_backend concurrent(root,halofpx::context_store_bootstrap_backend_outcome::uncertain);concurrent.reconciliation_check_quarantine_=true;halofpx::context_store_bootstrap_consumption_coordinator conc(concurrent);conc.consume(consumption_request(f,p,27));halofpx::context_store_bootstrap_reconciliation_coordinator cr1(concurrent),cr2(concurrent);std::array<halofpx::context_store_bootstrap_reconciliation_status,2> crs{};auto crr1=rr(27,28),crr2=rr(27,29);std::thread ct1([&]{crs[0]=cr1.reconcile(crr1).status;}),ct2([&]{crs[1]=cr2.reconcile(crr2).status;});ct1.join();ct2.join();assert(((crs[0]==halofpx::context_store_bootstrap_reconciliation_status::consumed_same_recovered_unexecuted)&&(crs[1]==halofpx::context_store_bootstrap_reconciliation_status::not_reconcilable)||((crs[1]==halofpx::context_store_bootstrap_reconciliation_status::consumed_same_recovered_unexecuted)&&(crs[0]==halofpx::context_store_bootstrap_reconciliation_status::not_reconcilable)))&&concurrent.reconciliation_calls_==1);
    consumption_backend truncated(root,halofpx::context_store_bootstrap_backend_outcome::uncertain);truncated.truncate_reconciliation_witness_=true;halofpx::context_store_bootstrap_consumption_coordinator trc0(truncated);trc0.consume(consumption_request(f,p,30));halofpx::context_store_bootstrap_reconciliation_coordinator trc1(truncated);assert(trc1.reconcile(rr(30,31)).status==halofpx::context_store_bootstrap_reconciliation_status::conflict);
    const std::array nonpresent={halofpx::context_store_bootstrap_reconciliation_backend_outcome::authoritative_absent,halofpx::context_store_bootstrap_reconciliation_backend_outcome::unreadable,halofpx::context_store_bootstrap_reconciliation_backend_outcome::incomplete,halofpx::context_store_bootstrap_reconciliation_backend_outcome::unconfirmed_fence,halofpx::context_store_bootstrap_reconciliation_backend_outcome::malformed_response,halofpx::context_store_bootstrap_reconciliation_backend_outcome::uncertain,halofpx::context_store_bootstrap_reconciliation_backend_outcome::late_completion_risk};
    for(size_t i=0;i<nonpresent.size();++i){consumption_backend np(root,halofpx::context_store_bootstrap_backend_outcome::uncertain);np.reconciliation_outcome_=nonpresent[i];np.reconciliation_witness_=3;halofpx::context_store_bootstrap_consumption_coordinator npc(np);const uint8_t original=(uint8_t)(40+i*2),fresh=(uint8_t)(41+i*2);npc.consume(consumption_request(f,p,original));halofpx::context_store_bootstrap_reconciliation_coordinator npr(np);assert(npr.reconcile(rr(original,fresh)).status==halofpx::context_store_bootstrap_reconciliation_status::visibility_uncertain);halofpx::context_store_bootstrap_reconciliation_coordinator again(np);assert(again.reconcile(rr(original,(uint8_t)(fresh+40))).status==halofpx::context_store_bootstrap_reconciliation_status::root_quarantined&&np.reconciliation_calls_==1);}
    consumption_backend contradiction(root,halofpx::context_store_bootstrap_backend_outcome::uncertain);contradiction.reconciliation_outcome_=halofpx::context_store_bootstrap_reconciliation_backend_outcome::unreadable;contradiction.force_reconciliation_bytes_=true;halofpx::context_store_bootstrap_consumption_coordinator cnc(contradiction);cnc.consume(consumption_request(f,p,60));halofpx::context_store_bootstrap_reconciliation_coordinator cnr(contradiction);assert(cnr.reconcile(rr(60,61)).status==halofpx::context_store_bootstrap_reconciliation_status::visibility_uncertain&&contradiction.reconciliation_calls_==1);
    consumption_backend wrong_inputs(root,halofpx::context_store_bootstrap_backend_outcome::uncertain);halofpx::context_store_bootstrap_consumption_coordinator wic(wrong_inputs);wic.consume(consumption_request(f,p,62));halofpx::context_store_bootstrap_reconciliation_coordinator wir(wrong_inputs);auto wrong_key=rr(62,63);auto changed_secret=f.registry;changed_secret[0]^=1;wrong_key.registry_authentication_key.master_key={changed_secret.data(),changed_secret.size()};assert(wir.reconcile(wrong_key).status==halofpx::context_store_bootstrap_reconciliation_status::invalid_request&&wrong_inputs.reconciliation_calls_==0);auto wrong_pred=rr(62,64);auto different=f;bind(different);seal_registry(different,0);auto dv=halofpx::context_store_verify_protected_registry_v1(different.registry_snapshot.data(),different.registry_snapshot.size(),different.config.protected_registry_authentication_key);wrong_pred.predecessor=*dv.authenticated_carrier();assert(wir.reconcile(wrong_pred).status==halofpx::context_store_bootstrap_reconciliation_status::invalid_request&&wrong_inputs.reconciliation_calls_==0);auto different_command=f;authorize(different_command,92);auto p2=plan(authority.plan(request(different_command,different_command.signed_data.data)));auto wrong_plan=rr(62,65);wrong_plan.plan=p2;assert(wir.reconcile(wrong_plan).status==halofpx::context_store_bootstrap_reconciliation_status::not_reconcilable&&wrong_inputs.reconciliation_calls_==0);auto different_manifest=f;manifest_options two_objects;two_objects.objects=2;different_manifest.signed_data=make_manifest(two_objects);different_manifest.manifest.fill(0x33);bind(different_manifest);authorize(different_manifest);auto p3=plan(authority.plan(request(different_manifest,different_manifest.signed_data.data)));auto wrong_material=rr(62,66);wrong_material.plan=p3;assert(wir.reconcile(wrong_material).status==halofpx::context_store_bootstrap_reconciliation_status::not_reconcilable&&wrong_inputs.reconciliation_calls_==0);
}

bytes object_frame(const bytes & payload) {
    bytes out={0x48,0x41,0x4c,0x4f,0x4f,0x42,0x4a,0x01};
    const std::string object_domain="halofpx.object.v1",type="tokens";
    out.push_back(0);out.push_back(static_cast<uint8_t>(object_domain.size()));out.insert(out.end(),object_domain.begin(),object_domain.end());
    out.push_back(0);out.push_back(static_cast<uint8_t>(type.size()));out.insert(out.end(),type.begin(),type.end());
    for(int shift=56;shift>=0;shift-=8)out.push_back(static_cast<uint8_t>(static_cast<uint64_t>(payload.size())>>shift));
    out.insert(out.end(),payload.begin(),payload.end());return out;
}

halofpx::context_store_bootstrap_material_synthetic_policy material_policy(const fixture & f,const digest & registry_root) {
    halofpx::context_store_bootstrap_material_synthetic_policy p;p.material_root_identity.fill(0x95);p.registry_root_identity=registry_root;p.store_uuid=f.config.store_uuid;p.namespace_id=f.config.namespace_id;p.checkpoint_lineage_id=f.config.checkpoint_lineage_id;p.policy_epoch=f.config.policy_epoch;p.manifest_key_generation=f.config.manifest_key_generation;p.writer_authority_epoch=f.config.authority_epoch;p.durability_policy_id=rid("synthetic-durable-v1");p.manifest_durability_mode=0;p.maximum_source_object_count=128;p.maximum_frame_bytes=16777216;p.maximum_aggregate_frame_bytes=67108864;return p;
}

digest independent_material_close(const halofpx::context_store_bootstrap_material_synthetic_operation & op) {
    bytes body;auto add=[&](const auto & value){body.insert(body.end(),value.begin(),value.end());};add(op.material_root_identity);add(op.registry_root_identity);add(op.material_attempt_id);add(op.operation_commitment);add(op.source_set_commitment);add(op.material_set_commitment);add(op.selected_manifest_digest);body.push_back(1);return sha(domain("halofpx.bootstrap-material-durable-close.v1",body));
}
void be64(bytes & out,uint64_t v){for(int s=56;s>=0;s-=8)out.push_back(static_cast<uint8_t>(v>>s));}
digest independent_source_set(const std::vector<bytes>&fs){bytes b;be64(b,fs.size());for(size_t i=0;i<fs.size();++i){be64(b,i);be64(b,fs[i].size());b.insert(b.end(),fs[i].begin(),fs[i].end());}return sha(domain("halofpx.bootstrap-material-source-set.v1",b));}
digest independent_root_policy(const halofpx::context_store_bootstrap_material_synthetic_policy&p,const digest&ss){bytes b;auto add=[&](const auto&v){b.insert(b.end(),v.begin(),v.end());};add(p.material_root_identity);add(p.registry_root_identity);add(p.store_uuid);add(p.namespace_id);add(p.checkpoint_lineage_id);be64(b,p.policy_epoch);be64(b,p.manifest_key_generation);be64(b,p.writer_authority_epoch);be64(b,p.durability_policy_id.size);b.insert(b.end(),p.durability_policy_id.bytes.begin(),p.durability_policy_id.bytes.begin()+p.durability_policy_id.size);b.push_back(p.manifest_durability_mode);be64(b,p.maximum_source_object_count);be64(b,p.maximum_frame_bytes);be64(b,p.maximum_aggregate_frame_bytes);add(ss);return sha(domain("halofpx.bootstrap-material-root-policy.v1",b));}
digest independent_authority_source(const halofpx::context_store_bootstrap_material_synthetic_operation&o){bytes b;auto add=[&](const auto&v){b.insert(b.end(),v.begin(),v.end());};be64(b,o.provenance_tag_size);b.insert(b.end(),o.provenance_tag.begin(),o.provenance_tag.begin()+o.provenance_tag_size);add(o.registry_root_identity);add(o.original_consumption_attempt);add(o.original_consumption_operation);be64(b,o.authenticated_successor.envelope_size());b.insert(b.end(),o.authenticated_successor.envelope_data(),o.authenticated_successor.envelope_data()+o.authenticated_successor.envelope_size());be64(b,o.authenticated_proposed_anchor.envelope_size());b.insert(b.end(),o.authenticated_proposed_anchor.envelope_data(),o.authenticated_proposed_anchor.envelope_data()+o.authenticated_proposed_anchor.envelope_size());be64(b,o.authorization_sequence);add(o.command_id);add(o.authorization_token_digest);add(o.plan_commitment);add(o.authority_snapshot_commitment);add(o.selected_manifest_digest);if(o.reconciled_source){add(o.reconciliation_attempt);add(o.reconciliation_commitment);be64(b,o.observed_successor.size());b.insert(b.end(),o.observed_successor.begin(),o.observed_successor.end());}return sha(domain("halofpx.bootstrap-material-authority-source.v1",b));}
digest independent_material_set(const halofpx::context_store_bootstrap_material_synthetic_operation&o){auto p=halofpx::context_store_parse_manifest_v1(o.manifest_envelope.data(),o.manifest_envelope.size());assert(p.status==halofpx::context_store_manifest_parse_status::structural_only);bytes b;be64(b,o.manifest_envelope.size());b.insert(b.end(),o.manifest_envelope.begin(),o.manifest_envelope.end());be64(b,p.manifest.object_count);for(size_t i=0;i<p.manifest.object_count;++i){const auto&r=p.manifest.object_references[i];be64(b,i);b.insert(b.end(),r.object_id.begin(),r.object_id.end());be64(b,r.stream_type.size);b.insert(b.end(),r.stream_type.bytes.begin(),r.stream_type.bytes.begin()+r.stream_type.size);be64(b,r.frame_bytes);}return sha(domain("halofpx.bootstrap-material-set.v1",b));}
digest independent_material_operation(const halofpx::context_store_bootstrap_material_synthetic_operation&o){bytes b;auto add=[&](const auto&v){b.insert(b.end(),v.begin(),v.end());};add(o.material_root_identity);add(o.registry_root_identity);add(o.material_attempt_id);add(o.root_policy_commitment);add(o.authority_source_commitment);add(o.source_set_commitment);add(o.material_set_commitment);add(o.selected_manifest_digest);add(o.proposed_anchor_envelope_digest);return sha(domain("halofpx.bootstrap-material-preparation.v1",b));}

class material_backend final : public halofpx::context_store_bootstrap_material_synthetic_backend {
public:
    material_backend(const halofpx::context_store_bootstrap_material_synthetic_policy & p,std::vector<std::vector<uint8_t>> frames)
        : context_store_bootstrap_material_synthetic_backend(p,std::move(frames)) {}
    halofpx::context_store_bootstrap_material_synthetic_backend_outcome outcome_=halofpx::context_store_bootstrap_material_synthetic_backend_outcome::prepared_backend_claim;
    int mutation_=0,failpoint_=-1;bool throws_=false,reentrant_=false;size_t calls_=0;
    std::atomic<int> active_{0},max_active_{0};bool overlap_=false;
protected:
    halofpx::context_store_bootstrap_material_synthetic_witness prepare_exact_material_set_and_durable_close(const halofpx::context_store_bootstrap_material_synthetic_operation & op) override {
        const int active=++active_;int prior=max_active_.load();while(prior<active&&!max_active_.compare_exchange_weak(prior,active)){};if(overlap_)for(size_t spin=0;spin<10000;++spin)std::this_thread::yield();
        std::vector<bytes> fs;for(size_t i=0;i<source_frame_count();++i)fs.emplace_back(source_frame_data(i),source_frame_data(i)+source_frame_size(i));const auto independent_ss=independent_source_set(fs);assert(independent_ss==op.source_set_commitment);assert(independent_root_policy(policy(),independent_ss)==op.root_policy_commitment);assert(independent_authority_source(op)==op.authority_source_commitment);assert(independent_material_set(op)==op.material_set_commitment);assert(independent_material_operation(op)==op.operation_commitment);
        ++calls_;if(reentrant_)assert(!quarantined());if(throws_)throw 1;halofpx::context_store_bootstrap_material_synthetic_witness w;w.outcome=outcome_;w.material_attempt_id=op.material_attempt_id;w.material_root_identity=op.material_root_identity;w.registry_root_identity=op.registry_root_identity;w.root_policy_commitment=op.root_policy_commitment;w.authority_source_commitment=op.authority_source_commitment;w.source_set_commitment=op.source_set_commitment;w.material_set_commitment=op.material_set_commitment;w.operation_commitment=op.operation_commitment;for(size_t i=0;i<source_frame_count();++i)w.observed_frames.emplace_back(source_frame_data(i),source_frame_data(i)+source_frame_size(i));w.observed_manifest_envelope=op.manifest_envelope;w.durable_close_confirmation=independent_material_close(op);
        if(mutation_==1)w.material_root_identity[0]^=1;else if(mutation_==2)w.registry_root_identity[0]^=1;else if(mutation_==3)w.material_attempt_id[0]^=1;else if(mutation_==4)w.root_policy_commitment[0]^=1;else if(mutation_==5)w.authority_source_commitment[0]^=1;else if(mutation_==6)w.source_set_commitment[0]^=1;else if(mutation_==7)w.material_set_commitment[0]^=1;else if(mutation_==8)w.operation_commitment[0]^=1;else if(mutation_==9)w.observed_frames.clear();else if(mutation_==10)w.observed_frames[0][0]^=1;else if(mutation_==11)w.observed_manifest_envelope[0]^=1;else if(mutation_==12)w.durable_close_confirmation[0]^=1;else if(mutation_==13)w.observed_frames.push_back(w.observed_frames[0]);else if(mutation_==14)w.observed_manifest_envelope.pop_back();else if(mutation_==15)std::swap(w.observed_frames[0],w.observed_frames[1]);else if(mutation_==16)w.observed_frames[0][0]^=1;else if(mutation_==17)w.observed_frames[0][10]^=1;else if(mutation_==18){w.observed_frames[0][27]=0;w.observed_frames[0][28]=0;}else if(mutation_==19){w.observed_frames[0][27]=0;w.observed_frames[0][28]=129;}else if(mutation_==20)for(size_t i=35;i<43;++i)w.observed_frames[0][i]=0xff;else if(mutation_==21)w.observed_frames[0].pop_back();else if(mutation_==22)w.observed_frames[0].push_back(0);else if(mutation_==23)w.observed_frames[0].back()^=1;
        if(failpoint_>=0)w.outcome=(failpoint_%2)==0?halofpx::context_store_bootstrap_material_synthetic_backend_outcome::definitely_aborted:halofpx::context_store_bootstrap_material_synthetic_backend_outcome::uncertain;--active_;return w;
    }
};

halofpx::context_store_bootstrap_consumption_proof direct_material_source(fixture & f,const halofpx::context_store_bootstrap_plan & p,const digest & root,halofpx::context_store_bootstrap_backend_outcome outcome=halofpx::context_store_bootstrap_backend_outcome::advanced_durable) {
    consumption_backend b(root,outcome);halofpx::context_store_bootstrap_consumption_coordinator c(b);auto r=c.consume(consumption_request(f,p,77));assert(r.has_proof());return std::move(r.proof);
}

halofpx::context_store_bootstrap_recovered_consumption_proof recovered_material_source(fixture & f,const halofpx::context_store_bootstrap_plan & p,const digest & root) {
    consumption_backend b(root,halofpx::context_store_bootstrap_backend_outcome::uncertain);halofpx::context_store_bootstrap_consumption_coordinator c(b);assert(c.consume(consumption_request(f,p,77)).status==halofpx::context_store_bootstrap_consumption_status::visibility_uncertain);halofpx::context_store_bootstrap_reconciliation_request rr;rr.original_attempt_id.fill(77);rr.reconciliation_attempt_id.fill(78);rr.plan=p;rr.registry_authentication_key=f.config.protected_registry_authentication_key;auto verified=halofpx::context_store_verify_protected_registry_v1(f.registry_snapshot.data(),f.registry_snapshot.size(),f.config.protected_registry_authentication_key);rr.predecessor=*verified.authenticated_carrier();halofpx::context_store_bootstrap_reconciliation_coordinator rc(b);auto recovered=rc.reconcile(rr);assert(recovered.has_proof());return std::move(recovered.proof);
}
void throw_after_positive(){throw 9;}

void test_bootstrap_material_synthetic_seam() {
    static_assert(!std::is_copy_constructible_v<halofpx::context_store_bootstrap_material_synthetic_proof>);
    static_assert(!std::is_copy_constructible_v<halofpx::context_store_bootstrap_material_synthetic_witness>);
    const auto frame=object_frame(bytes(21,0x5a));manifest_options options;options.object_ids={sha(frame)};options.frame_bytes={frame.size()};auto f=make_fixture();f.signed_data=make_manifest(options);f.config.trusted_compatibility=f.signed_data.compatibility;seal_registry(f);authorize(f);halofpx::context_store_bootstrap_authority authority(f.config);const auto p=plan(authority.plan(request(f,f.signed_data.data)));digest registry_root{};registry_root.fill(0x73);auto policy=material_policy(f,registry_root);
    auto req=[&](uint8_t id){halofpx::context_store_bootstrap_material_synthetic_request r;r.material_attempt_id.fill(id);r.manifest_envelope_data=f.signed_data.data.data();r.manifest_envelope_size=f.signed_data.data.size();return r;};
    material_backend good(policy,{frame});halofpx::context_store_bootstrap_material_synthetic_coordinator coordinator(good);auto source=direct_material_source(f,p,registry_root);auto ok=coordinator.prepare(std::move(source),req(1));assert(!source.valid()&&ok.has_proof()&&ok.status==halofpx::context_store_bootstrap_material_synthetic_status::prepared_backend_claim&&good.calls_==1&&ok.proof.manifest_envelope_size()==f.signed_data.data.size()&&ok.proof.observed_frame_count()==1&&ok.proof.proposed_anchor()&&ok.proof.proposed_anchor()->authenticated()&&ok.proof.proposed_anchor()->envelope_size()==p.anchor()->envelope_size()&&std::equal(ok.proof.proposed_anchor()->envelope_data(),ok.proof.proposed_anchor()->envelope_data()+ok.proof.proposed_anchor()->envelope_size(),p.anchor()->envelope_data())&&ok.proof.direct_source_proof());auto moved=std::move(ok.proof);assert(moved.valid()&&!ok.proof.valid());
    material_backend same(policy,{frame});same.outcome_=halofpx::context_store_bootstrap_material_synthetic_backend_outcome::already_same_backend_claim;halofpx::context_store_bootstrap_material_synthetic_coordinator sc(same);auto same_source=direct_material_source(f,p,registry_root,halofpx::context_store_bootstrap_backend_outcome::already_same_durable);assert(sc.prepare(std::move(same_source),req(2)).status==halofpx::context_store_bootstrap_material_synthetic_status::already_same_backend_claim&&!same_source.valid());
    material_backend reconciled(policy,{frame});halofpx::context_store_bootstrap_material_synthetic_coordinator rcc(reconciled);auto recovered=recovered_material_source(f,p,registry_root);assert(rcc.prepare(std::move(recovered),req(4)).has_proof()&&!recovered.valid());
    material_backend post_positive(policy,{frame});halofpx::context_store_bootstrap_material_synthetic_coordinator ppc(post_positive);auto pps=direct_material_source(f,p,registry_root);halofpx::context_store_bootstrap_material_synthetic_test_access::set(throw_after_positive);auto ppr=ppc.prepare(std::move(pps),req(8));halofpx::context_store_bootstrap_material_synthetic_test_access::set(nullptr);assert(ppr.status==halofpx::context_store_bootstrap_material_synthetic_status::visibility_uncertain&&!ppr.has_proof()&&post_positive.quarantined()&&!pps.valid());
    auto aggregate_too_small=policy;aggregate_too_small.maximum_aggregate_frame_bytes=frame.size()-1;material_backend underflow_guard(aggregate_too_small,{frame});assert(!underflow_guard.valid());auto frame_too_small=policy;frame_too_small.maximum_frame_bytes=frame.size()-1;material_backend frame_guard(frame_too_small,{frame});assert(!frame_guard.valid());
    auto zero_material_root=policy;zero_material_root.material_root_identity.fill(0);material_backend zero_root_guard(zero_material_root,{frame});assert(!zero_root_guard.valid());auto equal_roots=policy;equal_roots.material_root_identity=registry_root;material_backend equal_root_guard(equal_roots,{frame});assert(!equal_root_guard.valid());auto zero_count=policy;zero_count.maximum_source_object_count=0;material_backend count_guard(zero_count,{frame});assert(!count_guard.valid());
    material_backend invalid(policy,{frame});halofpx::context_store_bootstrap_material_synthetic_coordinator ic(invalid);auto bad_source=direct_material_source(f,p,registry_root);auto bad_request=req(3);bad_request.manifest_envelope_data=nullptr;assert(ic.prepare(std::move(bad_source),bad_request).status==halofpx::context_store_bootstrap_material_synthetic_status::invalid_request&&!bad_source.valid()&&invalid.calls_==0);
    halofpx::context_store_bootstrap_consumption_proof default_source;assert(ic.prepare(std::move(default_source),req(33)).status==halofpx::context_store_bootstrap_material_synthetic_status::invalid_source&&invalid.calls_==0);auto original=direct_material_source(f,p,registry_root);auto moved_source=std::move(original);assert(ic.prepare(std::move(original),req(34)).status==halofpx::context_store_bootstrap_material_synthetic_status::invalid_source&&!original.valid()&&moved_source.valid());auto wrong_root=registry_root;wrong_root[0]^=1;auto differently_bound=direct_material_source(f,p,wrong_root);assert(ic.prepare(std::move(differently_bound),req(35)).status==halofpx::context_store_bootstrap_material_synthetic_status::source_conflict&&!differently_bound.valid()&&invalid.calls_==0);
    for(int manifest_case=0;manifest_case<2;++manifest_case){material_backend malformed_manifest(policy,{frame});halofpx::context_store_bootstrap_material_synthetic_coordinator mmc(malformed_manifest);auto ms=direct_material_source(f,p,registry_root);auto mr=req(static_cast<uint8_t>(5+manifest_case));auto changed=f.signed_data.data;if(manifest_case==0)changed.pop_back();else changed[0]^=1;mr.manifest_envelope_data=changed.data();mr.manifest_envelope_size=changed.size();assert(mmc.prepare(std::move(ms),mr).status==halofpx::context_store_bootstrap_material_synthetic_status::invalid_request&&!ms.valid()&&malformed_manifest.calls_==0);}
    auto reject_manifest=[&](const bytes&candidate,uint8_t id){material_backend mb(policy,{frame});halofpx::context_store_bootstrap_material_synthetic_coordinator mc(mb);auto src=direct_material_source(f,p,registry_root);auto r=req(id);r.manifest_envelope_data=candidate.data();r.manifest_envelope_size=candidate.size();assert(mc.prepare(std::move(src),r).status==halofpx::context_store_bootstrap_material_synthetic_status::invalid_request&&!src.valid()&&mb.calls_==0);};manifest_options wrong_scope_options;wrong_scope_options.scope=0x81;auto wrong_scope_manifest=make_manifest(wrong_scope_options).data;manifest_options wrong_generation_options;wrong_generation_options.generation=2;auto wrong_generation_manifest=make_manifest(wrong_generation_options).data;manifest_options wrong_predecessor_options;wrong_predecessor_options.predecessor=true;auto wrong_predecessor_manifest=make_manifest(wrong_predecessor_options).data;manifest_options wrong_key_generation_options;wrong_key_generation_options.key_generation=6;auto wrong_key_generation_manifest=make_manifest(wrong_key_generation_options).data;manifest_options wrong_durability_options;wrong_durability_options.durability_mode=1;auto wrong_durability_manifest=make_manifest(wrong_durability_options).data;manifest_options wrong_count_options;wrong_count_options.objects=3;auto wrong_object_count_manifest=make_manifest(wrong_count_options).data;manifest_options duplicate_options;duplicate_options.objects=2;duplicate_options.object_ids={options.object_ids[0],options.object_ids[0]};duplicate_options.frame_bytes={frame.size(),frame.size()};auto duplicate_object_manifest=make_manifest(duplicate_options).data;manifest_options missing_options;missing_options.objects=0;auto missing_object_manifest=make_manifest(missing_options).data;manifest_options extra_options;extra_options.objects=2;auto extra_object_manifest=make_manifest(extra_options).data;manifest_options ordered_options;ordered_options.objects=2;ordered_options.object_ids={sha(object_frame(bytes(5,1))),sha(object_frame(bytes(5,2)))};ordered_options.frame_bytes={48,48};auto object_order_manifest=make_manifest(ordered_options).data;std::swap(ordered_options.object_ids[0],ordered_options.object_ids[1]);auto reordered_object_manifest=make_manifest(ordered_options).data;bytes malformed_manifest_bytes={0xff},truncated_manifest_bytes=f.signed_data.data;truncated_manifest_bytes.pop_back();bytes noncanonical_manifest_bytes={0xbf,0xff};bytes oversized_manifest_bytes(halofpx::context_store_manifest_max_bytes+1,0);const std::array<const bytes*,15> manifest_matrix={&wrong_scope_manifest,&wrong_generation_manifest,&wrong_predecessor_manifest,&wrong_key_generation_manifest,&wrong_durability_manifest,&wrong_object_count_manifest,&duplicate_object_manifest,&missing_object_manifest,&extra_object_manifest,&object_order_manifest,&reordered_object_manifest,&malformed_manifest_bytes,&truncated_manifest_bytes,&noncanonical_manifest_bytes,&oversized_manifest_bytes};for(size_t i=0;i<manifest_matrix.size();++i)reject_manifest(*manifest_matrix[i],static_cast<uint8_t>(120+i));
    for(int mutation=1;mutation<=14;++mutation){material_backend corrupt(policy,{frame});corrupt.mutation_=mutation;halofpx::context_store_bootstrap_material_synthetic_coordinator cc(corrupt);auto s=direct_material_source(f,p,registry_root);assert(cc.prepare(std::move(s),req(static_cast<uint8_t>(10+mutation))).status==halofpx::context_store_bootstrap_material_synthetic_status::visibility_uncertain&&corrupt.quarantined()&&corrupt.calls_==1);auto next=direct_material_source(f,p,registry_root);assert(cc.prepare(std::move(next),req(static_cast<uint8_t>(40+mutation))).status==halofpx::context_store_bootstrap_material_synthetic_status::material_root_quarantined&&corrupt.calls_==1);}
    const std::array<int,8> readback_structural_matrix={16,17,18,19,20,21,22,23};for(int mutation:readback_structural_matrix){material_backend readback_invalid(policy,{frame});readback_invalid.mutation_=mutation;halofpx::context_store_bootstrap_material_synthetic_coordinator ric(readback_invalid);auto ris=direct_material_source(f,p,registry_root);assert(ric.prepare(std::move(ris),req(static_cast<uint8_t>(140+mutation))).status==halofpx::context_store_bootstrap_material_synthetic_status::visibility_uncertain&&!ris.valid()&&readback_invalid.quarantined());}
    std::vector<bytes> malformed_frames;auto fm=frame;fm[0]^=1;malformed_frames.push_back(fm);fm=frame;fm[10]^=1;malformed_frames.push_back(fm);fm=frame;fm[29]='x';malformed_frames.push_back(fm);fm=frame;fm[27]=0;fm[28]=0;malformed_frames.push_back(fm);fm=frame;fm[27]=0;fm[28]=129;malformed_frames.push_back(fm);fm=frame;for(size_t i=35;i<43;++i)fm[i]=0xff;malformed_frames.push_back(fm);fm=frame;fm[42]=22;malformed_frames.push_back(fm);fm=frame;fm.pop_back();malformed_frames.push_back(fm);fm=frame;fm.push_back(0);malformed_frames.push_back(fm);fm=frame;fm.back()^=1;malformed_frames.push_back(fm);auto oversized_source_frame=frame;oversized_source_frame.resize(static_cast<size_t>(policy.maximum_frame_bytes)+1,0);malformed_frames.push_back(std::move(oversized_source_frame));for(size_t i=0;i<malformed_frames.size();++i){material_backend source_corrupt(policy,{malformed_frames[i]});halofpx::context_store_bootstrap_material_synthetic_coordinator scc(source_corrupt);auto s0=direct_material_source(f,p,registry_root);assert(scc.prepare(std::move(s0),req(static_cast<uint8_t>(70+i))).status==halofpx::context_store_bootstrap_material_synthetic_status::invalid_request&&!s0.valid()&&source_corrupt.calls_==0);}
    material_backend replay(policy,{frame});halofpx::context_store_bootstrap_material_synthetic_coordinator rc(replay);auto r1=direct_material_source(f,p,registry_root);assert(rc.prepare(std::move(r1),req(80)).has_proof());auto r2=direct_material_source(f,p,registry_root);assert(rc.prepare(std::move(r2),req(80)).status==halofpx::context_store_bootstrap_material_synthetic_status::attempt_replayed&&replay.calls_==1);
    material_backend uncertain(policy,{frame});uncertain.throws_=true;uncertain.reentrant_=true;halofpx::context_store_bootstrap_material_synthetic_coordinator uc(uncertain);auto us=direct_material_source(f,p,registry_root);assert(uc.prepare(std::move(us),req(81)).status==halofpx::context_store_bootstrap_material_synthetic_status::visibility_uncertain&&uncertain.quarantined());
    const std::array uncertain_outcomes={halofpx::context_store_bootstrap_material_synthetic_backend_outcome::malformed_response,halofpx::context_store_bootstrap_material_synthetic_backend_outcome::incomplete,halofpx::context_store_bootstrap_material_synthetic_backend_outcome::unconfirmed_close,halofpx::context_store_bootstrap_material_synthetic_backend_outcome::uncertain,halofpx::context_store_bootstrap_material_synthetic_backend_outcome::late_completion_risk};for(size_t i=0;i<uncertain_outcomes.size();++i){material_backend ub(policy,{frame});ub.outcome_=uncertain_outcomes[i];halofpx::context_store_bootstrap_material_synthetic_coordinator ucc(ub);auto ubs=direct_material_source(f,p,registry_root);assert(ucc.prepare(std::move(ubs),req(static_cast<uint8_t>(82+i))).status==halofpx::context_store_bootstrap_material_synthetic_status::visibility_uncertain&&!ubs.valid()&&ub.quarantined());}
    const std::array definite_outcomes={halofpx::context_store_bootstrap_material_synthetic_backend_outcome::definitely_aborted,halofpx::context_store_bootstrap_material_synthetic_backend_outcome::source_conflict,halofpx::context_store_bootstrap_material_synthetic_backend_outcome::policy_conflict,halofpx::context_store_bootstrap_material_synthetic_backend_outcome::writer_busy,halofpx::context_store_bootstrap_material_synthetic_backend_outcome::object_collision,halofpx::context_store_bootstrap_material_synthetic_backend_outcome::manifest_collision,halofpx::context_store_bootstrap_material_synthetic_backend_outcome::no_space,halofpx::context_store_bootstrap_material_synthetic_backend_outcome::quota_exhausted,halofpx::context_store_bootstrap_material_synthetic_backend_outcome::reserve_exhausted,halofpx::context_store_bootstrap_material_synthetic_backend_outcome::read_only,halofpx::context_store_bootstrap_material_synthetic_backend_outcome::storage_error,halofpx::context_store_bootstrap_material_synthetic_backend_outcome::synchronization_error};for(size_t i=0;i<definite_outcomes.size();++i){material_backend rejected_backend(policy,{frame});rejected_backend.outcome_=definite_outcomes[i];halofpx::context_store_bootstrap_material_synthetic_coordinator dc(rejected_backend);auto ds=direct_material_source(f,p,registry_root);auto dr=dc.prepare(std::move(ds),req(static_cast<uint8_t>(90+i)));assert(!dr.has_proof()&&!ds.valid()&&!rejected_backend.quarantined()&&rejected_backend.calls_==1);}
    const std::array<const char*,28> stage_boundary_failpoints={"before_object_stage","after_object_stage","before_object_write","after_object_write","before_object_verify","after_object_verify","before_object_file_sync","after_object_file_sync","before_object_no_replace","after_object_no_replace","before_object_dir_sync","after_object_dir_sync","before_manifest_stage","after_manifest_stage","before_manifest_write","after_manifest_write","before_manifest_verify","after_manifest_verify","before_manifest_file_sync","after_manifest_file_sync","before_manifest_no_replace","after_manifest_no_replace","before_manifest_dir_sync","after_manifest_dir_sync","before_durable_close","after_durable_close","before_attempt_terminal","after_attempt_terminal"};for(size_t fp=0;fp<stage_boundary_failpoints.size();++fp){assert(stage_boundary_failpoints[fp][0]!='\0');material_backend scripted(policy,{frame});scripted.failpoint_=static_cast<int>(fp);halofpx::context_store_bootstrap_material_synthetic_coordinator fc(scripted);auto fs=direct_material_source(f,p,registry_root);auto fr=fc.prepare(std::move(fs),req(static_cast<uint8_t>(180+fp)));assert(!fr.has_proof()&&!fs.valid()&&scripted.calls_==1);if((fp%2)==0)assert(fr.status==halofpx::context_store_bootstrap_material_synthetic_status::definitely_aborted&&!scripted.quarantined());else assert(fr.status==halofpx::context_store_bootstrap_material_synthetic_status::visibility_uncertain&&scripted.quarantined());}
    const auto frame2=object_frame(bytes(13,0x6b));manifest_options two;two.objects=2;two.object_ids={sha(frame),sha(frame2)};two.frame_bytes={frame.size(),frame2.size()};auto f2=make_fixture();f2.signed_data=make_manifest(two);f2.config.trusted_compatibility=f2.signed_data.compatibility;seal_registry(f2);authorize(f2);halofpx::context_store_bootstrap_authority a2(f2.config);const auto p2=plan(a2.plan(request(f2,f2.signed_data.data)));auto req2=[&](uint8_t id){halofpx::context_store_bootstrap_material_synthetic_request r;r.material_attempt_id.fill(id);r.manifest_envelope_data=f2.signed_data.data.data();r.manifest_envelope_size=f2.signed_data.data.size();return r;};material_backend reordered_readback(policy,{frame,frame2});reordered_readback.mutation_=15;halofpx::context_store_bootstrap_material_synthetic_coordinator roc(reordered_readback);auto ros=direct_material_source(f2,p2,registry_root);assert(roc.prepare(std::move(ros),req2(111)).status==halofpx::context_store_bootstrap_material_synthetic_status::visibility_uncertain&&reordered_readback.quarantined());material_backend reordered_source(policy,{frame2,frame});halofpx::context_store_bootstrap_material_synthetic_coordinator rsc(reordered_source);auto rss=direct_material_source(f2,p2,registry_root);assert(rsc.prepare(std::move(rss),req2(112)).status==halofpx::context_store_bootstrap_material_synthetic_status::invalid_request&&reordered_source.calls_==0);
    material_backend capacity(policy,{frame});capacity.outcome_=halofpx::context_store_bootstrap_material_synthetic_backend_outcome::definitely_aborted;halofpx::context_store_bootstrap_material_synthetic_coordinator hc(capacity);for(size_t i=0;i<511;++i){auto hs=direct_material_source(f,p,registry_root);auto hr=req(1);hr.material_attempt_id.fill(0);hr.material_attempt_id[30]=static_cast<uint8_t>(i>>8);hr.material_attempt_id[31]=static_cast<uint8_t>(i);if(i==0)hr.material_attempt_id[0]=1;assert(hc.prepare(std::move(hs),hr).status==halofpx::context_store_bootstrap_material_synthetic_status::definitely_aborted);}auto last_a=direct_material_source(f,p,registry_root),last_b=direct_material_source(f,p,registry_root);auto last_ra=req(1),last_rb=req(1);last_ra.material_attempt_id.fill(0xa1);last_rb.material_attempt_id.fill(0xb2);std::array<halofpx::context_store_bootstrap_material_synthetic_status,2> last_status{};std::thread lta([&]{last_status[0]=hc.prepare(std::move(last_a),last_ra).status;}),ltb([&]{last_status[1]=hc.prepare(std::move(last_b),last_rb).status;});lta.join();ltb.join();assert((last_status[0]==halofpx::context_store_bootstrap_material_synthetic_status::definitely_aborted&&last_status[1]==halofpx::context_store_bootstrap_material_synthetic_status::history_exhausted)||(last_status[1]==halofpx::context_store_bootstrap_material_synthetic_status::definitely_aborted&&last_status[0]==halofpx::context_store_bootstrap_material_synthetic_status::history_exhausted));auto overflow=direct_material_source(f,p,registry_root);auto over=req(1);over.material_attempt_id.fill(0xff);assert(hc.prepare(std::move(overflow),over).status==halofpx::context_store_bootstrap_material_synthetic_status::history_exhausted&&capacity.calls_==512);
    auto replay_after_capacity_source=direct_material_source(f,p,registry_root);auto replay_after_capacity_request=req(117);replay_after_capacity_request.material_attempt_id.fill(0);replay_after_capacity_request.material_attempt_id[0]=1;assert(hc.prepare(std::move(replay_after_capacity_source),replay_after_capacity_request).status==halofpx::context_store_bootstrap_material_synthetic_status::attempt_replayed&&capacity.calls_==512);
    material_backend serialized(policy,{frame});serialized.outcome_=halofpx::context_store_bootstrap_material_synthetic_backend_outcome::definitely_aborted;serialized.overlap_=true;halofpx::context_store_bootstrap_material_synthetic_coordinator ser(serialized);auto ser_source_a=direct_material_source(f,p,registry_root),ser_source_b=direct_material_source(f,p,registry_root);auto ser_req_a=req(115),ser_req_b=req(116);std::array<halofpx::context_store_bootstrap_material_synthetic_status,2> ser_status{};std::thread ser_a([&]{ser_status[0]=ser.prepare(std::move(ser_source_a),ser_req_a).status;}),ser_b([&]{ser_status[1]=ser.prepare(std::move(ser_source_b),ser_req_b).status;});ser_a.join();ser_b.join();assert(ser_status[0]==halofpx::context_store_bootstrap_material_synthetic_status::definitely_aborted&&ser_status[1]==halofpx::context_store_bootstrap_material_synthetic_status::definitely_aborted&&serialized.calls_==2&&serialized.max_active_.load()==1);
}
}

int main(){test_derived_plan_and_ownership();test_manifest_rejections();test_invalid_authority_and_separation();test_base_scope_sensitivity();test_public_scope_helpers_reject_malformed_ids();test_old_authenticated_snapshot_is_still_accepted();test_attempt_snapshot_bindings_and_concurrency();test_authenticated_token_semantic_rejections();test_consumption_coordinator_backend_fencing();test_consumption_reconciliation_fencing();test_bootstrap_material_synthetic_seam();}
