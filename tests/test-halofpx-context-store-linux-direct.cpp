#ifdef NDEBUG
#undef NDEBUG
#endif

#include "halofpx-context-store-linux-direct.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>

#if defined(__linux__)
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

using digest = halofpx::context_store_format_digest;
using open_status = halofpx::context_store_linux_direct_open_status;
using lookup_status = halofpx::context_store_linux_direct_lookup_status;
using publish_status = halofpx::context_store_linux_direct_publish_status;

struct temporary_root {
    std::string path;
    temporary_root() {
        std::array<char, 64> pattern {};
        std::strcpy(pattern.data(), "/tmp/halofpx-direct-XXXXXX");
        char * created = ::mkdtemp(pattern.data());
        assert(created != nullptr);
        path = created;
        assert(::chmod(path.c_str(), 0700) == 0);
    }
    ~temporary_root() { std::filesystem::remove_all(path); }
};

digest id(uint8_t seed) {
    digest value {};
    for (size_t index = 0; index < value.size(); ++index) value[index] = static_cast<uint8_t>(seed + index);
    return value;
}

std::string hex(const digest & value) {
    constexpr char digits[] = "0123456789abcdef";
    std::string output;
    output.reserve(64);
    for (const uint8_t byte : value) {
        output.push_back(digits[byte >> 4]);
        output.push_back(digits[byte & 0x0f]);
    }
    return output;
}

halofpx::context_store_linux_direct_config config_for(
        const temporary_root & root, const std::array<uint8_t, 32> & key,
        uint64_t quota = 1024 * 1024, uint64_t reserve = 0, size_t max_entries = 64) {
    halofpx::context_store_linux_direct_root_identity identity {};
    assert(halofpx::context_store_linux_direct_inspect_root(root.path.c_str(), identity) ==
        halofpx::context_store_linux_direct_identity_status::inspected);
    halofpx::context_store_linux_direct_config config;
    config.root_path = root.path.c_str();
    config.master_key = key.data();
    config.master_key_size = key.size();
    config.quota_bytes = quota;
    config.reserve_bytes = reserve;
    config.max_entries = max_entries;
    config.expected_root = identity;
    return config;
}

bool receipt_equal(
        const halofpx::context_store_linux_direct_receipt & left,
        const halofpx::context_store_linux_direct_receipt & right) {
    return left.manifest == right.manifest && left.selected_digest == right.selected_digest &&
        left.scope == right.scope && left.session == right.session &&
        left.compatibility == right.compatibility;
}

void flip_first_byte(const std::string & path) {
    const int fd = ::open(path.c_str(), O_RDWR | O_CLOEXEC | O_NOFOLLOW);
    assert(fd >= 0);
    uint8_t value = 0;
    assert(::pread(fd, &value, 1, 0) == 1);
    value ^= 0x80;
    assert(::pwrite(fd, &value, 1, 0) == 1);
    assert(::close(fd) == 0);
}

