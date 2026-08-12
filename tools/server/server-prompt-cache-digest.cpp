#include "server-prompt-cache-digest.h"

extern "C" {
#include "sha256/sha256.h"
}

#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>

#if defined(HALOFPX_SERVER_PROMPT_CACHE_OPENSSL_SHA256)
#include <openssl/evp.h>
#endif

namespace fs = std::filesystem;

namespace {

constexpr size_t digest_buffer_size = 64 * 1024;

struct digest_attempt_result {
    server_prompt_cache_disk_digest_status status = server_prompt_cache_disk_digest_status::open_failed;
    bool provider_failed = false;
    size_t bytes_hashed = 0;
    server_prompt_cache_sha256 digest {};
};

digest_attempt_result digest_scalar(
        const std::string & path,
                    size_t expected_size,
        const server_prompt_cache_sha256 * expected_digest) {
    digest_attempt_result result;

    std::ifstream file(fs::u8path(path), std::ios::binary);
    if (!file) {
        result.status = server_prompt_cache_disk_digest_status::open_failed;
        return result;
    }

    sha256_t sha;
    sha256_init(&sha);

    std::array<unsigned char, digest_buffer_size> buffer;
    size_t remaining = expected_size;
    while (remaining > 0) {
        const size_t requested = remaining < buffer.size() ? remaining : buffer.size();
        file.read(reinterpret_cast<char *>(buffer.data()), static_cast<std::streamsize>(requested));
        const std::streamsize n = file.gcount();
        if (n != static_cast<std::streamsize>(requested)) {
            result.status = file.bad()
                ? server_prompt_cache_disk_digest_status::read_failed
                : server_prompt_cache_disk_digest_status::size_mismatch;
            std::memset(&sha, 0, sizeof(sha));
            return result;
        }

        sha256_update(&sha, buffer.data(), requested);
        result.bytes_hashed += requested;
        remaining -= requested;
    }

    char extra = 0;
    file.read(&extra, 1);
    if (file.bad()) {
        result.status = server_prompt_cache_disk_digest_status::read_failed;
        std::memset(&sha, 0, sizeof(sha));
        return result;
    }
    if (file.gcount() != 0) {
        result.status = server_prompt_cache_disk_digest_status::size_mismatch;
        std::memset(&sha, 0, sizeof(sha));
        return result;
    }

    sha256_final(&sha, result.digest.data());
    std::memset(&sha, 0, sizeof(sha));

    result.status = expected_digest != nullptr && result.digest != *expected_digest
        ? server_prompt_cache_disk_digest_status::digest_mismatch
        : server_prompt_cache_disk_digest_status::ok;
    return result;
}

#if defined(HALOFPX_SERVER_PROMPT_CACHE_OPENSSL_SHA256)

struct evp_md_deleter {
    void operator()(EVP_MD * value) const noexcept {
        EVP_MD_free(value);
    }
};

struct evp_md_ctx_deleter {
    void operator()(EVP_MD_CTX * value) const noexcept {
        EVP_MD_CTX_free(value);
    }
};

digest_attempt_result digest_openssl_evp(
        const std::string & path,
                    size_t expected_size,
        const server_prompt_cache_sha256 * expected_digest) {
    digest_attempt_result result;

    // Filesystem failures are authoritative and must not activate the scalar
    // provider fallback.
    std::ifstream file(fs::u8path(path), std::ios::binary);
    if (!file) {
        result.status = server_prompt_cache_disk_digest_status::open_failed;
        return result;
    }

    const std::unique_ptr<EVP_MD, evp_md_deleter> algorithm(
        EVP_MD_fetch(nullptr, "SHA256", nullptr));
    const std::unique_ptr<EVP_MD_CTX, evp_md_ctx_deleter> context(EVP_MD_CTX_new());
    if (!algorithm || !context || EVP_DigestInit_ex(context.get(), algorithm.get(), nullptr) != 1) {
        result.provider_failed = true;
        return result;
    }

    std::array<unsigned char, digest_buffer_size> buffer;
    size_t remaining = expected_size;
    while (remaining > 0) {
        const size_t requested = remaining < buffer.size() ? remaining : buffer.size();
        file.read(reinterpret_cast<char *>(buffer.data()), static_cast<std::streamsize>(requested));
        const std::streamsize n = file.gcount();
        if (n != static_cast<std::streamsize>(requested)) {
            result.status = file.bad()
                ? server_prompt_cache_disk_digest_status::read_failed
                : server_prompt_cache_disk_digest_status::size_mismatch;
            return result;
        }

        if (EVP_DigestUpdate(context.get(), buffer.data(), requested) != 1) {
            result.provider_failed = true;
            return result;
        }
        result.bytes_hashed += requested;
        remaining -= requested;
    }

    char extra = 0;
    file.read(&extra, 1);
    if (file.bad()) {
        result.status = server_prompt_cache_disk_digest_status::read_failed;
        return result;
    }
    if (file.gcount() != 0) {
        result.status = server_prompt_cache_disk_digest_status::size_mismatch;
        return result;
    }

    unsigned int digest_size = 0;
    if (EVP_DigestFinal_ex(context.get(), result.digest.data(), &digest_size) != 1 ||
            digest_size != result.digest.size()) {
        result.digest.fill(0);
        result.provider_failed = true;
        return result;
    }

    result.status = expected_digest != nullptr && result.digest != *expected_digest
        ? server_prompt_cache_disk_digest_status::digest_mismatch
        : server_prompt_cache_disk_digest_status::ok;
    return result;
}

#endif

server_prompt_cache_disk_digest_result make_result(
        const digest_attempt_result & attempt,
        server_prompt_cache_sha256_backend backend,
        bool fallback_used) {
    server_prompt_cache_disk_digest_result result;
    result.status = attempt.status;
    result.backend = backend;
    result.fallback_used = fallback_used;
    result.bytes_hashed = attempt.bytes_hashed;
    result.digest = attempt.digest;
    return result;
}

} // namespace

server_prompt_cache_disk_digest_result server_prompt_cache_disk_sha256_exact(
        const std::string & path,
                    size_t expected_size,
        const server_prompt_cache_sha256 * expected_digest) {
#if defined(HALOFPX_SERVER_PROMPT_CACHE_OPENSSL_SHA256)
    const auto provider = digest_openssl_evp(path, expected_size, expected_digest);
    if (!provider.provider_failed) {
        return make_result(provider, server_prompt_cache_sha256_backend::openssl_evp, false);
    }

    // Provider failure is the only fallback condition. Reopen the path and
    // restart from byte zero so no partially consumed EVP read can be accepted.
    return make_result(
        digest_scalar(path, expected_size, expected_digest),
        server_prompt_cache_sha256_backend::scalar,
        true);
#else
    return make_result(
        digest_scalar(path, expected_size, expected_digest),
        server_prompt_cache_sha256_backend::scalar,
        false);
#endif
}

const char * server_prompt_cache_sha256_backend_name(server_prompt_cache_sha256_backend backend) {
    switch (backend) {
        case server_prompt_cache_sha256_backend::scalar:      return "scalar";
        case server_prompt_cache_sha256_backend::openssl_evp: return "openssl-evp";
    }
    return "unknown";
}

server_prompt_cache_sha256_backend server_prompt_cache_sha256_configured_backend() {
#if defined(HALOFPX_SERVER_PROMPT_CACHE_OPENSSL_SHA256)
    return server_prompt_cache_sha256_backend::openssl_evp;
#else
    return server_prompt_cache_sha256_backend::scalar;
#endif
}
