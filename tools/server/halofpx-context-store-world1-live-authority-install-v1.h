#pragma once

#include "halofpx-context-store-world1-prefix-product-v1.h"

#include <cstdint>
#include <memory>

namespace halofpx {

// This boundary installs an already-derived capability from a separately
// trusted live loader/context/lifecycle source. It never accepts component
// digests, command-line values, or an ADR-0052 world-two expectation directly.
enum class context_store_world1_live_authority_fact_v1 : uint8_t {
    model_artifacts,
    typed_model_metadata,
    tokenizer,
    chat_template_renderer,
    system_tool_context,
    adapters_projectors,
    runtime_build,
    state_abi,
    backend_device,
    quant_kv,
    context_rope_window,
    sampler_logits,
    grammar_parser_tool,
    rng,
    target_draft_mtp_speculative,
    global_plan,
    rank_ownership,
    rank_placement,
    stable_topology,
    security_scope,
    producer_identity,
    model_generation,
    count,
};

static_assert(static_cast<uint8_t>(
    context_store_world1_live_authority_fact_v1::count) < 64,
    "world1 live-authority fact mask must fit uint64_t");

constexpr uint64_t context_store_world1_live_authority_fact_bit_v1(
        context_store_world1_live_authority_fact_v1 fact) noexcept {
    return uint64_t {1} << static_cast<uint8_t>(fact);
}

constexpr uint64_t context_store_world1_live_authority_required_facts_v1 =
    (uint64_t {1} << static_cast<uint8_t>(
        context_store_world1_live_authority_fact_v1::count)) - 1;

enum class context_store_world1_live_authority_source_kind_v1 : uint8_t {
    unavailable,
    trusted_live_loader_context_lifecycle,
    operator_components,
    standalone_world2_authority,
};

// The fact mask is a custody declaration made by a trusted source, not a
// substitute for deriving and validating the underlying closed preimages.
// Product code must not construct this snapshot from operator assertions.
struct context_store_world1_live_authority_snapshot_v1 {
    context_store_world1_live_authority_source_kind_v1 source_kind =
        context_store_world1_live_authority_source_kind_v1::unavailable;
    uint64_t captured_facts = 0;
    context_store_world1_cache_authority_v1 authority {};
};

class context_store_world1_live_authority_source_v1 {
public:
    virtual ~context_store_world1_live_authority_source_v1() = default;

    // Called synchronously. A source must freeze every fact for the complete
    // model/context lifetime before returning the owned digest capability.
    virtual context_store_world1_live_authority_snapshot_v1 capture() const noexcept = 0;
};

enum class context_store_world1_live_authority_install_status_v1 : uint8_t {
    installed,
    feature_off,
    source_unavailable,
    untrusted_source,
    incomplete_fact_custody,
    invalid_authority,
    model_generation_unavailable,
    model_generation_changed,
    allocation_failed,
};

struct context_store_world1_live_authority_install_request_v1 {
    bool enabled = false;
    const context_store_world1_live_authority_source_v1 * source = nullptr;
    uint64_t expected_model_generation = 0;
};

struct context_store_world1_live_authority_install_result_v1 {
    context_store_world1_live_authority_install_status_v1 status =
        context_store_world1_live_authority_install_status_v1::feature_off;
    std::unique_ptr<const context_store_world1_cache_authority_v1> authority;

    bool installed() const noexcept {
        return status ==
                context_store_world1_live_authority_install_status_v1::installed &&
            static_cast<bool>(authority);
    }
};

context_store_world1_live_authority_install_result_v1
context_store_install_world1_live_authority_v1(
    const context_store_world1_live_authority_install_request_v1 & request) noexcept;

const char * context_store_world1_live_authority_install_status_name_v1(
    context_store_world1_live_authority_install_status_v1 status) noexcept;

} // namespace halofpx
