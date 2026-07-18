#include "halofpx-context-store-authority.h"

#include <algorithm>
#include <array>
#include <climits>

namespace halofpx {
namespace {

constexpr char snapshot_domain[] = "halofpx.bootstrap-authority-snapshot.v1";
constexpr char base_scope_domain[] = "halofpx.bootstrap-authority-base-scope.v1";
constexpr char scope_domain[] = "halofpx.bootstrap-authority-scope.v2";
constexpr char plan_domain[] = "halofpx.bootstrap-plan.v2";

struct bounded_buffer {
    std::array<uint8_t, 2048> bytes {};
    size_t size = 0;

    bool append(const void * source, size_t count) noexcept {
        if ((source == nullptr && count != 0) || count > bytes.size() - size) return false;
        const auto * first = static_cast<const uint8_t *>(source);
        std::copy_n(first, count, bytes.begin() + size);
        size += count;
        return true;
    }
    bool u64(uint64_t value) noexcept {
        std::array<uint8_t, 8> encoded {};
        for (size_t i = 0; i < encoded.size(); ++i) encoded[7 - i] = static_cast<uint8_t>(value >> (i * 8));
        return append(encoded.data(), encoded.size());
    }
    bool id(const context_store_registered_id & value) noexcept {
        if (value.size == 0 || value.size > context_store_registered_id_max_bytes) return false;
        for (size_t i = 0; i < value.size; ++i) {
            if (value.bytes[i] == 0 || value.bytes[i] > 0x7f) return false;
        }
        const uint8_t length[2] = { 0, value.size };
        return append(length, sizeof(length)) && append(value.bytes.data(), value.size);
    }
};

void wipe(void * pointer, size_t size) noexcept {
    volatile uint8_t * bytes = static_cast<volatile uint8_t *>(pointer);
    while (size-- != 0) *bytes++ = 0;
}

bool valid_id(const context_store_registered_id & id) noexcept {
    if (id.size == 0 || id.size > context_store_registered_id_max_bytes) return false;
    for (size_t i = 0; i < id.size; ++i) {
        const auto value = static_cast<uint8_t>(id.bytes[i]);
        if (value == 0 || value > 0x7f) return false;
    }
    return true;
}

bool valid_key(const context_store_key_disposition disposition,
        const context_store_registered_id & id, const context_store_key_view & key) noexcept {
    return disposition == context_store_key_disposition::active && valid_id(id) &&
        key.data != nullptr && key.size != 0 && key.size <= context_store_master_key_max_bytes;
}

bool same_id(const context_store_registered_id & left, const context_store_registered_id & right) noexcept {
    if (left.size != right.size) return false;
    volatile uint8_t difference = 0;
    for (size_t i = 0; i < left.size; ++i) difference = static_cast<uint8_t>(difference | (left.bytes[i] ^ right.bytes[i]));
    return difference == 0;
}

bool same_secret(const context_store_key_view & left, const context_store_key_view & right) noexcept {
    constexpr size_t hmac_sha256_block_bytes = 64;
    std::array<uint8_t, hmac_sha256_block_bytes> left_effective {};
    std::array<uint8_t, hmac_sha256_block_bytes> right_effective {};
    auto normalize = [](const context_store_key_view & source,
            std::array<uint8_t, hmac_sha256_block_bytes> & effective) noexcept {
        if (source.size <= effective.size()) {
            std::copy_n(source.data, source.size, effective.begin());
            return true;
        }
        context_store_format_digest hashed {};
        const bool ok = context_store_sha256_bounded(source.data, source.size,
            context_store_master_key_max_bytes, hashed);
        if (ok) std::copy(hashed.begin(), hashed.end(), effective.begin());
        wipe(hashed.data(), hashed.size());
        return ok;
    };
    if (!normalize(left, left_effective) || !normalize(right, right_effective)) {
        wipe(left_effective.data(), left_effective.size());
        wipe(right_effective.data(), right_effective.size());
        return true;
    }
    volatile uint8_t difference = 0;
    for (size_t i = 0; i < left_effective.size(); ++i)
        difference = static_cast<uint8_t>(difference | (left_effective[i] ^ right_effective[i]));
    wipe(left_effective.data(), left_effective.size());
    wipe(right_effective.data(), right_effective.size());
    return difference == 0;
}

bool snapshot_message(const context_store_bootstrap_authority_config & config,
        const context_store_format_digest & anchor_secret_digest,
        const context_store_format_digest & admin_secret_digest,
        const context_store_format_digest & manifest_secret_digest,
        const context_store_format_digest & registry_authority_binding,
        const context_store_protected_registry_body & registry,
        const context_store_format_digest & registry_snapshot_digest,
        bounded_buffer & message) noexcept {
    return message.append(snapshot_domain, sizeof(snapshot_domain)) &&
        message.id(config.bootstrap_admin_key.key_id) && message.u64(config.bootstrap_admin_key.generation) &&
        message.append(admin_secret_digest.data(), admin_secret_digest.size()) &&
        message.id(config.anchor_signing_key.key_id) && message.u64(config.anchor_signing_key.generation) &&
        message.append(anchor_secret_digest.data(), anchor_secret_digest.size()) &&
        message.id(config.manifest_authentication_key.key_id) &&
        message.u64(config.manifest_authentication_key.generation) &&
        message.append(manifest_secret_digest.data(), manifest_secret_digest.size()) &&
        message.id(config.protected_registry_authentication_key.key_id) &&
        message.u64(config.protected_registry_authentication_key.generation) &&
        message.append(registry_authority_binding.data(), registry_authority_binding.size()) &&
        message.append(config.store_uuid.data(), config.store_uuid.size()) &&
        message.append(config.namespace_id.data(), config.namespace_id.size()) &&
        message.u64(config.policy_epoch) &&
        message.append(config.checkpoint_lineage_id.data(), config.checkpoint_lineage_id.size()) &&
        message.u64(config.manifest_key_generation) && message.u64(config.authority_epoch) &&
        message.id(registry.registry_id) && message.u64(registry.registry_epoch) &&
        message.append(registry_snapshot_digest.data(), registry_snapshot_digest.size()) &&
        message.append(registry.policy_commitment.data(), registry.policy_commitment.size()) &&
        message.u64(registry.last_consumed_sequence) && [&] {
            for (const auto & component : config.trusted_compatibility.components) {
                if (!message.append(component.data(), component.size())) return false;
            }
            return message.append(config.trusted_compatibility.root.data(),
                config.trusted_compatibility.root.size());
        }();
}

bool base_scope_message(const context_store_bootstrap_authority_config & config,
        bounded_buffer & message) noexcept {
    return message.append(base_scope_domain, sizeof(base_scope_domain)) &&
        message.id(config.bootstrap_admin_key.key_id) && message.u64(config.bootstrap_admin_key.generation) &&
        message.id(config.anchor_signing_key.key_id) && message.u64(config.anchor_signing_key.generation) &&
        message.id(config.manifest_authentication_key.key_id) && message.u64(config.manifest_authentication_key.generation) &&
        message.id(config.protected_registry_authentication_key.key_id) &&
        message.u64(config.protected_registry_authentication_key.generation) &&
        message.append(config.store_uuid.data(), config.store_uuid.size()) &&
        message.append(config.namespace_id.data(), config.namespace_id.size()) && message.u64(config.policy_epoch) &&
        message.append(config.checkpoint_lineage_id.data(), config.checkpoint_lineage_id.size()) &&
        message.u64(config.manifest_key_generation) && message.u64(config.authority_epoch) && [&] {
            for (const auto & component : config.trusted_compatibility.components)
                if (!message.append(component.data(), component.size())) return false;
            return message.append(config.trusted_compatibility.root.data(), config.trusted_compatibility.root.size());
        }();
}

bool full_scope_message(const context_store_format_digest & base,
        const context_store_protected_registry_body & registry,
        const context_store_format_digest & snapshot_digest,
        bounded_buffer & message) noexcept {
    return message.append(scope_domain, sizeof(scope_domain)) &&
        message.append(base.data(), base.size()) && message.id(registry.registry_id) &&
        message.u64(registry.registry_epoch) && message.append(snapshot_digest.data(), snapshot_digest.size()) &&
        message.append(registry.policy_commitment.data(), registry.policy_commitment.size()) &&
        message.u64(registry.last_consumed_sequence);
}

bool trusted_compatibility_valid(
        const context_store_compatibility_expectation & compatibility) noexcept {
    constexpr char domain[] = "halofpx.compat.v1";
    bounded_buffer encoded;
    constexpr uint8_t map_header = 0xb0;
    constexpr uint8_t digest_header[] = { 0x58, 0x20 };
    if (!encoded.append(domain, sizeof(domain)) || !encoded.append(&map_header, 1)) return false;
    for (size_t index = 0; index < compatibility.components.size(); ++index) {
        const auto key = static_cast<uint8_t>(index);
        if (!encoded.append(&key, 1) || !encoded.append(digest_header, sizeof(digest_header)) ||
            !encoded.append(compatibility.components[index].data(), compatibility.components[index].size())) {
            wipe(encoded.bytes.data(), encoded.bytes.size());
            return false;
        }
    }
    context_store_format_digest expected {};
    const bool hashed = context_store_sha256_bounded(encoded.bytes.data(), encoded.size,
        encoded.bytes.size(), expected);
    wipe(encoded.bytes.data(), encoded.bytes.size());
    volatile uint8_t difference = 0;
    for (size_t index = 0; index < expected.size(); ++index)
        difference = static_cast<uint8_t>(difference | (expected[index] ^ compatibility.root[index]));
    wipe(expected.data(), expected.size());
    return hashed && difference == 0;
}

bool plan_message(const context_store_format_digest & snapshot, const context_store_bootstrap_command_id & command_id,
        uint64_t authorization_sequence,
        size_t object_count,
        const context_store_format_digest & selected_manifest_digest,
        const context_store_format_digest & authorization_token_digest,
        const context_store_format_digest & anchor_digest, bounded_buffer & message) noexcept {
    return message.append(plan_domain, sizeof(plan_domain)) &&
        message.append(snapshot.data(), snapshot.size()) && message.append(command_id.data(), command_id.size()) &&
        message.u64(authorization_sequence) &&
        message.u64(static_cast<uint64_t>(object_count)) &&
        message.append(selected_manifest_digest.data(), selected_manifest_digest.size()) &&
        message.append(authorization_token_digest.data(), authorization_token_digest.size()) &&
        message.append(anchor_digest.data(), anchor_digest.size());
}


template <size_t N>
bool nonzero_bytes(const std::array<uint8_t, N> & value) noexcept {
    volatile uint8_t accumulated = 0;
    for (const uint8_t byte : value) accumulated = static_cast<uint8_t>(accumulated | byte);
    return accumulated != 0;
}

} // namespace

bool context_store_bootstrap_authority_base_scope_commitment(
        const context_store_bootstrap_authority_config & config,
        context_store_format_digest & commitment) noexcept {
    bounded_buffer message;
    const bool encoded = base_scope_message(config, message);
    const bool hashed = encoded && context_store_sha256_bounded(
        message.bytes.data(), message.size, message.bytes.size(), commitment);
    wipe(message.bytes.data(), message.bytes.size());
    return hashed;
}

bool context_store_bootstrap_authority_scope_commitment(
        const context_store_bootstrap_authority_config & config,
        context_store_format_digest & commitment) noexcept {
    context_store_format_digest base {};
    if (!context_store_bootstrap_authority_base_scope_commitment(config, base)) return false;
    const auto verified = context_store_verify_protected_registry_v1(
        config.protected_registry_snapshot_data, config.protected_registry_snapshot_size,
        config.protected_registry_authentication_key);
    const auto * carrier = verified.authenticated_carrier();
    const auto * body = carrier ? carrier->body() : nullptr;
    const auto * digest = carrier ? carrier->envelope_digest() : nullptr;
    bounded_buffer message;
    const bool encoded = body && digest && body->authority_base_scope_commitment == base &&
        full_scope_message(base, *body, *digest, message);
    const bool hashed = encoded && context_store_sha256_bounded(
        message.bytes.data(), message.size, message.bytes.size(), commitment);
    wipe(base.data(), base.size()); wipe(message.bytes.data(), message.bytes.size());
    return hashed;
}

context_store_bootstrap_authority::context_store_bootstrap_authority(
        const context_store_bootstrap_authority_config & config) noexcept {
    const auto & anchor = config.anchor_signing_key;
    const auto & admin = config.bootstrap_admin_key;
    const auto & manifest = config.manifest_authentication_key;
    const auto & registry_key = config.protected_registry_authentication_key;
    if (!valid_key(anchor.disposition, anchor.key_id, anchor.master_key) ||
        !valid_key(admin.disposition, admin.key_id, admin.master_key) ||
        !valid_key(manifest.disposition, manifest.key_id, manifest.master_key) ||
        !valid_key(registry_key.disposition, registry_key.key_id, registry_key.master_key) ||
        anchor.generation == 0 || admin.generation == 0 || manifest.generation == 0 || registry_key.generation == 0 ||
        manifest.generation != config.manifest_key_generation || config.policy_epoch == 0 ||
        config.manifest_key_generation == 0 || config.authority_epoch == 0 ||
        config.protected_registry_snapshot_data == nullptr || config.protected_registry_snapshot_size == 0 ||
        config.protected_registry_snapshot_size > context_store_protected_registry_max_bytes ||
        !nonzero_bytes(config.store_uuid) || !nonzero_bytes(config.namespace_id) ||
        !nonzero_bytes(config.checkpoint_lineage_id) ||
        !trusted_compatibility_valid(config.trusted_compatibility) ||
        (anchor.generation == admin.generation && same_id(anchor.key_id, admin.key_id)) ||
        (anchor.generation == manifest.generation && same_id(anchor.key_id, manifest.key_id)) ||
        (admin.generation == manifest.generation && same_id(admin.key_id, manifest.key_id)) ||
        (anchor.generation == registry_key.generation && same_id(anchor.key_id, registry_key.key_id)) ||
        (admin.generation == registry_key.generation && same_id(admin.key_id, registry_key.key_id)) ||
        (manifest.generation == registry_key.generation && same_id(manifest.key_id, registry_key.key_id)) ||
        same_secret(anchor.master_key, admin.master_key) ||
        same_secret(anchor.master_key, manifest.master_key) ||
        same_secret(admin.master_key, manifest.master_key) ||
        same_secret(anchor.master_key, registry_key.master_key) ||
        same_secret(admin.master_key, registry_key.master_key) ||
        same_secret(manifest.master_key, registry_key.master_key)) return;

    context_store_format_digest base_scope {};
    if (!context_store_bootstrap_authority_base_scope_commitment(config, base_scope)) return;
    const auto registry_result = context_store_verify_protected_registry_v1(
        config.protected_registry_snapshot_data, config.protected_registry_snapshot_size, registry_key);
    const auto * registry = registry_result.authenticated_carrier();
    const auto * registry_body = registry ? registry->body() : nullptr;
    const auto * registry_digest = registry ? registry->envelope_digest() : nullptr;
    const auto * registry_binding = registry ? registry->authority_binding() : nullptr;
    if (!registry_body || !registry_digest || !registry_binding ||
        registry_body->authority_base_scope_commitment != base_scope ||
        registry_body->last_consumed_sequence == UINT64_MAX) {
        wipe(base_scope.data(), base_scope.size()); return;
    }

    anchor_key_id_ = anchor.key_id;
    anchor_key_generation_ = anchor.generation;
    anchor_key_size_ = anchor.master_key.size;
    std::copy_n(anchor.master_key.data, anchor_key_size_, anchor_key_.begin());
    admin_key_id_ = admin.key_id;
    admin_key_generation_ = admin.generation;
    admin_key_size_ = admin.master_key.size;
    std::copy_n(admin.master_key.data, admin_key_size_, admin_key_.begin());
    manifest_key_id_ = manifest.key_id;
    manifest_key_generation_ = manifest.generation;
    manifest_key_size_ = manifest.master_key.size;
    std::copy_n(manifest.master_key.data, manifest_key_size_, manifest_key_.begin());
    trusted_compatibility_ = config.trusted_compatibility;
    protected_registry_id_ = registry_body->registry_id;
    protected_registry_epoch_ = registry_body->registry_epoch;
    protected_registry_snapshot_digest_ = *registry_digest;
    protected_registry_policy_digest_ = registry_body->policy_commitment;
    expected_authorization_sequence_ = registry_body->last_consumed_sequence + 1;

    bootstrap_body_.store_uuid = config.store_uuid;
    bootstrap_body_.namespace_id = config.namespace_id;
    bootstrap_body_.policy_epoch = config.policy_epoch;
    bootstrap_body_.checkpoint_lineage_id = config.checkpoint_lineage_id;
    bootstrap_body_.manifest_key_generation = config.manifest_key_generation;
    bootstrap_body_.authority_epoch = config.authority_epoch;
    bootstrap_body_.generation = 1;
    bootstrap_body_.has_predecessor = false;
    bootstrap_body_.predecessor_manifest_digest.fill(0);

    context_store_format_digest anchor_secret_digest {};
    context_store_format_digest admin_secret_digest {};
    context_store_format_digest manifest_secret_digest {};
    bounded_buffer message;
    context_store_bootstrap_authority_config owned_config;
    owned_config.anchor_signing_key.key_id = anchor_key_id_;
    owned_config.anchor_signing_key.generation = anchor_key_generation_;
    owned_config.bootstrap_admin_key.key_id = admin_key_id_;
    owned_config.bootstrap_admin_key.generation = admin_key_generation_;
    owned_config.manifest_authentication_key.key_id = manifest_key_id_;
    owned_config.manifest_authentication_key.generation = manifest_key_generation_;
    owned_config.store_uuid = bootstrap_body_.store_uuid;
    owned_config.namespace_id = bootstrap_body_.namespace_id;
    owned_config.policy_epoch = bootstrap_body_.policy_epoch;
    owned_config.checkpoint_lineage_id = bootstrap_body_.checkpoint_lineage_id;
    owned_config.manifest_key_generation = bootstrap_body_.manifest_key_generation;
    owned_config.authority_epoch = bootstrap_body_.authority_epoch;
    owned_config.trusted_compatibility = trusted_compatibility_;
    owned_config.protected_registry_authentication_key.key_id = registry_key.key_id;
    owned_config.protected_registry_authentication_key.generation = registry_key.generation;
    const bool secrets_hashed = context_store_sha256_bounded(anchor_key_.data(), anchor_key_size_,
            context_store_master_key_max_bytes, anchor_secret_digest) &&
        context_store_sha256_bounded(admin_key_.data(), admin_key_size_,
            context_store_master_key_max_bytes, admin_secret_digest) &&
        context_store_sha256_bounded(manifest_key_.data(), manifest_key_size_,
            context_store_master_key_max_bytes, manifest_secret_digest);
    const bool scope_hashed = full_scope_message(base_scope, *registry_body, *registry_digest, message) &&
        context_store_sha256_bounded(message.bytes.data(), message.size, message.bytes.size(), authority_scope_commitment_);
    wipe(message.bytes.data(), message.bytes.size()); message.size = 0;
    const bool ok = secrets_hashed && scope_hashed &&
        snapshot_message(owned_config, anchor_secret_digest, admin_secret_digest,
            manifest_secret_digest, *registry_binding, *registry_body, *registry_digest, message) &&
        context_store_hmac_sha256(admin_key_.data(), admin_key_size_, message.bytes.data(), message.size,
            snapshot_commitment_);
    wipe(anchor_secret_digest.data(), anchor_secret_digest.size());
    wipe(admin_secret_digest.data(), admin_secret_digest.size());
    wipe(manifest_secret_digest.data(), manifest_secret_digest.size());
    wipe(base_scope.data(), base_scope.size());
    wipe(message.bytes.data(), message.bytes.size());
    wipe(&owned_config, sizeof(owned_config));
    valid_ = ok;
}

context_store_bootstrap_authority::~context_store_bootstrap_authority() noexcept {
    wipe(anchor_key_.data(), anchor_key_.size());
    wipe(admin_key_.data(), admin_key_.size());
    wipe(manifest_key_.data(), manifest_key_.size());
    wipe(&anchor_key_id_, sizeof(anchor_key_id_));
    wipe(&admin_key_id_, sizeof(admin_key_id_));
    wipe(&manifest_key_id_, sizeof(manifest_key_id_));
    wipe(snapshot_commitment_.data(), snapshot_commitment_.size());
    wipe(authority_scope_commitment_.data(), authority_scope_commitment_.size());
    wipe(&protected_registry_id_, sizeof(protected_registry_id_));
    wipe(protected_registry_snapshot_digest_.data(), protected_registry_snapshot_digest_.size());
    wipe(protected_registry_policy_digest_.data(), protected_registry_policy_digest_.size());
    wipe(&bootstrap_body_, sizeof(bootstrap_body_));
    wipe(&trusted_compatibility_, sizeof(trusted_compatibility_));
    anchor_key_generation_ = admin_key_generation_ = manifest_key_generation_ = 0;
    protected_registry_epoch_ = expected_authorization_sequence_ = 0;
    anchor_key_size_ = admin_key_size_ = manifest_key_size_ = 0;
    valid_ = false;
}

context_store_bootstrap_result context_store_bootstrap_authority::plan(
        const context_store_bootstrap_request & request) const noexcept {
    context_store_bootstrap_result result;
    if (!valid_) return result;
    if (request.manifest_data == nullptr || request.manifest_size == 0 ||
        request.manifest_size > context_store_manifest_max_bytes ||
        request.authorization_token_data == nullptr || request.authorization_token_size == 0 ||
        request.authorization_token_size > context_store_bootstrap_token_max_bytes) {
        result.status = context_store_bootstrap_status::invalid_request;
        return result;
    }

    context_store_bootstrap_admin_key_record admin_policy;
    admin_policy.disposition = context_store_key_disposition::active;
    admin_policy.key_id = admin_key_id_;
    admin_policy.generation = admin_key_generation_;
    admin_policy.master_key = { admin_key_.data(), admin_key_size_ };
    const auto token_result = context_store_verify_bootstrap_token_v1(
        request.authorization_token_data, request.authorization_token_size, admin_policy);
    const auto * token = token_result.authenticated_carrier();
    const auto * token_body = token == nullptr ? nullptr : token->body();
    const auto * token_digest = token == nullptr ? nullptr : token->envelope_digest();
    if (token_body == nullptr || token_digest == nullptr ||
        token_body->store_uuid != bootstrap_body_.store_uuid ||
        token_body->namespace_id != bootstrap_body_.namespace_id ||
        token_body->policy_epoch != bootstrap_body_.policy_epoch ||
        token_body->checkpoint_lineage_id != bootstrap_body_.checkpoint_lineage_id ||
        !same_id(token_body->manifest_key_id, manifest_key_id_) ||
        token_body->manifest_key_generation != manifest_key_generation_ ||
        token_body->compatibility_root != trusted_compatibility_.root ||
        token_body->authority_epoch != bootstrap_body_.authority_epoch ||
        !same_id(token_body->anchor_key_id, anchor_key_id_) ||
        token_body->anchor_key_generation != anchor_key_generation_ ||
        token_body->authority_scope_commitment != authority_scope_commitment_ ||
        !same_id(token_body->protected_registry_id, protected_registry_id_) ||
        token_body->protected_registry_epoch != protected_registry_epoch_ ||
        token_body->protected_registry_snapshot_digest != protected_registry_snapshot_digest_ ||
        token_body->protected_registry_policy_digest != protected_registry_policy_digest_ ||
        token_body->authorization_sequence != expected_authorization_sequence_) {
        result.status = context_store_bootstrap_status::authorization_rejected;
        return result;
    }

    const auto parsed = context_store_parse_manifest_v1(request.manifest_data, request.manifest_size);
    context_store_format_digest selected_manifest_digest {};
    if (parsed.status != context_store_manifest_parse_status::structural_only ||
        !context_store_manifest_digest_v1(request.manifest_data, request.manifest_size,
            selected_manifest_digest) || selected_manifest_digest != token_body->selected_manifest_digest) {
        result.status = context_store_bootstrap_status::manifest_rejected;
        return result;
    }
    context_store_manifest_verification_policy verification_policy;
    verification_policy.key.disposition = context_store_key_disposition::active;
    verification_policy.key.key_id = manifest_key_id_;
    verification_policy.key.generation = manifest_key_generation_;
    verification_policy.key.master_key = { manifest_key_.data(), manifest_key_size_ };
    verification_policy.anchor.store_uuid = bootstrap_body_.store_uuid;
    verification_policy.anchor.checkpoint_lineage_id = bootstrap_body_.checkpoint_lineage_id;
    verification_policy.anchor.namespace_id = bootstrap_body_.namespace_id;
    verification_policy.anchor.policy_epoch = bootstrap_body_.policy_epoch;
    verification_policy.anchor.key_generation = bootstrap_body_.manifest_key_generation;
    verification_policy.anchor.generation = 1;
    verification_policy.anchor.has_predecessor = false;
    verification_policy.anchor.predecessor_manifest_digest.fill(0);
    verification_policy.anchor.selected_manifest_digest = selected_manifest_digest;
    verification_policy.compatibility = trusted_compatibility_;
    const auto verified = context_store_verify_manifest_v1(
        request.manifest_data, request.manifest_size, verification_policy);
    if (verified.status != context_store_manifest_verify_status::authenticated_unadmitted ||
        !verified.has_authenticated_carrier() || verified.authenticated_object_count() == 0 ||
        verified.authenticated_object_count() > context_store_manifest_max_objects) {
        wipe(selected_manifest_digest.data(), selected_manifest_digest.size());
        result.status = context_store_bootstrap_status::manifest_rejected;
        return result;
    }
    const size_t object_count = verified.authenticated_object_count();

    auto body = bootstrap_body_;
    body.selected_manifest_digest = selected_manifest_digest;
    context_store_anchor_key_record signing_key;
    signing_key.disposition = context_store_key_disposition::active;
    signing_key.key_id = anchor_key_id_;
    signing_key.generation = anchor_key_generation_;
    signing_key.master_key = { anchor_key_.data(), anchor_key_size_ };
    std::array<uint8_t, context_store_anchor_max_bytes> encoded {};
    const auto signed_anchor = context_store_encode_anchor_v1(body, signing_key, encoded.data(), encoded.size());
    if (signed_anchor.status != context_store_anchor_status::authenticated_unadmitted ||
        signed_anchor.authenticated_carrier() == nullptr) {
        wipe(encoded.data(), encoded.size());
        wipe(selected_manifest_digest.data(), selected_manifest_digest.size());
        result.status = context_store_bootstrap_status::signing_failed;
        return result;
    }

    bounded_buffer message;
    context_store_format_digest authorization {};
    const auto * anchor_digest = signed_anchor.authenticated_carrier()->envelope_digest();
    const bool authorized = anchor_digest != nullptr &&
        plan_message(snapshot_commitment_, token_body->command_id, token_body->authorization_sequence,
            object_count, selected_manifest_digest, *token_digest, *anchor_digest, message) &&
        context_store_hmac_sha256(admin_key_.data(), admin_key_size_, message.bytes.data(), message.size,
            authorization);
    wipe(message.bytes.data(), message.bytes.size());
    wipe(encoded.data(), encoded.size());
    if (!authorized) {
        wipe(authorization.data(), authorization.size());
        wipe(selected_manifest_digest.data(), selected_manifest_digest.size());
        result.status = context_store_bootstrap_status::signing_failed;
        return result;
    }

    result.plan_.command_id_ = token_body->command_id;
    result.plan_.authorization_sequence_ = token_body->authorization_sequence;
    result.plan_.object_count_ = object_count;
    result.plan_.selected_manifest_digest_ = selected_manifest_digest;
    wipe(selected_manifest_digest.data(), selected_manifest_digest.size());
    result.plan_.authority_snapshot_commitment_ = snapshot_commitment_;
    result.plan_.authorization_token_digest_ = *token_digest;
    result.plan_.plan_commitment_ = authorization;
    wipe(authorization.data(), authorization.size());
    result.plan_.bootstrap_admin_key_id_ = admin_key_id_;
    result.plan_.bootstrap_admin_key_generation_ = admin_key_generation_;
    result.plan_.anchor_ = *signed_anchor.authenticated_carrier();
    result.plan_.authorized_ = true;
    result.status = context_store_bootstrap_status::authorized_unexecuted;
    return result;
}

const char * context_store_bootstrap_status_name(context_store_bootstrap_status status) noexcept {
    switch (status) {
        case context_store_bootstrap_status::authorized_unexecuted: return "authorized-unexecuted";
        case context_store_bootstrap_status::invalid_authority: return "invalid-authority";
        case context_store_bootstrap_status::invalid_request: return "invalid-request";
        case context_store_bootstrap_status::authorization_rejected: return "authorization-rejected";
        case context_store_bootstrap_status::manifest_rejected: return "manifest-rejected";
        case context_store_bootstrap_status::signing_failed: return "signing-failed";
    }
    return "unknown";
}

} // namespace halofpx
