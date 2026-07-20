#pragma once

#include "halofpx-context-store-v1-read-only.h"

#include <cstdint>
#include <memory>

namespace halofpx {

// Exact identity of an already-open snapshot root. The factory duplicates the
// descriptor and rejects any mismatch before retaining authority.
struct context_store_linux_root_identity_v1 {
    uint64_t device = 0;
    uint64_t inode = 0;
    uint64_t mount_id = 0;
    uint64_t owner_uid = 0;
    uint32_t mode = 0;
    uint64_t filesystem_type = 0;
};

// Borrowed only for the factory call. The adapter owns copies of the
// verification policy, fixture admission, and authentication key. root_fd must
// name the exact immutable publication root described by root_identity.
struct context_store_v1_linux_read_only_source {
    int root_fd = -1;
    context_store_linux_root_identity_v1 root_identity;
    context_store_manifest_verification_policy verification_policy;
    context_store_v1_read_only_admission admission;
    context_store_object_limits object_limits;
    uint64_t max_total_frame_bytes = 0;
};

// Linux-only, excluded filesystem adapter. It selects one manifest from the
// trusted replay anchor, authenticates it before deriving object names, and
// delegates complete frame admission to the memory-only L08d provider. It has
// no writer, enumeration, codec, restore, or server edge.
std::unique_ptr<context_store_provider> make_context_store_v1_linux_read_only_provider(
    const context_store_v1_linux_read_only_source & source);

} // namespace halofpx