void direct_store_contract() {
    temporary_root root;
    std::array<uint8_t, 32> key {};
    for (size_t index = 0; index < key.size(); ++index) key[index] = static_cast<uint8_t>(0xa0 + index);
    const auto config = config_for(root, key);
    const digest scope = id(1), compatibility = id(65);
    const std::array<int32_t, 4> tokens = { 1, -2, INT32_MAX, INT32_MIN };
    const std::array<uint8_t, 5> state = { 9, 8, 7, 6, 5 };

    halofpx::context_store_linux_direct unavailable;
    halofpx::context_store_linux_direct_value value;
    assert(!unavailable.available());
    assert(unavailable.lookup(scope, id(2), compatibility, value) == lookup_status::unavailable);

    {
        halofpx::context_store_linux_direct store;
        assert(halofpx::context_store_linux_direct_open(config, store) == open_status::opened);
        halofpx::context_store_linux_direct second;
        assert(halofpx::context_store_linux_direct_open(config, second) == open_status::writer_busy);

        const digest session = id(10);
        assert(store.publish(scope, session, compatibility, tokens.data(), tokens.size(),
            state.data(), state.size()) == publish_status::published);
        assert(store.publish(scope, session, compatibility, tokens.data(), tokens.size(),
            state.data(), state.size()) == publish_status::already_exists);
        const std::array<uint8_t, 5> conflicting_state = { 9, 8, 7, 6, 4 };
        assert(store.publish(scope, session, compatibility, tokens.data(), tokens.size(),
            conflicting_state.data(), conflicting_state.size()) == publish_status::conflict);
        assert(store.lookup(scope, session, compatibility, value) == lookup_status::hit);
        assert(value.tokens == std::vector<int32_t>(tokens.begin(), tokens.end()));
        assert(value.state == std::vector<uint8_t>(state.begin(), state.end()));
        assert(store.lookup(id(3), session, compatibility, value) == lookup_status::miss_not_found);
        assert(store.lookup(scope, id(11), compatibility, value) == lookup_status::miss_not_found);
        assert(store.lookup(scope, session, id(66), value) == lookup_status::miss_incompatible);

        const std::string scope_path = root.path + "/" + hex(scope) + "/";
        const digest corrupt = id(20);
        assert(store.publish(scope, corrupt, compatibility, tokens.data(), tokens.size(), state.data(), state.size()) == publish_status::published);
        flip_first_byte(scope_path + hex(corrupt) + "/tokens");
        assert(store.lookup(scope, corrupt, compatibility, value) == lookup_status::miss_corrupt);

        const digest truncated = id(30);
        assert(store.publish(scope, truncated, compatibility, tokens.data(), tokens.size(), state.data(), state.size()) == publish_status::published);
        assert(::truncate((scope_path + hex(truncated) + "/state").c_str(), 2) == 0);
        assert(store.lookup(scope, truncated, compatibility, value) == lookup_status::miss_corrupt);

        const digest extra = id(40);
        assert(store.publish(scope, extra, compatibility, tokens.data(), tokens.size(), state.data(), state.size()) == publish_status::published);
        const int extra_fd = ::open((scope_path + hex(extra) + "/unexpected").c_str(), O_CREAT | O_EXCL | O_WRONLY, 0600);
        assert(extra_fd >= 0 && ::close(extra_fd) == 0);
        assert(store.lookup(scope, extra, compatibility, value) == lookup_status::miss_corrupt);

        const digest linked = id(50);
        assert(store.publish(scope, linked, compatibility, tokens.data(), tokens.size(), state.data(), state.size()) == publish_status::published);
        const std::string manifest = scope_path + hex(linked) + "/manifest";
        assert(::unlink(manifest.c_str()) == 0);
        assert(::symlink("/dev/null", manifest.c_str()) == 0);
        assert(store.lookup(scope, linked, compatibility, value) == lookup_status::miss_corrupt);

        const digest invisible = id(60);
        const std::string staging = root.path + "/.staging/" + hex(invisible);
        assert(::mkdir(staging.c_str(), 0700) == 0);
        assert(store.lookup(scope, invisible, compatibility, value) == lookup_status::miss_not_found);
    }

    {
        halofpx::context_store_linux_direct reopened;
        assert(halofpx::context_store_linux_direct_open(config, reopened) == open_status::opened);
        assert(reopened.lookup(scope, id(10), compatibility, value) == lookup_status::hit);
        assert(reopened.entry_count() == 5);
    }

    temporary_root quota_root;
    const auto quota_config = config_for(quota_root, key, 1, 0, 1);
    halofpx::context_store_linux_direct quota_store;
    assert(halofpx::context_store_linux_direct_open(quota_config, quota_store) == open_status::opened);
    assert(quota_store.publish(scope, id(70), compatibility, tokens.data(), tokens.size(),
        state.data(), state.size()) == publish_status::quota_exceeded);

    temporary_root full_root;
    const auto full_config = config_for(full_root, key, 1024 * 1024, 0, 1);
    halofpx::context_store_linux_direct full_store;
    assert(halofpx::context_store_linux_direct_open(full_config, full_store) == open_status::opened);
    assert(full_store.publish(scope, id(71), compatibility, tokens.data(), tokens.size(),
        state.data(), state.size()) == publish_status::published);
    assert(full_store.publish(scope, id(71), compatibility, tokens.data(), tokens.size(),
        state.data(), state.size()) == publish_status::already_exists);
    const std::array<uint8_t, 5> full_conflict = { 9, 8, 7, 6, 4 };
    assert(full_store.publish(scope, id(71), compatibility, tokens.data(), tokens.size(),
        full_conflict.data(), full_conflict.size()) == publish_status::conflict);
    assert(full_store.publish(scope, id(72), compatibility, tokens.data(), tokens.size(),
        state.data(), state.size()) == publish_status::quota_exceeded);

    temporary_root reserve_root;
    const auto reserve_config = config_for(reserve_root, key, UINT64_MAX, UINT64_MAX, 1);
    halofpx::context_store_linux_direct reserve_store;
    assert(halofpx::context_store_linux_direct_open(reserve_config, reserve_store) == open_status::opened);
    assert(reserve_store.publish(scope, id(80), compatibility, tokens.data(), tokens.size(),
        state.data(), state.size()) == publish_status::reserve_exhausted);
}

