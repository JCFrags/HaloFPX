#pragma once

#include "halofpx-context-store-v1-linux-read-only.h"

#include <array>
#include <cstdint>
#include <memory>

namespace halofpx {

using context_store_v1_publish_attempt_id = std::array<uint8_t, 32>;

enum class context_store_v1_linux_publish_status : uint8_t {
    materialized_non_authoritative,
    already_equal_non_authoritative,
    invalid,
    busy,
    unsupported,
    conflict,
    storage,
    synchronization,
    verification,
    incomplete_or_uncertain_discard_root,
};

// The descriptor is borrowed only while the factory duplicates root_fd. The
// exact identity is rechecked before each synchronous publication attempt.
struct context_store_v1_linux_publish_root {
    int root_fd = -1;
    context_store_linux_root_identity_v1 identity;
};

// Excluded Linux-only synthetic write-mechanics lab. This is deliberately not
// a publication authority and its result cannot advance an anchor/generation.
// It holds a root-scoped OFD lock while materializing one caller-supplied test
// fixture in a disposable root. It has no server edge, background work, or
// live-state codec. incomplete_or_uncertain_discard_root requires discarding
// the entire root; retry or adoption is not admitted.
class context_store_v1_linux_snapshot_materializer {
public:
    ~context_store_v1_linux_snapshot_materializer();

    context_store_v1_linux_snapshot_materializer(
        const context_store_v1_linux_snapshot_materializer &) = delete;
    context_store_v1_linux_snapshot_materializer & operator=(
        const context_store_v1_linux_snapshot_materializer &) = delete;
    context_store_v1_linux_snapshot_materializer(
        context_store_v1_linux_snapshot_materializer &&) = delete;
    context_store_v1_linux_snapshot_materializer & operator=(
        context_store_v1_linux_snapshot_materializer &&) = delete;

    context_store_v1_linux_publish_status publish(
        const context_store_v1_publish_attempt_id & attempt_id,
        const context_store_v1_read_only_source & source) noexcept;

private:
    class implementation;
    explicit context_store_v1_linux_snapshot_materializer(
        std::unique_ptr<implementation> implementation) noexcept;

    std::unique_ptr<implementation> implementation_;

    friend std::unique_ptr<context_store_v1_linux_snapshot_materializer>
    make_context_store_v1_linux_snapshot_materializer(
        const context_store_v1_linux_publish_root & root);
};

// Construction may throw std::invalid_argument for an invalid/mismatched root
// or an allocation exception. No filesystem mutation occurs in the factory.
std::unique_ptr<context_store_v1_linux_snapshot_materializer>
make_context_store_v1_linux_snapshot_materializer(
    const context_store_v1_linux_publish_root & root);

const char * context_store_v1_linux_publish_status_name(
    context_store_v1_linux_publish_status status) noexcept;

} // namespace halofpx
