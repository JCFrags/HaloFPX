#include "halofpx-context-store-linux-protected.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {

using namespace halofpx;

void require(bool condition, const char * expression, int line) {
    if (!condition) {
        std::fprintf(stderr, "requirement failed at line %d: %s\n", line, expression);
        std::exit(1);
    }
}

#define REQUIRE(condition) require((condition), #condition, __LINE__)

context_store_format_digest digest(uint8_t seed) {
    context_store_format_digest output {};
    for (size_t index = 0; index < output.size(); ++index) {
        output[index] = static_cast<uint8_t>(seed + index);
    }
    return output;
}

std::string hex(const context_store_format_digest & value) {
    constexpr char digits[] = "0123456789abcdef";
    std::string output(value.size() * 2, '0');
    for (size_t index = 0; index < value.size(); ++index) {
        output[index * 2] = digits[value[index] >> 4];
        output[index * 2 + 1] = digits[value[index] & 0x0f];
    }
    return output;
}

struct fixture {
    std::string base;
    std::string direct_root;
    std::string anchor_root;
    std::array<uint8_t, 32> operator_key {};
    std::array<uint8_t, 16> store_uuid {};
    context_store_linux_protected_root_authority authority;
    context_store_linux_direct_root_identity direct_identity;
    context_store_linux_direct_root_identity anchor_identity;

    fixture() {
        std::array<char, 64> name {};
        std::snprintf(name.data(), name.size(), "/tmp/halofpx-protected-test-XXXXXX");
        char * created = ::mkdtemp(name.data());
        REQUIRE(created != nullptr);
        base = created;
        direct_root = base + "/direct";
        anchor_root = base + "/anchor";
        REQUIRE(::mkdir(direct_root.c_str(), 0700) == 0);
        REQUIRE(::mkdir(anchor_root.c_str(), 0700) == 0);
        for (size_t index = 0; index < operator_key.size(); ++index) {
            operator_key[index] = static_cast<uint8_t>(0x40 + index);
        }
        for (size_t index = 0; index < store_uuid.size(); ++index) {
            store_uuid[index] = static_cast<uint8_t>(0x90 + index);
        }
        REQUIRE(context_store_linux_protected_derive_root_authority(
            operator_key.data(), operator_key.size(), store_uuid, authority));
        REQUIRE(context_store_linux_direct_inspect_root(direct_root.c_str(), direct_identity) ==
            context_store_linux_direct_identity_status::inspected);
        REQUIRE(context_store_linux_direct_inspect_root(anchor_root.c_str(), anchor_identity) ==
            context_store_linux_direct_identity_status::inspected);
    }

    ~fixture() {
        std::filesystem::remove_all(base);
    }

    context_store_linux_protected_config config(
            context_store_linux_protected_test_failpoint failpoint =
                context_store_linux_protected_test_failpoint::none) {
        context_store_linux_protected_config output;
        output.direct.root_path = direct_root.c_str();
        output.direct.quota_bytes = 16 * 1024 * 1024;
        output.direct.reserve_bytes = 0;
        output.direct.max_entries = 8;
        output.direct.expected_root = direct_identity;
        output.anchor_root_path = anchor_root.c_str();
        output.expected_anchor_root = anchor_identity;
        output.store_uuid = store_uuid;
        output.direct_root_key = authority.direct_root_key.data();
        output.direct_root_key_size = authority.direct_root_key.size();
        output.anchor_root_key = authority.anchor_root_key.data();
        output.anchor_root_key_size = authority.anchor_root_key.size();
        output.test_failpoint = failpoint;
        return output;
    }
};

void flip_first_byte(const std::string & path) {
    const int fd = ::open(path.c_str(), O_RDWR | O_CLOEXEC | O_NOFOLLOW);
    REQUIRE(fd >= 0);
    uint8_t value = 0;
    REQUIRE(::pread(fd, &value, 1, 0) == 1);
    value ^= 0x01;
    REQUIRE(::pwrite(fd, &value, 1, 0) == 1);
    REQUIRE(::fdatasync(fd) == 0);
    REQUIRE(::close(fd) == 0);
}

} // namespace

int main() {
#if !defined(__linux__)
    return 0;
#else
    fixture env;
    const auto scope = digest(1);
    const auto session = digest(40);
    const auto compatibility = digest(80);
    const std::array<int32_t, 4> tokens { 4, 8, 15, 16 };
    const std::array<uint8_t, 7> state { 23, 42, 1, 2, 3, 5, 8 };

    {
        context_store_linux_protected store;
        auto config = env.config(
            context_store_linux_protected_test_failpoint::ambiguous_before_anchor_rename);
        REQUIRE(context_store_linux_protected_open(config, store) ==
            context_store_linux_protected_open_status::opened);
        context_store_linux_direct_value output;
        REQUIRE(store.lookup(scope, session, compatibility, output) ==
            context_store_linux_protected_lookup_status::miss_not_found);
        REQUIRE(store.publish(scope, session, compatibility, tokens.data(), tokens.size(),
            state.data(), state.size()) == context_store_linux_protected_publish_status::unreachable);
        REQUIRE(store.lookup(scope, session, compatibility, output) ==
            context_store_linux_protected_lookup_status::miss_unanchored);
    }

    {
        context_store_linux_protected store;
        auto config = env.config();
        REQUIRE(context_store_linux_protected_open(config, store) ==
            context_store_linux_protected_open_status::opened);
        REQUIRE(store.publish(scope, session, compatibility, tokens.data(), tokens.size(),
            state.data(), state.size()) == context_store_linux_protected_publish_status::published);
    }

    const std::string anchor_path = env.anchor_root + "/" + hex(scope) + "/" + hex(session) + ".anchor";
    {
        context_store_linux_protected store;
        auto config = env.config();
        REQUIRE(context_store_linux_protected_open(config, store) ==
            context_store_linux_protected_open_status::opened);
        context_store_linux_direct_value output;
        REQUIRE(store.lookup(scope, session, compatibility, output) ==
            context_store_linux_protected_lookup_status::hit);
        REQUIRE(output.tokens == std::vector<int32_t>(tokens.begin(), tokens.end()));
        REQUIRE(output.state == std::vector<uint8_t>(state.begin(), state.end()));
        flip_first_byte(anchor_path);
        REQUIRE(store.lookup(scope, session, compatibility, output) ==
            context_store_linux_protected_lookup_status::miss_corrupt);
        REQUIRE(output.tokens.empty() && output.state.empty());
        flip_first_byte(anchor_path);
        REQUIRE(store.lookup(scope, session, compatibility, output) ==
            context_store_linux_protected_lookup_status::hit);
        auto different = state;
        different[0] ^= 0x20;
        REQUIRE(store.publish(scope, session, compatibility, tokens.data(), tokens.size(),
            different.data(), different.size()) == context_store_linux_protected_publish_status::conflict);
    }

    const auto recovered_session = digest(120);
    {
        context_store_linux_protected store;
        auto config = env.config(
            context_store_linux_protected_test_failpoint::ambiguous_after_anchor_rename);
        REQUIRE(context_store_linux_protected_open(config, store) ==
            context_store_linux_protected_open_status::opened);
        REQUIRE(store.publish(scope, recovered_session, compatibility, tokens.data(), tokens.size(),
            state.data(), state.size()) ==
            context_store_linux_protected_publish_status::recovered_durable);
        context_store_linux_direct_value output;
        REQUIRE(store.lookup(scope, recovered_session, compatibility, output) ==
            context_store_linux_protected_lookup_status::hit);
    }

    return 0;
#endif
}