void direct_receipt_contract() {
    temporary_root root;
    std::array<uint8_t, 32> configured_key {};
    for (size_t index = 0; index < configured_key.size(); ++index) {
        configured_key[index] = static_cast<uint8_t>(0x40 + index);
    }
    const auto config = config_for(root, configured_key);
    const digest manifest_key = id(180);
    const digest wrong_manifest_key = id(181);
    const digest scope = id(90), session = id(100), compatibility = id(130);
    const std::array<int32_t, 4> tokens = { 4, 3, 2, 1 };
    const std::array<uint8_t, 6> state = { 1, 3, 3, 7, 9, 11 };

    halofpx::context_store_linux_direct store;
    assert(halofpx::context_store_linux_direct_open(config, store) == open_status::opened);
    halofpx::context_store_linux_direct_receipt published;
    assert(store.publish_with_receipt(manifest_key, scope, session, compatibility,
        tokens.data(), tokens.size(), state.data(), state.size(), published) == publish_status::published);
    assert(published.scope == scope);
    assert(published.session == session);
    assert(published.compatibility == compatibility);
    assert(std::equal(published.manifest.begin(), published.manifest.begin() + 8, "HFPXLD01"));

    std::array<uint8_t, 27 + halofpx::context_store_linux_direct_manifest_bytes> selected_preimage {};
    constexpr char selected_domain[] = "halofpx.direct-manifest.v1";
    static_assert(sizeof(selected_domain) == 27, "selected digest test domain drift");
    std::copy_n(reinterpret_cast<const uint8_t *>(selected_domain), sizeof(selected_domain), selected_preimage.begin());
    std::copy(published.manifest.begin(), published.manifest.end(), selected_preimage.begin() + sizeof(selected_domain));
    digest independently_selected {};
    assert(halofpx::context_store_sha256(selected_preimage.data(), selected_preimage.size(), independently_selected));
    assert(independently_selected == published.selected_digest);

    halofpx::context_store_linux_direct_receipt inspected;
    assert(store.inspect_manifest(manifest_key, scope, session, compatibility, inspected) == lookup_status::hit);
    assert(inspected.manifest == published.manifest);
    assert(inspected.selected_digest == published.selected_digest);
    assert(inspected.scope == published.scope && inspected.session == published.session &&
        inspected.compatibility == published.compatibility);

    halofpx::context_store_linux_direct_receipt repeated;
    assert(store.publish_with_receipt(manifest_key, scope, session, compatibility,
        tokens.data(), tokens.size(), state.data(), state.size(), repeated) == publish_status::already_exists);
    assert(repeated.manifest == published.manifest);
    assert(repeated.selected_digest == published.selected_digest);

    halofpx::context_store_linux_direct_value value;
    assert(store.authorized_load(manifest_key, published, value) == lookup_status::hit);
    assert(value.tokens == std::vector<int32_t>(tokens.begin(), tokens.end()));
    assert(value.state == std::vector<uint8_t>(state.begin(), state.end()));

    auto wrong_digest = published;
    wrong_digest.selected_digest[0] ^= 0x80;
    value.tokens = { 99 };
    value.state = { 99 };
    assert(store.authorized_load(manifest_key, wrong_digest, value) == lookup_status::miss_corrupt);
    assert(value.tokens.empty() && value.state.empty());

    auto wrong_receipt = published;
    wrong_receipt.manifest[20] ^= 0x01;
    std::copy(wrong_receipt.manifest.begin(), wrong_receipt.manifest.end(),
        selected_preimage.begin() + sizeof(selected_domain));
    assert(halofpx::context_store_sha256(selected_preimage.data(), selected_preimage.size(),
        wrong_receipt.selected_digest));
    value.tokens = { 98 };
    value.state = { 98 };
    assert(store.authorized_load(manifest_key, wrong_receipt, value) == lookup_status::miss_corrupt);
    assert(value.tokens.empty() && value.state.empty());

    halofpx::context_store_linux_direct_receipt rejected;
    assert(store.inspect_manifest(wrong_manifest_key, scope, session, compatibility, rejected) ==
        lookup_status::miss_corrupt);
    assert(receipt_equal(rejected, halofpx::context_store_linux_direct_receipt {}));

    const digest payload_corrupt_session = id(101);
    halofpx::context_store_linux_direct_receipt payload_receipt;
    assert(store.publish_with_receipt(manifest_key, scope, payload_corrupt_session, compatibility,
        tokens.data(), tokens.size(), state.data(), state.size(), payload_receipt) == publish_status::published);
    flip_first_byte(root.path + "/" + hex(scope) + "/" + hex(payload_corrupt_session) + "/state");
    halofpx::context_store_linux_direct_receipt payload_inspected;
    assert(store.inspect_manifest(manifest_key, scope, payload_corrupt_session, compatibility,
        payload_inspected) == lookup_status::hit);
    assert(receipt_equal(payload_inspected, payload_receipt));
    value.tokens = { 97 };
    value.state = { 97 };
    assert(store.authorized_load(manifest_key, payload_receipt, value) == lookup_status::miss_corrupt);
    assert(value.tokens.empty() && value.state.empty());

    flip_first_byte(root.path + "/" + hex(scope) + "/" + hex(session) + "/manifest");
    value.tokens = { 96 };
    value.state = { 96 };
    assert(store.authorized_load(manifest_key, published, value) == lookup_status::miss_corrupt);
    assert(value.tokens.empty() && value.state.empty());
}

} // namespace
#endif

int main() {
#if defined(__linux__)
    direct_store_contract();
    direct_receipt_contract();
#endif
    return 0;
}
