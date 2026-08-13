#include "halofpx-context-store-v1-catalog.h"
#if defined(HALOFPX_CONTEXT_STORE_WORLD1_PREFIX_PRODUCT)
#include "halofpx-context-store-world1-prefix-product-v1.h"
#endif

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

#include <algorithm>
#include <array>
#include <cerrno>
#include <filesystem>
#include <fcntl.h>
#include <map>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {

void create_lock(const std::filesystem::path & path) {
    const int fd = ::open(path.c_str(), O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    assert(fd >= 0);
    assert(::fsync(fd) == 0);
    assert(::close(fd) == 0);
}

void create_child_layout(const std::filesystem::path & data,
                         const std::filesystem::path & anchor) {
    assert(::mkdir(data.c_str(), 0700) == 0);
    assert(::mkdir(anchor.c_str(), 0700) == 0);
    assert(::mkdir((data / "staging").c_str(), 0700) == 0);
    assert(::mkdir((data / "objects").c_str(), 0700) == 0);
    assert(::mkdir((data / "manifests").c_str(), 0700) == 0);
    create_lock(data / "writer.lock");
    create_lock(anchor / "writer.lock");
}

class catalog_roots {
public:
    catalog_roots() {
        std::array<char, 64> pattern {};
        const char value[] = "/tmp/halofpx-v1-catalog-XXXXXX";
        std::copy(value, value + sizeof(value), pattern.begin());
        const char * created = ::mkdtemp(pattern.data());
        assert(created != nullptr);
        base = created;
        catalog = base / "catalog";
        assert(::chmod(base.c_str(), 0700) == 0);
        assert(::mkdir(catalog.c_str(), 0700) == 0);
        create_lock(catalog / "writer.lock");
        for (size_t i = 0; i != 2; ++i) {
            data[i] = base / ("data-" + std::to_string(i));
            anchor[i] = base / ("anchor-" + std::to_string(i));
            create_child_layout(data[i], anchor[i]);
            data_text[i] = data[i].string();
            anchor_text[i] = anchor[i].string();
            slots[i] = { data_text[i].c_str(), anchor_text[i].c_str() };
        }
        catalog_text = catalog.string();
    }

    ~catalog_roots() {
        std::error_code ignored;
        std::filesystem::remove_all(base, ignored);
    }

    catalog_roots(const catalog_roots &) = delete;
    catalog_roots & operator=(const catalog_roots &) = delete;

    std::filesystem::path base;
    std::filesystem::path catalog;
    std::array<std::filesystem::path, 2> data;
    std::array<std::filesystem::path, 2> anchor;
    std::string catalog_text;
    std::array<std::string, 2> data_text;
    std::array<std::string, 2> anchor_text;
    std::array<halofpx::context_store_v1_catalog_slot_config, 2> slots {};
};

void cbor_head(std::vector<uint8_t> & out, uint8_t major, uint64_t value) {
    assert(value <= 0xff);
    if (value < 24) {
        out.push_back(static_cast<uint8_t>((major << 5) | value));
    } else {
        out.push_back(static_cast<uint8_t>((major << 5) | 24));
        out.push_back(static_cast<uint8_t>(value));
    }
}

void bind_compatibility(halofpx::context_store_v1_server_canary_config & config) {
    std::vector<uint8_t> encoded;
    cbor_head(encoded, 5, config.compatibility.components.size());
    for (size_t i = 0; i < config.compatibility.components.size(); ++i) {
        cbor_head(encoded, 0, i);
        cbor_head(encoded, 2, config.compatibility.components[i].size());
        encoded.insert(encoded.end(), config.compatibility.components[i].begin(),
                       config.compatibility.components[i].end());
    }
    static constexpr char domain[] = "halofpx.compat.v1";
    std::vector<uint8_t> preimage(reinterpret_cast<const uint8_t *>(domain),
                                  reinterpret_cast<const uint8_t *>(domain) + sizeof(domain));
    preimage.insert(preimage.end(), encoded.begin(), encoded.end());
    assert(halofpx::context_store_sha256(preimage.data(), preimage.size(),
                                        config.compatibility.root));
}

halofpx::context_store_v1_catalog_config make_config(
        catalog_roots & roots, const std::array<uint8_t, 32> & operator_key) {
    halofpx::context_store_v1_catalog_config config;
    config.catalog_root_path = roots.catalog_text.c_str();
    config.slots = roots.slots.data();
    config.slot_count = roots.slots.size();
    config.child.operator_key = { operator_key.data(), operator_key.size() };
    config.child.store_uuid.fill(0x11);
    for (size_t i = 0; i < config.child.compatibility.components.size(); ++i) {
        config.child.compatibility.components[i].fill(static_cast<uint8_t>(0x20 + i));
    }
    bind_compatibility(config.child);
    config.child.producer_identity.fill(0x31);
    config.child.global_plan_digest.fill(0x32);
    config.child.rank_ownership_digest.fill(0x33);
    config.child.rank_placement_digest.fill(0x34);
    config.child.topology_epoch = 1;
    config.child.quota_bytes = 1024 * 1024;
    config.child.max_entries = 1;
    config.child.limits = { { 64, 16 }, 4096, 65536 };
    return config;
}

halofpx::context_store_transformer_profile_v1 admitted_profile() {
    halofpx::context_store_transformer_profile_v1 profile;
    profile.target_only = true;
    profile.world_size = 1;
    profile.rank = 0;
    profile.architecture = halofpx::context_store_transformer_architecture_v1::transformer;
    profile.greedy_memoryless_sampling = true;
    return profile;
}

halofpx::context_store_transformer_snapshot_v1 snapshot_for(
        const halofpx::context_store_v1_catalog_config & config, uint8_t identity_byte,
        std::initializer_list<llama_token> tokens, std::initializer_list<uint8_t> state) {
    halofpx::context_store_transformer_snapshot_v1 snapshot;
    snapshot.compatibility_identity.compatibility_root = config.child.compatibility.root;
    snapshot.compatibility_identity.scope_namespace.fill(0x51);
    snapshot.compatibility_identity.checkpoint_lineage_id.fill(identity_byte);
    snapshot.compatibility_identity.policy_epoch = 1;
    snapshot.profile = admitted_profile();
    snapshot.tokens.assign(tokens);
    snapshot.state.assign(state);
    return snapshot;
}

using tree_state = std::map<std::string, uintmax_t>;

tree_state inspect_tree(const std::filesystem::path & base) {
    tree_state result;
    for (const auto & entry : std::filesystem::recursive_directory_iterator(base)) {
        const auto relative = std::filesystem::relative(entry.path(), base).generic_string();
        result.emplace(relative, entry.is_regular_file() ? entry.file_size() : UINTMAX_MAX);
    }
    return result;
}

void flip_first_byte(const std::filesystem::path & path) {
    const int fd = ::open(path.c_str(), O_RDWR | O_CLOEXEC | O_NOFOLLOW);
    assert(fd >= 0);
    uint8_t byte = 0;
    ssize_t count;
    do count = ::pread(fd, &byte, 1, 0); while (count < 0 && errno == EINTR);
    assert(count == 1);
    byte ^= 1;
    do count = ::pwrite(fd, &byte, 1, 0); while (count < 0 && errno == EINTR);
    assert(count == 1);
    assert(::fsync(fd) == 0);
    assert(::close(fd) == 0);
}

std::array<uint8_t, 12> read_record_prefix_metadata(
        const std::filesystem::path & path) {
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    assert(fd >= 0);
    std::array<uint8_t, 12> bytes {};
    size_t offset = 0;
    while (offset != bytes.size()) {
        const ssize_t count = ::pread(fd, bytes.data() + offset,
            bytes.size() - offset, static_cast<off_t>(452 + offset));
        if (count < 0 && errno == EINTR) continue;
        assert(count > 0);
        offset += static_cast<size_t>(count);
    }
    assert(::close(fd) == 0);
    return bytes;
}

void assert_hit(halofpx::context_store_v1_catalog & catalog,
                const halofpx::context_store_transformer_snapshot_v1 & expected) {
    const auto restored = catalog.restore_exact(
        expected.tokens.data(), expected.tokens.size(), expected.compatibility_identity,
        expected.profile);
    assert(restored.status == halofpx::context_store_v1_catalog_status::hit);
    assert(restored.snapshot.tokens == expected.tokens);
    assert(restored.snapshot.state == expected.state);
}

void test_two_restart_hits_misses_and_capacity() {
    catalog_roots roots;
    std::array<uint8_t, 32> operator_key {};
    operator_key.fill(0x41);
    auto config = make_config(roots, operator_key);
    const auto first = snapshot_for(config, 0x61, { 7, 11, 13 }, { 0xa1, 0xb2 });
    const auto second = snapshot_for(config, 0x62, { 17, 19, 23 }, { 0xc3, 0xd4, 0xe5 });

    auto opened = halofpx::make_context_store_v1_catalog(config);
    assert(opened.status == halofpx::context_store_v1_catalog_status::ready);
    assert(opened.catalog != nullptr);
    assert(opened.catalog->publish(first).status ==
           halofpx::context_store_v1_catalog_status::published);
    assert(opened.catalog->publish(second).status ==
           halofpx::context_store_v1_catalog_status::published);
    opened.catalog.reset();

    opened = halofpx::make_context_store_v1_catalog(config);
    assert(opened.status == halofpx::context_store_v1_catalog_status::ready);
    assert(opened.catalog != nullptr);
    assert_hit(*opened.catalog, first);
    assert_hit(*opened.catalog, second);

    auto changed_tokens = first.tokens;
    changed_tokens.back() += 1;
    const auto changed = opened.catalog->restore_exact(
        changed_tokens.data(), changed_tokens.size(), first.compatibility_identity,
        first.profile);
    assert(changed.status != halofpx::context_store_v1_catalog_status::hit);

    auto changed_identity = first.compatibility_identity;
    changed_identity.checkpoint_lineage_id[0] ^= 1;
    const auto changed_key = opened.catalog->restore_exact(
        first.tokens.data(), first.tokens.size(), changed_identity, first.profile);
    assert(changed_key.status == halofpx::context_store_v1_catalog_status::capacity_exhausted);

    auto wrong_scope = first.compatibility_identity;
    wrong_scope.scope_namespace[0] ^= 1;
    const auto scoped = opened.catalog->restore_exact(
        first.tokens.data(), first.tokens.size(), wrong_scope, first.profile);
    assert(scoped.status == halofpx::context_store_v1_catalog_status::capacity_exhausted);

    const auto before = inspect_tree(roots.base);
    const auto third = snapshot_for(config, 0x63, { 29, 31 }, { 0xf6 });
    assert(opened.catalog->publish(third).status ==
           halofpx::context_store_v1_catalog_status::capacity_exhausted);
    assert(inspect_tree(roots.base) == before);
    assert_hit(*opened.catalog, first);
    assert_hit(*opened.catalog, second);
}

void test_catalog_final_tamper_is_safe_miss() {
    catalog_roots roots;
    std::array<uint8_t, 32> operator_key {};
    operator_key.fill(0x41);
    auto config = make_config(roots, operator_key);
    const auto snapshot = snapshot_for(config, 0x61, { 3, 5 }, { 0x71, 0x72 });
    auto opened = halofpx::make_context_store_v1_catalog(config);
    assert(opened.catalog != nullptr);
    assert(opened.catalog->publish(snapshot).status ==
           halofpx::context_store_v1_catalog_status::published);
    flip_first_byte(roots.catalog / "slot-00.final.v1");
    const auto restored = opened.catalog->restore_exact(
        snapshot.tokens.data(), snapshot.tokens.size(), snapshot.compatibility_identity,
        snapshot.profile);
    assert(restored.status == halofpx::context_store_v1_catalog_status::miss_corrupt);
    assert(restored.snapshot.tokens.empty() && restored.snapshot.state.empty());
}

void test_catalog_root_transplant_and_pending_duplicates_are_safe_misses() {
    catalog_roots roots;
    std::array<uint8_t, 32> operator_key {};
    operator_key.fill(0x41);
    auto config = make_config(roots, operator_key);
    const auto snapshot = snapshot_for(config, 0x61, { 17, 19 }, { 0x74, 0x75 });
    auto opened = halofpx::make_context_store_v1_catalog(config);
    assert(opened.catalog != nullptr);
    assert(opened.catalog->publish(snapshot).status ==
           halofpx::context_store_v1_catalog_status::published);
    opened.catalog.reset();

    const auto alternate = roots.base / "alternate-catalog";
    assert(::mkdir(alternate.c_str(), 0700) == 0);
    create_lock(alternate / "writer.lock");
    std::filesystem::copy_file(roots.catalog / "slot-00.reserve.v1",
                               alternate / "slot-00.reserve.v1");
    std::filesystem::copy_file(roots.catalog / "slot-00.final.v1",
                               alternate / "slot-00.final.v1");
    const std::string alternate_text = alternate.string();
    config.catalog_root_path = alternate_text.c_str();
    auto transplanted = halofpx::make_context_store_v1_catalog(config);
    assert(transplanted.catalog != nullptr);
    auto restored = transplanted.catalog->restore_exact(
        snapshot.tokens.data(), snapshot.tokens.size(), snapshot.compatibility_identity,
        snapshot.profile);
    assert(restored.status == halofpx::context_store_v1_catalog_status::miss_corrupt);
    assert(restored.snapshot.tokens.empty() && restored.snapshot.state.empty());
    transplanted.catalog.reset();

    config.catalog_root_path = roots.catalog_text.c_str();
    std::filesystem::copy_file(roots.catalog / "slot-00.final.v1",
                               roots.catalog / "slot-00.final-pending.v1");
    opened = halofpx::make_context_store_v1_catalog(config);
    assert(opened.catalog != nullptr);
    restored = opened.catalog->restore_exact(
        snapshot.tokens.data(), snapshot.tokens.size(), snapshot.compatibility_identity,
        snapshot.profile);
    assert(restored.status == halofpx::context_store_v1_catalog_status::miss_corrupt);
    assert(restored.snapshot.tokens.empty() && restored.snapshot.state.empty());
}

void test_corrupt_child_is_safe_miss() {
    catalog_roots roots;
    std::array<uint8_t, 32> operator_key {};
    operator_key.fill(0x41);
    auto config = make_config(roots, operator_key);
    const auto snapshot = snapshot_for(config, 0x61, { 37, 41 }, { 0x81, 0x82 });
    auto opened = halofpx::make_context_store_v1_catalog(config);
    assert(opened.catalog != nullptr);
    assert(opened.catalog->publish(snapshot).status ==
           halofpx::context_store_v1_catalog_status::published);

    std::filesystem::path object;
    for (const auto & entry : std::filesystem::directory_iterator(roots.data[0] / "objects")) {
        if (entry.is_regular_file()) {
            object = entry.path();
            break;
        }
    }
    assert(!object.empty());
    flip_first_byte(object);
    const auto restored = opened.catalog->restore_exact(
        snapshot.tokens.data(), snapshot.tokens.size(), snapshot.compatibility_identity,
        snapshot.profile);
    assert(restored.status != halofpx::context_store_v1_catalog_status::hit);
    assert(restored.snapshot.tokens.empty() && restored.snapshot.state.empty());
}

void test_unexpected_catalog_layout_is_safe_miss() {
    catalog_roots roots;
    std::array<uint8_t, 32> operator_key {};
    operator_key.fill(0x41);
    auto config = make_config(roots, operator_key);
    const auto snapshot = snapshot_for(config, 0x61, { 43, 47 }, { 0x91, 0x92 });
    auto opened = halofpx::make_context_store_v1_catalog(config);
    assert(opened.catalog != nullptr);
    assert(opened.catalog->publish(snapshot).status ==
           halofpx::context_store_v1_catalog_status::published);

    const auto unknown = roots.catalog / "unexpected.v1";
    const int unknown_fd = ::open(unknown.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    assert(unknown_fd >= 0);
    assert(::close(unknown_fd) == 0);
    auto restored = opened.catalog->restore_exact(
        snapshot.tokens.data(), snapshot.tokens.size(), snapshot.compatibility_identity,
        snapshot.profile);
    assert(restored.status == halofpx::context_store_v1_catalog_status::miss_corrupt);
    assert(restored.snapshot.tokens.empty() && restored.snapshot.state.empty());
    assert(::unlink(unknown.c_str()) == 0);

    assert(::link((roots.catalog / "slot-00.final.v1").c_str(),
                  (roots.catalog / "slot-01.final.v1").c_str()) == 0);
    restored = opened.catalog->restore_exact(
        snapshot.tokens.data(), snapshot.tokens.size(), snapshot.compatibility_identity,
        snapshot.profile);
    assert(restored.status == halofpx::context_store_v1_catalog_status::miss_corrupt);
    assert(restored.snapshot.tokens.empty() && restored.snapshot.state.empty());
}

void test_prefix_token_counts_are_authenticated_scoped_and_restart_stable() {
    catalog_roots roots;
    std::array<uint8_t, 32> operator_key {};
    operator_key.fill(0x41);
    auto config = make_config(roots, operator_key);
    const auto shorter = snapshot_for(config, 0x61, { 2, 3 }, { 0xa1, 0xa2 });
    const auto longer = snapshot_for(config, 0x62, { 2, 3, 5, 7 }, { 0xb1, 0xb2 });

    auto opened = halofpx::make_context_store_v1_catalog(config);
    assert(opened.catalog != nullptr);
    assert(opened.catalog->publish(shorter).status ==
           halofpx::context_store_v1_catalog_status::published);
    assert(opened.catalog->publish(longer).status ==
           halofpx::context_store_v1_catalog_status::published);

    halofpx::context_store_v1_catalog_prefix_query query;
    query.compatibility_root = shorter.compatibility_identity.compatibility_root;
    query.scope_namespace = shorter.compatibility_identity.scope_namespace;
    query.policy_epoch = shorter.compatibility_identity.policy_epoch;
    query.max_token_count = 8;
    query.profile = shorter.profile;
    auto discovered = opened.catalog->discover_prefix_token_counts(query);
    assert(discovered.status == halofpx::context_store_v1_catalog_status::ready);
    assert(discovered.token_count == 2);
    assert(discovered.token_counts[0] == 2);
    assert(discovered.token_counts[1] == 4);

    auto mismatch = query;
    mismatch.scope_namespace[0] ^= 1;
    discovered = opened.catalog->discover_prefix_token_counts(mismatch);
    assert(discovered.status == halofpx::context_store_v1_catalog_status::miss_not_found);
    assert(discovered.token_count == 0);
    mismatch = query;
    mismatch.compatibility_root[0] ^= 1;
    discovered = opened.catalog->discover_prefix_token_counts(mismatch);
    assert(discovered.status == halofpx::context_store_v1_catalog_status::miss_not_found);
    mismatch = query;
    ++mismatch.policy_epoch;
    discovered = opened.catalog->discover_prefix_token_counts(mismatch);
    assert(discovered.status == halofpx::context_store_v1_catalog_status::miss_not_found);

    opened.catalog.reset();
    opened = halofpx::make_context_store_v1_catalog(config);
    assert(opened.catalog != nullptr);
    discovered = opened.catalog->discover_prefix_token_counts(query);
    assert(discovered.status == halofpx::context_store_v1_catalog_status::ready);
    assert(discovered.token_count == 2);
    assert(discovered.token_counts[0] == 2);
    assert(discovered.token_counts[1] == 4);

    flip_first_byte(roots.catalog / "slot-01.final.v1");
    discovered = opened.catalog->discover_prefix_token_counts(query);
    assert(discovered.status == halofpx::context_store_v1_catalog_status::miss_corrupt);
    assert(discovered.token_count == 0);
}

void test_exact_catalog_retains_zero_reserved_bytes_and_supports_manifest_discovery() {
    catalog_roots roots;
    std::array<uint8_t, 32> operator_key {};
    operator_key.fill(0x41);
    auto config = make_config(roots, operator_key);
    const auto snapshot = snapshot_for(config, 0x61, { 11, 13 }, { 0xc1, 0xc2 });
    auto opened = halofpx::make_context_store_v1_catalog(config);
    assert(opened.catalog != nullptr);
    assert(opened.catalog->publish(snapshot).status ==
           halofpx::context_store_v1_catalog_status::published);
    assert_hit(*opened.catalog, snapshot);

    halofpx::context_store_v1_catalog_prefix_query query;
    query.compatibility_root = snapshot.compatibility_identity.compatibility_root;
    query.scope_namespace = snapshot.compatibility_identity.scope_namespace;
    query.policy_epoch = snapshot.compatibility_identity.policy_epoch;
    query.max_token_count = snapshot.tokens.size();
    query.profile = snapshot.profile;
    const auto discovered = opened.catalog->discover_prefix_token_counts(query);
    assert(discovered.status == halofpx::context_store_v1_catalog_status::ready);
    assert(discovered.token_count == 1);
    assert(discovered.token_counts[0] == snapshot.tokens.size());
    const auto reservation = read_record_prefix_metadata(
        roots.catalog / "slot-00.reserve.v1");
    const auto final = read_record_prefix_metadata(
        roots.catalog / "slot-00.final.v1");
    assert(std::all_of(reservation.begin(), reservation.end(),
        [](uint8_t byte) { return byte == 0; }));
    assert(std::all_of(final.begin(), final.end(),
        [](uint8_t byte) { return byte == 0; }));
}

void test_authenticated_incomplete_identity_is_terminal() {
    catalog_roots roots;
    std::array<uint8_t, 32> operator_key {};
    operator_key.fill(0x41);
    auto config = make_config(roots, operator_key);
    const auto snapshot = snapshot_for(config, 0x61, { 17, 19, 23 }, { 0xd1, 0xd2 });
    auto opened = halofpx::make_context_store_v1_catalog(config);
    assert(opened.catalog != nullptr);
    assert(opened.catalog->publish(snapshot).status ==
           halofpx::context_store_v1_catalog_status::published);

    // Simulate an interrupted/tampered catalog publication after the
    // authenticated reservation became durable.  Neither exact lookup nor
    // prefix discovery may reinterpret this as a clean miss.
    assert(::unlink((roots.catalog / "slot-00.final.v1").c_str()) == 0);
    const auto restored = opened.catalog->restore_exact(
        snapshot.tokens.data(), snapshot.tokens.size(),
        snapshot.compatibility_identity, snapshot.profile);
    assert(restored.status == halofpx::context_store_v1_catalog_status::miss_corrupt);
    assert(restored.authenticated_record_selected);
    assert(restored.snapshot.tokens.empty() && restored.snapshot.state.empty());

    halofpx::context_store_v1_catalog_prefix_query query;
    query.compatibility_root = snapshot.compatibility_identity.compatibility_root;
    query.scope_namespace = snapshot.compatibility_identity.scope_namespace;
    query.policy_epoch = snapshot.compatibility_identity.policy_epoch;
    query.max_token_count = snapshot.tokens.size();
    query.profile = snapshot.profile;
    const auto discovered = opened.catalog->discover_prefix_token_counts(query);
    assert(discovered.status == halofpx::context_store_v1_catalog_status::miss_corrupt);
    assert(discovered.token_count == 0);
}

#if defined(HALOFPX_CONTEXT_STORE_WORLD1_PREFIX_PRODUCT)
class fake_product_state_api final
    : public halofpx::context_store_transformer_state_api_v1 {
public:
    mutable size_t set_calls = 0;
    mutable llama_context * last_context = nullptr;
    mutable llama_seq_id last_sequence = -1;
    mutable llama_state_seq_flags last_flags = UINT32_MAX;
    mutable std::vector<uint8_t> restored_state;
    size_t restore_return = 0;

    size_t get_size(llama_context *, llama_seq_id,
                    llama_state_seq_flags) const noexcept override {
        return 0;
    }
    size_t get_data(llama_context *, uint8_t *, size_t,
                    llama_seq_id, llama_state_seq_flags) const noexcept override {
        return 0;
    }
    size_t set_data(llama_context * context, const uint8_t * state, size_t size,
                    llama_seq_id sequence,
                    llama_state_seq_flags flags) const noexcept override {
        ++set_calls;
        last_context = context;
        last_sequence = sequence;
        last_flags = flags;
        restored_state.assign(state, state + size);
        return restore_return == 0 ? size : restore_return;
    }
};

llama_context * fake_product_context() {
    return reinterpret_cast<llama_context *>(static_cast<uintptr_t>(0x4321));
}

halofpx::context_store_world1_cache_authority_v1 authority_for(
        const halofpx::context_store_v1_catalog_config & config) {
    halofpx::context_store_world1_cache_authority_v1 authority;
    authority.compatibility = config.child.compatibility;
    authority.producer_identity = config.child.producer_identity;
    authority.global_plan_digest = config.child.global_plan_digest;
    authority.rank_ownership_digest = config.child.rank_ownership_digest;
    authority.rank_placement_digest = config.child.rank_placement_digest;
    authority.topology_epoch = config.child.topology_epoch;
    authority.model_generation = 7;
    authority.world_size = 1;
    authority.rank = 0;
    return authority;
}

halofpx::context_store_exact_session_inputs_v1 exact_for(
        const halofpx::context_store_world1_cache_authority_v1 & authority,
        const std::array<uint8_t, 32> & derivation_key,
        const halofpx::context_store_format_digest & scope,
        const std::vector<llama_token> & tokens) {
    halofpx::context_store_exact_session_inputs_v1 exact;
    exact.derivation_key = { derivation_key.data(), derivation_key.size() };
    exact.scope_namespace = scope;
    exact.compatibility_root = authority.compatibility.root;
    exact.tokens = tokens.data();
    exact.token_count = tokens.size();
    exact.logical_boundary = tokens.size();
    exact.output_boundary = tokens.size();
    exact.profile = halofpx::context_store_exact_session_profile_v1::
        target_only_greedy_memoryless;
    exact.global_plan_digest = authority.global_plan_digest;
    exact.rank_ownership_digest = authority.rank_ownership_digest;
    exact.rank_placement_digest = authority.rank_placement_digest;
    exact.topology_epoch = authority.topology_epoch;
    exact.world_size = 1;
    exact.rank = 0;
    return exact;
}

halofpx::context_store_transformer_snapshot_v1 exact_snapshot(
        const halofpx::context_store_world1_cache_authority_v1 & authority,
        const std::array<uint8_t, 32> & derivation_key,
        const halofpx::context_store_format_digest & scope,
        std::vector<llama_token> tokens,
        std::initializer_list<uint8_t> state) {
    const auto exact = exact_for(authority, derivation_key, scope, tokens);
    const auto resolved = halofpx::context_store_resolve_exact_session_v1(exact);
    assert(resolved.resolved());
    halofpx::context_store_transformer_snapshot_v1 snapshot;
    snapshot.compatibility_identity.compatibility_root = authority.compatibility.root;
    snapshot.compatibility_identity.scope_namespace = scope;
    snapshot.compatibility_identity.checkpoint_lineage_id = resolved.session_id;
    snapshot.compatibility_identity.policy_epoch = 1;
    snapshot.profile = admitted_profile();
    snapshot.tokens = std::move(tokens);
    snapshot.state.assign(state);
    return snapshot;
}

halofpx::context_store_world1_prefix_lookup_result_v1 product_lookup(
        halofpx::context_store_v1_catalog * catalog,
        const halofpx::context_store_world1_cache_authority_v1 * authority,
        const std::array<uint8_t, 32> & derivation_key,
        const halofpx::context_store_format_digest & scope,
        const std::vector<llama_token> & tokens,
        bool enabled = true,
        uint64_t expected_generation = 7) {
    halofpx::context_store_world1_prefix_lookup_request_v1 request;
    request.enabled = enabled;
    request.authority = authority;
    request.expected_model_generation = expected_generation;
    request.catalog = catalog;
    if (authority) {
        request.exact_session = exact_for(*authority, derivation_key, scope, tokens);
    } else {
        request.exact_session.tokens = tokens.data();
        request.exact_session.token_count = tokens.size();
        request.exact_session.logical_boundary = tokens.size();
        request.exact_session.output_boundary = tokens.size();
    }
    request.policy_epoch = 1;
    request.profile = admitted_profile();
    return halofpx::context_store_world1_prefix_lookup_v1(request);
}

void test_product_feature_off_and_live_authority_unavailable_are_cold() {
    std::array<uint8_t, 32> key {};
    key.fill(0x91);
    halofpx::context_store_format_digest scope {};
    scope.fill(0x51);
    const std::vector<llama_token> tokens { 2, 3, 5 };
    auto result = product_lookup(nullptr, nullptr, key, scope, tokens, false);
    assert(!result.hit());
    assert(result.fallback ==
        halofpx::context_store_world1_prefix_fallback_v1::feature_off);
    result = product_lookup(nullptr, nullptr, key, scope, tokens, true);
    assert(!result.hit());
    assert(result.residual_tokens == tokens.size());
    assert(result.fallback ==
        halofpx::context_store_world1_prefix_fallback_v1::live_authority_unavailable);
}

void test_product_same_system_prefix_different_suffix_and_exact_hit() {
    catalog_roots roots;
    std::array<uint8_t, 32> operator_key {};
    operator_key.fill(0x41);
    auto config = make_config(roots, operator_key);
    auto authority = authority_for(config);
    std::array<uint8_t, 32> derivation_key {};
    derivation_key.fill(0x91);
    halofpx::context_store_format_digest scope {};
    scope.fill(0x51);
    const std::vector<llama_token> shared_system { 101, 102, 103 };
    const std::vector<llama_token> request_a { 101, 102, 103, 201 };
    const std::vector<llama_token> request_b { 101, 102, 103, 202 };
    const auto prefix = exact_snapshot(
        authority, derivation_key, scope, shared_system, { 0xa1, 0xa2 });
    const auto exact = exact_snapshot(
        authority, derivation_key, scope, request_a, { 0xb1, 0xb2 });
    auto opened = halofpx::make_context_store_v1_catalog(config);
    assert(opened.catalog != nullptr);
    assert(opened.catalog->publish(prefix).status ==
        halofpx::context_store_v1_catalog_status::published);

    auto result = product_lookup(
        opened.catalog.get(), &authority, derivation_key, scope, request_a);
    assert(result.hit());
    assert(result.source == halofpx::context_store_world1_prefix_source_v1::prefix);
    assert(result.selected_prefix_tokens == shared_system.size());
    assert(result.restored_tokens == shared_system.size());
    assert(result.residual_tokens == 1);
    fake_product_state_api state_api;
    halofpx::context_store_world1_prefix_install_request_v1 install_request;
    install_request.authority = &authority;
    install_request.expected_model_generation = authority.model_generation;
    install_request.lookup = &result;
    install_request.context = fake_product_context();
    install_request.sequence = 3;
    install_request.full_tokens = request_a.data();
    install_request.full_token_count = request_a.size();
    install_request.profile = admitted_profile();
    install_request.limits = { 64, 16 };
    const auto installed =
        halofpx::context_store_world1_prefix_install_v1_with_api(
            state_api, install_request);
    assert(installed.installed());
    assert(installed.installed_prefix_tokens == shared_system.size());
    assert(installed.residual_tokens == 1);
    assert(state_api.set_calls == 1);
    assert(state_api.last_context == fake_product_context());
    assert(state_api.last_sequence == 3);
    assert(state_api.last_flags == LLAMA_STATE_SEQ_FLAGS_NONE);
    assert((state_api.restored_state == std::vector<uint8_t>{ 0xa1, 0xa2 }));
    assert(result.snapshot.state.empty());
    assert(!result.hit());
    const auto second_install =
        halofpx::context_store_world1_prefix_install_v1_with_api(
            state_api, install_request);
    assert(second_install.status ==
        halofpx::context_store_world1_prefix_install_status_v1::rejected);
    assert(state_api.set_calls == 1);

    result = product_lookup(
        opened.catalog.get(), &authority, derivation_key, scope, request_b);
    assert(result.hit());
    assert(result.source == halofpx::context_store_world1_prefix_source_v1::prefix);
    assert(result.selected_prefix_tokens == shared_system.size());
    assert(result.residual_tokens == 1);

    assert(opened.catalog->publish(exact).status ==
        halofpx::context_store_v1_catalog_status::published);
    result = product_lookup(
        opened.catalog.get(), &authority, derivation_key, scope, request_a);
    assert(result.hit());
    assert(result.source == halofpx::context_store_world1_prefix_source_v1::exact);
    assert(result.selected_prefix_tokens == request_a.size());
    assert(result.residual_tokens == 0);

    state_api = {};
    install_request.lookup = &result;
    const auto exact_installed =
        halofpx::context_store_world1_prefix_install_v1_with_api(
            state_api, install_request);
    assert(exact_installed.installed());
    assert(exact_installed.installed_prefix_tokens == request_a.size());
    assert(exact_installed.residual_tokens == 0);
    assert((state_api.restored_state == std::vector<uint8_t>{ 0xb1, 0xb2 }));

    result = product_lookup(
        opened.catalog.get(), &authority, derivation_key, scope, request_b);
    state_api = {};
    state_api.restore_return = 1;
    install_request.lookup = &result;
    install_request.full_tokens = request_b.data();
    install_request.full_token_count = request_b.size();
    const auto failed_install =
        halofpx::context_store_world1_prefix_install_v1_with_api(
            state_api, install_request);
    assert(failed_install.status ==
        halofpx::context_store_world1_prefix_install_status_v1::state_apply_failed);
    assert(state_api.set_calls == 1);
    assert(result.snapshot.state.empty());
    assert(!result.hit());
}

void test_product_corrupt_longer_candidate_is_terminal_cold() {
    catalog_roots roots;
    std::array<uint8_t, 32> operator_key {};
    operator_key.fill(0x41);
    auto config = make_config(roots, operator_key);
    auto authority = authority_for(config);
    std::array<uint8_t, 32> derivation_key {};
    derivation_key.fill(0x91);
    halofpx::context_store_format_digest scope {};
    scope.fill(0x51);
    const std::vector<llama_token> shorter_tokens { 31, 32 };
    const std::vector<llama_token> longer_tokens { 31, 32, 33, 34 };
    const std::vector<llama_token> request_tokens { 31, 32, 33, 34, 35 };
    const auto shorter = exact_snapshot(
        authority, derivation_key, scope, shorter_tokens, { 0xc1, 0xc2 });
    const auto longer = exact_snapshot(
        authority, derivation_key, scope, longer_tokens, { 0xd1, 0xd2 });
    auto opened = halofpx::make_context_store_v1_catalog(config);
    assert(opened.catalog != nullptr);
    assert(opened.catalog->publish(shorter).status ==
        halofpx::context_store_v1_catalog_status::published);
    assert(opened.catalog->publish(longer).status ==
        halofpx::context_store_v1_catalog_status::published);
    std::filesystem::path object;
    for (const auto & entry :
            std::filesystem::directory_iterator(roots.data[1] / "objects")) {
        if (entry.is_regular_file()) { object = entry.path(); break; }
    }
    assert(!object.empty());
    flip_first_byte(object);
    const auto before = inspect_tree(roots.base);
    const auto result = product_lookup(
        opened.catalog.get(), &authority, derivation_key, scope, request_tokens);
    assert(!result.hit());
    assert(result.fallback ==
        halofpx::context_store_world1_prefix_fallback_v1::authenticated_state_corrupt);
    assert(result.selected_prefix_tokens == 0);
    assert(inspect_tree(roots.base) == before);
}

void test_product_authority_mismatch_and_generation_change_are_cold() {
    catalog_roots roots;
    std::array<uint8_t, 32> operator_key {};
    operator_key.fill(0x41);
    auto config = make_config(roots, operator_key);
    auto authority = authority_for(config);
    std::array<uint8_t, 32> derivation_key {};
    derivation_key.fill(0x91);
    halofpx::context_store_format_digest scope {};
    scope.fill(0x51);
    const std::vector<llama_token> tokens { 7, 8, 9 };
    auto opened = halofpx::make_context_store_v1_catalog(config);
    assert(opened.catalog != nullptr);
    auto result = product_lookup(opened.catalog.get(), &authority,
        derivation_key, scope, tokens, true, authority.model_generation + 1);
    assert(!result.hit());
    assert(result.fallback == halofpx::context_store_world1_prefix_fallback_v1::
        authority_generation_changed);
    const auto exact = exact_for(authority, derivation_key, scope, tokens);
    authority.global_plan_digest[0] ^= 1;
    halofpx::context_store_world1_prefix_lookup_request_v1 request;
    request.enabled = true;
    request.authority = &authority;
    request.expected_model_generation = authority.model_generation;
    request.catalog = opened.catalog.get();
    request.exact_session = exact;
    request.policy_epoch = 1;
    request.profile = admitted_profile();
    result = halofpx::context_store_world1_prefix_lookup_v1(request);
    assert(!result.hit());
    assert(result.fallback ==
        halofpx::context_store_world1_prefix_fallback_v1::invalid_request);

    authority = authority_for(config);
    authority.compatibility.root[0] ^= 1;
    result = product_lookup(opened.catalog.get(), &authority,
        derivation_key, scope, tokens);
    assert(!result.hit());
    assert(result.fallback ==
        halofpx::context_store_world1_prefix_fallback_v1::live_authority_invalid);
}
#endif

} // namespace

int main() {
    test_two_restart_hits_misses_and_capacity();
    test_catalog_final_tamper_is_safe_miss();
    test_catalog_root_transplant_and_pending_duplicates_are_safe_misses();
    test_corrupt_child_is_safe_miss();
    test_unexpected_catalog_layout_is_safe_miss();
    test_prefix_token_counts_are_authenticated_scoped_and_restart_stable();
    test_exact_catalog_retains_zero_reserved_bytes_and_supports_manifest_discovery();
    test_authenticated_incomplete_identity_is_terminal();
#if defined(HALOFPX_CONTEXT_STORE_WORLD1_PREFIX_PRODUCT)
    test_product_feature_off_and_live_authority_unavailable_are_cold();
    test_product_same_system_prefix_different_suffix_and_exact_hit();
    test_product_corrupt_longer_candidate_is_terminal_cold();
    test_product_authority_mismatch_and_generation_change_are_cold();
#endif
    return 0;
}
