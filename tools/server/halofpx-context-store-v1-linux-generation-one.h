#pragma once

#include "halofpx-context-store-protected-canary-anchor.h"
#include "halofpx-context-store-v1-attempt-wire.h"
#include "halofpx-context-store-v1-linux-publish.h"

#include <cstdint>
#include <memory>

namespace halofpx {

enum class context_store_v1_linux_generation_one_failpoint : uint8_t {
    none,
    after_pending,
    after_material,
    after_anchor,
};

enum class context_store_v1_linux_generation_one_status : uint8_t {
    ready,
    published,
    recovered_success,
    recovered_aborted,
    interrupted,
    busy,
    invalid,
    unsupported,
    source_mismatch,
    conflict,
    storage,
    synchronization,
    quarantined,
};

// All pointers and descriptors are borrowed only during construction. The
// authority duplicates both roots and owns copies of all keys and admission
// metadata. The exact anchor is generation one with a null predecessor.
struct context_store_v1_linux_generation_one_config {
    context_store_v1_linux_publish_root data_root;
    context_store_v1_linux_publish_root anchor_root;
    context_store_manifest_verification_policy verification_policy;
    context_store_v1_read_only_admission admission;
    context_store_object_limits object_limits;
    uint64_t max_total_frame_bytes = 0;
    context_store_protected_canary_anchor_body anchor_body;
    context_store_protected_canary_anchor_key anchor_key;
    context_store_v1_attempt_key attempt_key;
    context_store_v1_linux_generation_one_failpoint test_failpoint =
        context_store_v1_linux_generation_one_failpoint::none;
};

struct context_store_v1_linux_generation_one_open_result;

// Linux-only, excluded generation-one authority. It never mutates live llama
// state. A sticky quarantine makes every later operation fail closed. One
// controller thread owns an instance: publish, lookup, status, and quarantined
// must not be called concurrently until a server-edge serialization contract is
// introduced and independently reviewed.
class context_store_v1_linux_generation_one {
public:
    ~context_store_v1_linux_generation_one();
    context_store_v1_linux_generation_one(
        const context_store_v1_linux_generation_one &) = delete;
    context_store_v1_linux_generation_one & operator=(
        const context_store_v1_linux_generation_one &) = delete;

    context_store_v1_linux_generation_one_status status() const noexcept;
    bool quarantined() const noexcept;

    context_store_v1_linux_generation_one_status publish(
        const context_store_v1_read_only_source & source) noexcept;

    context_store_lookup_result lookup(
        const context_store_lookup_request & request) const noexcept;

private:
    class implementation;
    explicit context_store_v1_linux_generation_one(
        std::unique_ptr<implementation> implementation) noexcept;
    std::unique_ptr<implementation> implementation_;

    friend struct context_store_v1_linux_generation_one_open_result;
    friend context_store_v1_linux_generation_one_open_result
    make_context_store_v1_linux_generation_one(
        const context_store_v1_linux_generation_one_config & config) noexcept;
};

struct context_store_v1_linux_generation_one_open_result {
    context_store_v1_linux_generation_one_status status =
        context_store_v1_linux_generation_one_status::invalid;
    std::unique_ptr<context_store_v1_linux_generation_one> authority;
};

// Construction validates and duplicates distinct roots, acquires the
// anchor-root writer.lock OFD lock, and reconciles a fixed pending.v1 before
// returning. All construction failures are represented by the result status.
context_store_v1_linux_generation_one_open_result
make_context_store_v1_linux_generation_one(
    const context_store_v1_linux_generation_one_config & config) noexcept;

const char * context_store_v1_linux_generation_one_status_name(
    context_store_v1_linux_generation_one_status status) noexcept;

} // namespace halofpx
