#pragma once

#include "halofpx-context-store-auth.h"
#include "halofpx-context-store-object.h"
#include "halofpx-context-store.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace halofpx {

struct context_store_v1_frame_view {
    const uint8_t * data = nullptr;
    size_t size = 0;
};

struct context_store_v1_read_only_admission {
    context_store_authenticated_manifest_metadata manifest;
    const context_store_object_reference * objects = nullptr;
    size_t object_count = 0;
};

// Borrowed only for the factory call. The resulting provider owns an immutable
// copy of the manifest, verification policy (including key material), and all
// frames. This seam is intentionally memory-only and has no discovery path.
struct context_store_v1_read_only_source {
    const uint8_t * manifest_data = nullptr;
    size_t manifest_size = 0;
    context_store_manifest_verification_policy verification_policy;
    context_store_v1_read_only_admission admission;
    const context_store_v1_frame_view * frames = nullptr;
    size_t frame_count = 0;
    context_store_object_limits object_limits;
    uint64_t max_total_frame_bytes = 0;
};

struct context_store_v1_payload_view {
    const uint8_t * data = nullptr;
    size_t size = 0;
};

// A fully authenticated and frame-verified synthetic full-v1 candidate. It
// exposes immutable metadata and opaque payload bytes only. It is not a codec
// admission and cannot mutate live inference state.
class context_store_v1_read_only_candidate : public context_store_candidate {
public:
    ~context_store_v1_read_only_candidate() override;

    virtual const context_store_format_digest & manifest_digest() const noexcept = 0;
    virtual uint64_t generation() const noexcept = 0;
    virtual const context_store_registered_id & state_profile_id() const noexcept = 0;
    virtual uint64_t world_size() const noexcept = 0;
    virtual uint64_t topology_epoch() const noexcept = 0;
    virtual const context_store_format_digest & producer_identity() const noexcept = 0;
    virtual uint8_t durability_mode() const noexcept = 0;
    virtual size_t object_count() const noexcept = 0;
    virtual const context_store_object_reference * descriptor(size_t index) const noexcept = 0;
    virtual context_store_v1_payload_view payload(size_t index) const noexcept = 0;
};

// Creates the excluded, memory-only full-v1 composition seam. Construction may
// throw on invalid input or allocation failure. lookup() remains noexcept and
// maps every rejection to a miss; publish() is always disabled.
std::unique_ptr<context_store_provider> make_context_store_v1_read_only_provider(
    const context_store_v1_read_only_source & source);

} // namespace halofpx
