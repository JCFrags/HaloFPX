#pragma once

#include "halofpx-context-store-v1-server-canary.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>

namespace halofpx {

constexpr size_t context_store_v1_catalog_max_slots = 8;

enum class context_store_v1_catalog_status : uint8_t {
    ready,
    published,
    hit,
    miss_not_found,
    miss_corrupt,
    miss_incompatible,
    source_rejected,
    capacity_exhausted,
    busy,
    storage,
};

struct context_store_v1_catalog_slot_config {
    const char * data_root_path = nullptr;
    const char * anchor_root_path = nullptr;
};

struct context_store_v1_catalog_config {
    const char * catalog_root_path = nullptr;
    const context_store_v1_catalog_slot_config * slots = nullptr;
    size_t slot_count = 0;
    context_store_v1_server_canary_config child;
};

struct context_store_v1_catalog_publish_result {
    context_store_v1_catalog_status status = context_store_v1_catalog_status::source_rejected;
    context_store_format_digest selected_manifest {};
};

struct context_store_v1_catalog_restore_result {
    context_store_v1_catalog_status status = context_store_v1_catalog_status::miss_not_found;
    context_store_transformer_snapshot_v1 snapshot;
    // True only after an authenticated final, reservation, or pending record
    // matched the exact requested identity.  A non-hit with this bit set is
    // terminal uncertainty for prefix selection and must not fall through to
    // a shorter checkpoint.
    bool authenticated_record_selected = false;
};

struct context_store_v1_catalog_prefix_query {
    context_store_format_digest compatibility_root {};
    context_store_format_digest producer_identity {};
    context_store_format_digest scope_namespace {};
    uint64_t policy_epoch = 0;
    size_t max_token_count = 0;
    context_store_transformer_profile_v1 profile;
};

struct context_store_v1_catalog_prefix_result {
    context_store_v1_catalog_status status =
        context_store_v1_catalog_status::miss_not_found;
    std::array<size_t, context_store_v1_catalog_max_slots> token_counts {};
    size_t token_count = 0;
};

struct context_store_v1_catalog_open_result;

// A nonblocking, process-local exclusion token for a sequence of catalog reads
// that must observe no intervening publication. The catalog's lifetime must
// exceed the token's lifetime. Read-only operations retain their independent
// bounded operation locks while this token is held.
class context_store_v1_catalog_mutation_custody {
public:
    context_store_v1_catalog_mutation_custody(
        context_store_v1_catalog_mutation_custody &&) noexcept = default;
    context_store_v1_catalog_mutation_custody & operator=(
        context_store_v1_catalog_mutation_custody &&) noexcept = default;
    context_store_v1_catalog_mutation_custody(
        const context_store_v1_catalog_mutation_custody &) = delete;
    context_store_v1_catalog_mutation_custody & operator=(
        const context_store_v1_catalog_mutation_custody &) = delete;

    bool owns_custody() const noexcept { return lock_.owns_lock(); }

private:
    explicit context_store_v1_catalog_mutation_custody(std::mutex & mutex) noexcept
        : lock_(mutex, std::try_to_lock) {}

    std::unique_lock<std::mutex> lock_;
    friend class context_store_v1_catalog;
};

class context_store_v1_catalog {
public:
    ~context_store_v1_catalog();
    context_store_v1_catalog(const context_store_v1_catalog &) = delete;
    context_store_v1_catalog & operator=(const context_store_v1_catalog &) = delete;

    context_store_v1_catalog_publish_result publish(
        const context_store_transformer_snapshot_v1 & snapshot) noexcept;
    context_store_v1_catalog_restore_result restore_exact(
        const llama_token * expected_tokens,
        size_t expected_token_count,
        const context_store_identity & identity,
        const context_store_transformer_profile_v1 & profile) noexcept;
    // Returns strictly increasing checkpoint lengths derived from authenticated
    // child manifests for the requested compatibility/scope/policy/profile.
    // It never loads object payloads or exposes lineage, prompt, path, digest,
    // or state material.  restore_exact() remains the sole child-state
    // validation authority.
    context_store_v1_catalog_prefix_result discover_prefix_token_counts(
        const context_store_v1_catalog_prefix_query & query) noexcept;
    context_store_v1_catalog_mutation_custody
        acquire_mutation_custody() noexcept;

private:
    class implementation;
    explicit context_store_v1_catalog(std::unique_ptr<implementation>) noexcept;
    std::unique_ptr<implementation> implementation_;
    std::mutex mutation_mutex_;
    std::mutex operation_mutex_;
    friend struct context_store_v1_catalog_open_result;
    friend context_store_v1_catalog_open_result
        make_context_store_v1_catalog(const context_store_v1_catalog_config &) noexcept;
};

struct context_store_v1_catalog_open_result {
    context_store_v1_catalog_status status = context_store_v1_catalog_status::storage;
    std::unique_ptr<context_store_v1_catalog> catalog;
};

context_store_v1_catalog_open_result make_context_store_v1_catalog(
    const context_store_v1_catalog_config & config) noexcept;
const char * context_store_v1_catalog_status_name(context_store_v1_catalog_status) noexcept;

} // namespace halofpx
