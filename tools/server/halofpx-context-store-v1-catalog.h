#pragma once

#include "halofpx-context-store-v1-server-canary.h"

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
};

struct context_store_v1_catalog_open_result;

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

private:
    class implementation;
    explicit context_store_v1_catalog(std::unique_ptr<implementation>) noexcept;
    std::unique_ptr<implementation> implementation_;
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
