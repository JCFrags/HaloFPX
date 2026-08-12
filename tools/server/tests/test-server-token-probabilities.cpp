#include "server-token-probabilities.h"

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>
#include <cmath>
#include <cstdio>

static void assert_normalized(const std::vector<llama_token_data> & probabilities) {
    double sum = 0.0;
    for (const llama_token_data & token : probabilities) {
        assert(std::isfinite(token.p));
        assert(token.p >= 0.0f);
        sum += token.p;
    }
    assert(std::fabs(sum - 1.0) < 1e-6);
}

static void test_raw_row_ignores_empty_global_sampled_row() {
    const float raw_logits[] = {-1.0f, 3.0f, 1.0f, 0.0f};
    const llama_token global_candidate_ids[] = {0, 1, 2, 3};

    const auto probabilities = server_build_token_probabilities(
            nullptr, global_candidate_ids, 0, raw_logits, 4);

    assert(probabilities.size() == 4);
    assert(probabilities[0].id == 1);
    assert(probabilities[1].id == 2);
    assert(probabilities[2].id == 3);
    assert(probabilities[3].id == 0);
    assert_normalized(probabilities);
}

static void test_complete_sampled_row_wins_over_raw_row() {
    const float sampled_logits[] = {0.25f, 1.25f};
    const llama_token sampled_ids[] = {5, 2};
    const float raw_logits[] = {50.0f, 40.0f, 30.0f, 20.0f, 10.0f, 0.0f};

    const auto probabilities = server_build_token_probabilities(
            sampled_logits, sampled_ids, 2, raw_logits, 6);

    assert(probabilities.size() == 2);
    assert(probabilities[0].id == 2);
    assert(probabilities[1].id == 5);
    assert_normalized(probabilities);
}

static void test_incoherent_or_unavailable_rows_fail_closed() {
    const float sampled_logits[] = {1.0f};
    const llama_token sampled_ids[] = {0};
    const float raw_logits[] = {1.0f, 0.0f};

    assert(server_build_token_probabilities(
            sampled_logits, sampled_ids, 0, raw_logits, 2).empty());
    assert(server_build_token_probabilities(
            sampled_logits, nullptr, 1, raw_logits, 2).empty());
    assert(server_build_token_probabilities(
            nullptr, nullptr, 1, raw_logits, 2).empty());
    assert(server_build_token_probabilities(
            nullptr, nullptr, 0, nullptr, 2).empty());
    assert(server_build_token_probabilities(
            nullptr, nullptr, 0, raw_logits, 0).empty());
    assert(server_build_token_probabilities(
            sampled_logits, sampled_ids, 3, raw_logits, 2).empty());
}

int main() {
    test_raw_row_ignores_empty_global_sampled_row();
    test_complete_sampled_row_wins_over_raw_row();
    test_incoherent_or_unavailable_rows_fail_closed();

    std::puts("server token probability row tests passed");
    return 0;
}
