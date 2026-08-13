#include "llama-output-row.h"

#include <cstdio>
#include <vector>

namespace {

bool check(bool condition, const char * expression, int line) {
    if (!condition) {
        std::fprintf(stderr, "check failed at line %d: %s\n", line, expression);
    }
    return condition;
}

#define CHECK(expr) do { if (!check((expr), #expr, __LINE__)) return 1; } while (false)

} // namespace

int main() {
    constexpr size_t n_vocab = 8;
    std::vector<float> raw(n_vocab, 1.0f);
    std::vector<float> sampled_logits(n_vocab, 2.0f);
    std::vector<float> sampled_probs(n_vocab, 0.125f);
    std::vector<llama_token> canonical(n_vocab);
    for (size_t i = 0; i < canonical.size(); ++i) {
        canonical[i] = static_cast<llama_token>(i);
    }

    llama_output_row_input input;
    input.n_vocab = n_vocab;
    input.requested_index = -1;
    input.resolved_row = 3;
    input.generation = 11;
    input.canonical_ids = canonical.data();
    input.canonical_ids_count = canonical.size();

    llama_output_row_view view;
    CHECK(!llama_output_row_build(input, true, view));
    CHECK(view.source == LLAMA_OUTPUT_ROW_SOURCE_UNAVAILABLE);
    CHECK(view.requested_index == -1);
    CHECK(view.resolved_row == 3);
    CHECK(view.generation == 11);

    input.raw_available = true;
    input.raw_logits = raw.data();
    CHECK(llama_output_row_build(input, true, view));
    CHECK(view.source == LLAMA_OUTPUT_ROW_SOURCE_RAW);
    CHECK(view.logits == raw.data());
    CHECK(view.candidates == canonical.data());
    CHECK(view.logits_count == n_vocab);
    CHECK(view.candidates_count == n_vocab);

    // Raw-only preserves raw provenance even when a complete sampled tuple is
    // simultaneously present.
    input.sampled_logits = sampled_logits.data();
    input.sampled_logits_count = static_cast<uint32_t>(n_vocab);
    CHECK(llama_output_row_build(input, false, view));
    CHECK(view.source == LLAMA_OUTPUT_ROW_SOURCE_RAW);
    CHECK(view.logits == raw.data());

    // Raw-only requests never switch to a sampled source.
    input.raw_available = false;
    input.raw_logits = nullptr;
    input.sampled_logits = sampled_logits.data();
    input.sampled_logits_count = static_cast<uint32_t>(n_vocab);
    CHECK(!llama_output_row_build(input, false, view));
    CHECK(view.source == LLAMA_OUTPUT_ROW_SOURCE_UNAVAILABLE);

    // A full-vocabulary sampled row uses the canonical identity IDs without a
    // redundant candidate buffer.
    CHECK(llama_output_row_build(input, true, view));
    CHECK(view.source == LLAMA_OUTPUT_ROW_SOURCE_SAMPLED_LOGITS);
    CHECK(view.logits == sampled_logits.data());
    CHECK(view.candidates == canonical.data());
    CHECK(view.logits_count == n_vocab);

    input.sampled_probs = sampled_probs.data();
    input.sampled_probs_count = static_cast<uint32_t>(n_vocab);
    CHECK(llama_output_row_build(input, true, view));
    CHECK(view.source == LLAMA_OUTPUT_ROW_SOURCE_SAMPLED_PROBS);
    CHECK(view.probs == sampled_probs.data());

    // Count disagreement is an explicit miss; raw data must not be substituted.
    input.raw_available = true;
    input.raw_logits = raw.data();
    input.sampled_probs_count = static_cast<uint32_t>(n_vocab - 1);
    CHECK(!llama_output_row_build(input, true, view));
    CHECK(view.source == LLAMA_OUTPUT_ROW_SOURCE_UNAVAILABLE);

    input.sampled_probs = nullptr;
    input.sampled_probs_count = 0;
    std::vector<llama_token> reduced = { 1, 4, 6 };
    input.sampled_logits_count = static_cast<uint32_t>(reduced.size());
    input.sampled_candidates = reduced.data();
    input.sampled_candidates_count = static_cast<uint32_t>(reduced.size());
    input.sampled_token = 4;
    CHECK(llama_output_row_build(input, true, view));
    CHECK(view.source == LLAMA_OUTPUT_ROW_SOURCE_SAMPLED_LOGITS);
    CHECK(view.candidates == reduced.data());
    CHECK(view.candidates_count == reduced.size());
    CHECK(view.sampled_token == 4);
    CHECK(view.sampled_candidate_index == 1);

    CHECK(llama_output_row_build(input, true, true, view));
    CHECK(view.source == LLAMA_OUTPUT_ROW_SOURCE_SAMPLED_TOKEN);
    CHECK(view.sampled_token == 4);
    CHECK(view.sampled_candidate_index == 1);

    input.sampled_candidates_count = 2;
    CHECK(!llama_output_row_build(input, true, view));
    CHECK(!llama_output_row_build(input, true, true, view));
    input.sampled_candidates_count = static_cast<uint32_t>(reduced.size());

    reduced[1] = static_cast<llama_token>(n_vocab);
    CHECK(!llama_output_row_build(input, true, view));
    CHECK(!llama_output_row_build(input, true, true, view));
    reduced[1] = 4;

    reduced[1] = -1;
    CHECK(!llama_output_row_build(input, true, view));
    CHECK(view.source == LLAMA_OUTPUT_ROW_SOURCE_UNAVAILABLE);
    CHECK(view.logits == nullptr);
    CHECK(view.probs == nullptr);
    CHECK(view.candidates == nullptr);
    CHECK(view.logits_count == 0);
    CHECK(view.probs_count == 0);
    CHECK(view.candidates_count == 0);
    reduced[1] = 4;

    input.sampled_token = 2;
    CHECK(!llama_output_row_build(input, true, view));

    // Token-only output is coherent and retains explicit sampled provenance.
    input.sampled_logits = nullptr;
    input.sampled_logits_count = 0;
    input.sampled_candidates = nullptr;
    input.sampled_candidates_count = 0;
    input.sampled_token = 5;
    CHECK(llama_output_row_build(input, true, view));
    CHECK(view.source == LLAMA_OUTPUT_ROW_SOURCE_SAMPLED_TOKEN);
    CHECK(view.sampled_token == 5);
    CHECK(view.sampled_candidate_index == -1);
    CHECK(view.logits == nullptr);
    CHECK(view.candidates == nullptr);

    CHECK(llama_output_row_build(input, true, true, view));
    CHECK(view.source == LLAMA_OUTPUT_ROW_SOURCE_SAMPLED_TOKEN);

    input.sampled_token = static_cast<llama_token>(n_vocab);
    CHECK(!llama_output_row_build(input, true, view));
    CHECK(view.source == LLAMA_OUTPUT_ROW_SOURCE_UNAVAILABLE);

    return 0;
}
