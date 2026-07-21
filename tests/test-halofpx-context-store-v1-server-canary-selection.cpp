#include "halofpx-context-store-v1-server-canary.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {

void create_lock(const std::filesystem::path & path) {
    const int fd = ::open(path.c_str(), O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    assert(fd >= 0 && ::close(fd) == 0);
}

void create_data_layout(const std::filesystem::path & data) {
    assert(::mkdir((data / "staging").c_str(), 0700) == 0);
    assert(::mkdir((data / "objects").c_str(), 0700) == 0);
    assert(::mkdir((data / "manifests").c_str(), 0700) == 0);
}

void cbor_head(std::vector<uint8_t> & out, uint8_t major, uint64_t value) {
    assert(value <= 0xff);
    if (value < 24) out.push_back(static_cast<uint8_t>((major << 5) | value));
    else {
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

halofpx::context_store_v1_server_canary_config config_for(
        const std::string & data_path, const std::string & anchor_path,
        const std::array<uint8_t, 32> & operator_key) {
    halofpx::context_store_v1_server_canary_config config;
    config.data_root_path = data_path.c_str();
    config.anchor_root_path = anchor_path.c_str();
    config.operator_key = { operator_key.data(), operator_key.size() };
    config.store_uuid.fill(0x11);
    for (size_t i = 0; i < config.compatibility.components.size(); ++i)
        config.compatibility.components[i].fill(static_cast<uint8_t>(0x20 + i));
    bind_compatibility(config);
    config.producer_identity.fill(0x31);
    config.global_plan_digest.fill(0x32);
    config.rank_ownership_digest.fill(0x33);
    config.rank_placement_digest.fill(0x34);
    config.topology_epoch = 1;
    config.quota_bytes = 1024 * 1024;
    config.max_entries = 1;
    config.limits = { { 64, 16 }, 4096, 65536 };
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

void test_missing_anchor_is_miss() {
    std::array<char, 64> pattern {};
    const char text[] = "/tmp/halofpx-selection-miss-XXXXXX";
    std::copy(text, text + sizeof(text), pattern.begin());
    const char * base_text = ::mkdtemp(pattern.data());
    assert(base_text != nullptr);
    const std::filesystem::path base(base_text);
    const auto data = base / "data";
    const auto anchor = base / "anchor";
    assert(::chmod(base.c_str(), 0700) == 0);
    assert(::mkdir(data.c_str(), 0700) == 0 && ::mkdir(anchor.c_str(), 0700) == 0);
    create_lock(data / "writer.lock");
    create_lock(anchor / "writer.lock");
    create_data_layout(data);

    std::array<uint8_t, 32> operator_key {};
    operator_key.fill(0x41);
    const std::string data_path = data.string();
    const std::string anchor_path = anchor.string();
    auto config = config_for(data_path, anchor_path, operator_key);
    auto opened = halofpx::make_context_store_v1_server_canary(config);
    assert(opened.canary != nullptr);
    halofpx::context_store_identity identity;
    identity.compatibility_root = config.compatibility.root;
    identity.scope_namespace.fill(0x51);
    identity.checkpoint_lineage_id.fill(0x52);
    identity.policy_epoch = 1;
    const auto profile = admitted_profile();
    const llama_token token = 1;
    const auto restored = opened.canary->restore_selected(&token, 1, identity, profile);
    assert(restored.status == halofpx::context_store_v1_server_canary_status::miss_not_found);
    std::error_code ignored;
    std::filesystem::remove_all(base, ignored);
}

void test_selected_restore_and_rejection() {
    std::array<char, 64> pattern {};
    const char text[] = "/tmp/halofpx-selection-hit-XXXXXX";
    std::copy(text, text + sizeof(text), pattern.begin());
    const char * base_text = ::mkdtemp(pattern.data());
    assert(base_text != nullptr);
    const std::filesystem::path base(base_text);
    const auto data = base / "data";
    const auto anchor = base / "anchor";
    assert(::chmod(base.c_str(), 0700) == 0);
    assert(::mkdir(data.c_str(), 0700) == 0 && ::mkdir(anchor.c_str(), 0700) == 0);
    create_lock(data / "writer.lock");
    create_lock(anchor / "writer.lock");
    create_data_layout(data);

    std::array<uint8_t, 32> operator_key {};
    operator_key.fill(0x41);
    const std::string data_path = data.string();
    const std::string anchor_path = anchor.string();
    auto config = config_for(data_path, anchor_path, operator_key);

    halofpx::context_store_transformer_snapshot_v1 snapshot;
    snapshot.compatibility_identity.compatibility_root = config.compatibility.root;
    snapshot.compatibility_identity.scope_namespace.fill(0x51);
    snapshot.compatibility_identity.checkpoint_lineage_id.fill(0x52);
    snapshot.compatibility_identity.policy_epoch = 1;
    snapshot.profile = admitted_profile();
    snapshot.tokens = { 7, 11, 13 };
    snapshot.state = { 0xa1, 0xb2, 0xc3, 0xd4 };

    auto opened = halofpx::make_context_store_v1_server_canary(config);
    assert(opened.canary != nullptr);
    const auto published = opened.canary->publish(snapshot);
    assert(published.status == halofpx::context_store_v1_server_canary_status::published);
    opened.canary.reset();

    opened = halofpx::make_context_store_v1_server_canary(config);
    assert(opened.canary != nullptr);
    const auto restored = opened.canary->restore_selected(
        snapshot.tokens.data(), snapshot.tokens.size(), snapshot.compatibility_identity,
        snapshot.profile);
    assert(restored.status == halofpx::context_store_v1_server_canary_status::hit);
    assert(restored.snapshot.tokens == snapshot.tokens);
    assert(restored.snapshot.state == snapshot.state);

    auto wrong_identity = snapshot.compatibility_identity;
    wrong_identity.scope_namespace[0] ^= 1;
    const auto wrong = opened.canary->restore_selected(
        snapshot.tokens.data(), snapshot.tokens.size(), wrong_identity, snapshot.profile);
    assert(wrong.status == halofpx::context_store_v1_server_canary_status::miss_corrupt);

    const int anchor_fd = ::open((anchor / "anchor.v1").c_str(), O_RDWR | O_CLOEXEC);
    assert(anchor_fd >= 0);
    uint8_t byte = 0;
    assert(::pread(anchor_fd, &byte, 1, 0) == 1);
    byte ^= 1;
    assert(::pwrite(anchor_fd, &byte, 1, 0) == 1 && ::fsync(anchor_fd) == 0);
    assert(::close(anchor_fd) == 0);
    const auto corrupt = opened.canary->restore_selected(
        snapshot.tokens.data(), snapshot.tokens.size(), snapshot.compatibility_identity,
        snapshot.profile);
    assert(corrupt.status == halofpx::context_store_v1_server_canary_status::miss_corrupt);

    opened.canary.reset();
    std::error_code ignored;
    std::filesystem::remove_all(base, ignored);
}

} // namespace

int main() {
    test_missing_anchor_is_miss();
    test_selected_restore_and_rejection();
    return 0;
}
