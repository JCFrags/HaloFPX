#pragma once

#include "halofpx-context-store-v1-linux-generation-one.h"
#include "halofpx-context-store-v1-transformer-codec.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace halofpx {

constexpr size_t context_store_v1_server_canary_operator_key_bytes = 32;

enum class context_store_v1_server_canary_status : uint8_t {
    ready,
    published,
    hit,
    miss_not_found,
    miss_corrupt,
    miss_incompatible,
    miss_unsupported,
    source_rejected,
    busy,
    storage,
    quarantined,
};

struct context_store_v1_server_canary_config {
    const char * data_root_path = nullptr;
    const char * anchor_root_path = nullptr;
    context_store_key_view operator_key;
    std::array<uint8_t, 16> store_uuid {};
    context_store_compatibility_expectation compatibility;
    context_store_format_digest producer_identity {};
    context_store_format_digest global_plan_digest {};
    context_store_format_digest rank_ownership_digest {};
    context_store_format_digest rank_placement_digest {};
    uint64_t topology_epoch = 0;
    context_store_v1_transformer_codec_limits limits;
};

struct context_store_v1_server_canary_publish_result {
    context_store_v1_server_canary_status status =
        context_store_v1_server_canary_status::source_rejected;
    context_store_format_digest selected_manifest {};
};

struct context_store_v1_server_canary_restore_result {
    context_store_v1_server_canary_status status =
        context_store_v1_server_canary_status::miss_unsupported;
    context_store_transformer_snapshot_v1 snapshot;
};

struct context_store_v1_server_canary_open_result;

// Default-off explicit-handle server adapter. One server controller thread must
// own and call an instance. The caller-supplied selected digest is only a bounded
// direct selector: the fixed anchor, selected manifest, admission, and objects
// must all authenticate before this adapter returns a snapshot.
class context_store_v1_server_canary {
public:
    ~context_store_v1_server_canary();
    context_store_v1_server_canary(const context_store_v1_server_canary &) = delete;
    context_store_v1_server_canary & operator=(const context_store_v1_server_canary &) = delete;

    bool available() const noexcept;

    context_store_v1_server_canary_publish_result publish(
        const context_store_transformer_snapshot_v1 & snapshot) noexcept;

    context_store_v1_server_canary_restore_result restore(
        const context_store_format_digest & selected_manifest,
        const llama_token * expected_tokens,
        size_t expected_token_count,
        const context_store_identity & identity,
        const context_store_transformer_profile_v1 & profile) noexcept;

private:
    class implementation;
    explicit context_store_v1_server_canary(std::unique_ptr<implementation>) noexcept;
    std::unique_ptr<implementation> implementation_;

    friend struct context_store_v1_server_canary_open_result;
    friend context_store_v1_server_canary_open_result
    make_context_store_v1_server_canary(
        const context_store_v1_server_canary_config &) noexcept;
};

struct context_store_v1_server_canary_open_result {
    context_store_v1_server_canary_status status =
        context_store_v1_server_canary_status::storage;
    std::unique_ptr<context_store_v1_server_canary> canary;
};

context_store_v1_server_canary_open_result make_context_store_v1_server_canary(
    const context_store_v1_server_canary_config & config) noexcept;

const char * context_store_v1_server_canary_status_name(
    context_store_v1_server_canary_status status) noexcept;

} // namespace halofpx
