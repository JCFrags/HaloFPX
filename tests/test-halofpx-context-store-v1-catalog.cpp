#include "halofpx-context-store-v1-catalog.h"

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

} // namespace

int main() {
    test_two_restart_hits_misses_and_capacity();
    test_catalog_final_tamper_is_safe_miss();
    test_catalog_root_transplant_and_pending_duplicates_are_safe_misses();
    test_corrupt_child_is_safe_miss();
    test_unexpected_catalog_layout_is_safe_miss();
    return 0;
}
