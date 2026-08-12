#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

constexpr size_t SERVER_PROMPT_CACHE_SHA256_SIZE = 32;

using server_prompt_cache_sha256 = std::array<uint8_t, SERVER_PROMPT_CACHE_SHA256_SIZE>;

enum class server_prompt_cache_disk_digest_status {
    ok,
    open_failed,
    read_failed,
    size_mismatch,
    digest_mismatch,
};

enum class server_prompt_cache_sha256_backend {
    scalar,
    openssl_evp,
};

struct server_prompt_cache_disk_digest_result {
    server_prompt_cache_disk_digest_status status = server_prompt_cache_disk_digest_status::open_failed;
    server_prompt_cache_sha256_backend backend = server_prompt_cache_sha256_backend::scalar;
    bool fallback_used = false;
    size_t bytes_hashed = 0;
    server_prompt_cache_sha256 digest {};
};

// Hash exactly expected_size bytes and reject both truncation and append.
// Open/read/size/digest failures are authoritative. If the optional EVP
// provider fails internally, the implementation reopens the file and retries
// the complete operation with the scalar provider.
server_prompt_cache_disk_digest_result server_prompt_cache_disk_sha256_exact(
        const std::string & path,
                    size_t expected_size,
        const server_prompt_cache_sha256 * expected_digest);

const char * server_prompt_cache_sha256_backend_name(server_prompt_cache_sha256_backend backend);

server_prompt_cache_sha256_backend server_prompt_cache_sha256_configured_backend();
