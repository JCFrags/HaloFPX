#ifdef NDEBUG
#undef NDEBUG
#endif

#include "server-prompt-cache-digest.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

#if defined(HALOFPX_TEST_EXPECT_OPENSSL_SHA256)
constexpr auto expected_backend = server_prompt_cache_sha256_backend::openssl_evp;
#else
constexpr auto expected_backend = server_prompt_cache_sha256_backend::scalar;
#endif

struct temp_directory {
    fs::path path;

    ~temp_directory() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
};

uint8_t hex_nibble(char value) {
    if (value >= '0' && value <= '9') {
        return static_cast<uint8_t>(value - '0');
    }
    if (value >= 'a' && value <= 'f') {
        return static_cast<uint8_t>(10 + value - 'a');
    }
    assert(false);
    return 0;
}

server_prompt_cache_sha256 digest_from_hex(const char * value) {
    const std::string hex(value);
    assert(hex.size() == 2 * SERVER_PROMPT_CACHE_SHA256_SIZE);

    server_prompt_cache_sha256 digest {};
    for (size_t i = 0; i < digest.size(); ++i) {
        digest[i] = static_cast<uint8_t>(
            (hex_nibble(hex[2 * i]) << 4) | hex_nibble(hex[2 * i + 1]));
    }
    return digest;
}

void write_file(const fs::path & path, const std::vector<uint8_t> & bytes) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    assert(file);
    if (!bytes.empty()) {
        file.write(
            reinterpret_cast<const char *>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
    }
    assert(file);
    file.close();
    assert(file);
}

void assert_backend(const server_prompt_cache_disk_digest_result & result) {
    assert(result.backend == expected_backend);
    assert(!result.fallback_used);
}

void check_vector(
        const fs::path & directory,
        const std::string & name,
        const std::vector<uint8_t> & bytes,
        const char * expected_hex) {
    const fs::path path = directory / name;
    write_file(path, bytes);

    const auto expected = digest_from_hex(expected_hex);
    const auto computed = server_prompt_cache_disk_sha256_exact(
        path.u8string(), bytes.size(), nullptr);
    assert(computed.status == server_prompt_cache_disk_digest_status::ok);
    assert_backend(computed);
    assert(computed.bytes_hashed == bytes.size());
    assert(computed.digest == expected);

    const auto verified = server_prompt_cache_disk_sha256_exact(
        path.u8string(), bytes.size(), &expected);
    assert(verified.status == server_prompt_cache_disk_digest_status::ok);
    assert_backend(verified);
    assert(verified.bytes_hashed == bytes.size());
    assert(verified.digest == expected);
}

} // namespace

int main() {
    assert(server_prompt_cache_sha256_configured_backend() == expected_backend);

    const auto nonce = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    temp_directory temporary {
        fs::temp_directory_path() /
        ("halofpx-prompt-cache-digest-" + std::to_string(nonce))
    };
    assert(fs::create_directory(temporary.path));

    check_vector(
        temporary.path, "empty.bin", {},
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    check_vector(
        temporary.path, "abc.bin", {'a', 'b', 'c'},
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

    struct repeated_vector {
        size_t size;
        const char * digest;
    };
    const repeated_vector vectors[] = {
        {     1, "6922e93e3827642ce4b883c756b31abf80036649d3614bf5fcb3adda43b8ea32" },
        {    55, "26ee0116778740a66fe2ba10ea063748b27306acc99188ec812746d4e8d70083" },
        {    56, "4cf71e2b0aa0fcc0c271f68353026a77b8e50153632a8e4a73833cd64080e92e" },
        {    63, "a1942663a5b8b93dffc9c4ff5f62c71a1c021d1fcc1e470dd46172abace1bca5" },
        {    64, "bb626e5577021df95ea17eb6339e75904855b80087e40660931c4a89b302f74a" },
        {    65, "667f84020d981fcedce2816e4e9969a02d5c317a0aef56a6c588175820f82a81" },
        { 65535, "1acaf51d16a9c1a2b05a091386f193dad790603c7803b5e55dd4681a5bb9dfd2" },
        { 65536, "77007cd74a06dc54e5114d01a41d2721679d5668a0c20022fe102c87ad4d65b8" },
        { 65537, "a3253bfa7d790dc6f5d41b75421bcc2c606e8043f9fad4331e8aae0a6c504cfa" },
    };
    for (const auto & vector : vectors) {
        check_vector(
            temporary.path,
            "a5-" + std::to_string(vector.size) + ".bin",
            std::vector<uint8_t>(vector.size, 0xa5),
            vector.digest);
    }

    const fs::path abc_path = temporary.path / "abc.bin";
    const auto too_short = server_prompt_cache_disk_sha256_exact(
        abc_path.u8string(), 2, nullptr);
    assert(too_short.status == server_prompt_cache_disk_digest_status::size_mismatch);
    assert_backend(too_short);

    const auto too_long = server_prompt_cache_disk_sha256_exact(
        abc_path.u8string(), 4, nullptr);
    assert(too_long.status == server_prompt_cache_disk_digest_status::size_mismatch);
    assert_backend(too_long);

    auto wrong_digest = digest_from_hex(
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    wrong_digest[0] ^= 0x80;
    const auto mismatch = server_prompt_cache_disk_sha256_exact(
        abc_path.u8string(), 3, &wrong_digest);
    assert(mismatch.status == server_prompt_cache_disk_digest_status::digest_mismatch);
    assert_backend(mismatch);

    const auto missing = server_prompt_cache_disk_sha256_exact(
        (temporary.path / "missing.bin").u8string(), 0, nullptr);
    assert(missing.status == server_prompt_cache_disk_digest_status::open_failed);
    assert_backend(missing);

    std::cout << "prompt-cache SHA-256 vectors passed with backend="
              << server_prompt_cache_sha256_backend_name(expected_backend) << '\n';
    return 0;
}
