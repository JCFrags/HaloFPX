#pragma once

#include "llama-ext.h"

#include <cstddef>
#include <cstdint>

// Pure validation input for one already-resolved host output row. Buffer-bound
// checks remain owned by llama_context; this helper enforces provenance and
// tuple coherence without touching a backend or scheduler.
struct llama_output_row_input {
    size_t n_vocab = 0;
    int32_t requested_index = 0;
    int32_t resolved_row = -1;
    uint64_t generation = 0;

    bool raw_available = false;
    const float * raw_logits = nullptr;
    const llama_token * canonical_ids = nullptr;
    size_t canonical_ids_count = 0;

    const float * sampled_logits = nullptr;
    const float * sampled_probs = nullptr;
    const llama_token * sampled_candidates = nullptr;
    uint32_t sampled_logits_count = 0;
    uint32_t sampled_probs_count = 0;
    uint32_t sampled_candidates_count = 0;
    llama_token sampled_token = LLAMA_TOKEN_NULL;
};

inline bool llama_output_row_build(
        const llama_output_row_input & input,
        bool prefer_sampled,
        bool token_only,
        llama_output_row_view & view) {
    view = {};
    view.requested_index = input.requested_index;
    view.resolved_row = input.resolved_row;
    view.generation = input.generation;
    view.sampled_token = LLAMA_TOKEN_NULL;
    view.sampled_candidate_index = -1;

    if (input.n_vocab == 0 || input.n_vocab > UINT32_MAX || input.resolved_row < 0) {
        return false;
    }

    const auto set_raw = [&]() {
        if (!input.raw_available || !input.raw_logits || !input.canonical_ids ||
                input.canonical_ids_count != input.n_vocab) {
            return false;
        }

        view.source = LLAMA_OUTPUT_ROW_SOURCE_RAW;
        view.logits = input.raw_logits;
        view.candidates = input.canonical_ids;
        view.logits_count = static_cast<uint32_t>(input.n_vocab);
        view.candidates_count = static_cast<uint32_t>(input.n_vocab);
        return true;
    };

    // A caller asking for raw logits must never silently receive sampled data.
    if (!prefer_sampled) {
        return set_raw();
    }

    const bool has_sampled_row = input.sampled_logits_count != 0 ||
        input.sampled_probs_count != 0 || input.sampled_candidates_count != 0 ||
        input.sampled_token != LLAMA_TOKEN_NULL;
    if (!has_sampled_row) {
        return set_raw();
    }

    // Some backend samplers publish only their selected token. This is a
    // coherent sampled result, but deliberately exposes no logits tuple.
    if (input.sampled_token != LLAMA_TOKEN_NULL &&
            input.sampled_logits_count == 0 && input.sampled_probs_count == 0 &&
            input.sampled_candidates_count == 0) {
        if (input.sampled_token < 0 || (size_t) input.sampled_token >= input.n_vocab) {
            return false;
        }
        view.source = LLAMA_OUTPUT_ROW_SOURCE_SAMPLED_TOKEN;
        view.sampled_token = input.sampled_token;
        return true;
    }

    // Any partial sampled metadata makes the sampled row unavailable. Never
    // combine it with raw data that happens to share the context allocation.
    if (input.sampled_logits_count == 0 || input.sampled_logits_count > input.n_vocab ||
            !input.sampled_logits ||
            (input.sampled_probs_count != 0 &&
                (input.sampled_probs_count != input.sampled_logits_count || !input.sampled_probs))) {
        return false;
    }

    const llama_token * candidates = nullptr;
    uint32_t candidates_count = 0;
    bool canonical_candidates = false;
    if (input.sampled_candidates_count != 0) {
        if (input.sampled_candidates_count != input.sampled_logits_count ||
                !input.sampled_candidates) {
            return false;
        }
        candidates = input.sampled_candidates;
        candidates_count = input.sampled_candidates_count;
    } else {
        if (input.sampled_logits_count != input.n_vocab || !input.canonical_ids ||
                input.canonical_ids_count != input.n_vocab) {
            return false;
        }
        candidates = input.canonical_ids;
        candidates_count = static_cast<uint32_t>(input.n_vocab);
        canonical_candidates = true;
    }

    bool sampled_token_found = input.sampled_token == LLAMA_TOKEN_NULL;
    int32_t sampled_candidate_index = -1;
    if (canonical_candidates) {
        sampled_token_found = sampled_token_found ||
            (input.sampled_token >= 0 && (size_t) input.sampled_token < input.n_vocab);
        if (input.sampled_token != LLAMA_TOKEN_NULL && sampled_token_found) {
            sampled_candidate_index = input.sampled_token;
        }
    } else {
        for (uint32_t i = 0; i < candidates_count; ++i) {
            const llama_token candidate = candidates[i];
            if (candidate < 0 || (size_t) candidate >= input.n_vocab) {
                return false;
            }
            if (candidate == input.sampled_token) {
                sampled_token_found = true;
                sampled_candidate_index = static_cast<int32_t>(i);
            }
        }
    }
    if (!sampled_token_found) {
        return false;
    }

    if (token_only && input.sampled_token != LLAMA_TOKEN_NULL) {
        view.source = LLAMA_OUTPUT_ROW_SOURCE_SAMPLED_TOKEN;
        view.sampled_token = input.sampled_token;
        view.sampled_candidate_index = sampled_candidate_index;
        return true;
    }

    view.source = input.sampled_probs_count != 0
        ? LLAMA_OUTPUT_ROW_SOURCE_SAMPLED_PROBS
        : LLAMA_OUTPUT_ROW_SOURCE_SAMPLED_LOGITS;
    view.logits = input.sampled_logits;
    view.probs = input.sampled_probs_count != 0 ? input.sampled_probs : nullptr;
    view.candidates = candidates;
    view.logits_count = input.sampled_logits_count;
    view.probs_count = input.sampled_probs_count;
    view.candidates_count = candidates_count;
    view.sampled_token = input.sampled_token;
    view.sampled_candidate_index = sampled_candidate_index;
    return true;
}

inline bool llama_output_row_build(
        const llama_output_row_input & input,
        bool prefer_sampled,
        llama_output_row_view & view) {
    return llama_output_row_build(input, prefer_sampled, false, view);
}
