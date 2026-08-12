#include "server-token-probabilities.h"

#include <algorithm>
#include <cmath>

std::vector<llama_token_data> server_build_token_probabilities(
        const float * sampled_logits,
        const llama_token * sampled_ids,
        uint32_t sampled_logits_count,
        const float * raw_logits,
        int32_t n_vocab) {
    std::vector<llama_token_data> cur;

    const float * logits = nullptr;
    const llama_token * token_ids = nullptr;
    size_t n_logits = 0;

    if (sampled_logits != nullptr) {
        if (sampled_ids == nullptr || sampled_logits_count == 0 ||
                n_vocab <= 0 || sampled_logits_count > (uint32_t) n_vocab) {
            return cur;
        }

        logits = sampled_logits;
        token_ids = sampled_ids;
        n_logits = sampled_logits_count;
    } else {
        // A non-zero sampled count without sampled logits is an incoherent row.
        // Do not combine it with an otherwise valid raw pointer.
        if (sampled_logits_count != 0 || raw_logits == nullptr || n_vocab <= 0) {
            return cur;
        }

        logits = raw_logits;
        n_logits = (size_t) n_vocab;
    }

    cur.resize(n_logits);
    if (token_ids) {
        for (size_t i = 0; i < n_logits; ++i) {
            cur[i] = llama_token_data{token_ids[i], logits[i], 0.0f};
        }
    } else {
        for (llama_token token_id = 0; token_id < (llama_token) n_logits; ++token_id) {
            cur[token_id] = llama_token_data{token_id, logits[token_id], 0.0f};
        }
    }

    // sort tokens by logits
    std::sort(cur.begin(), cur.end(), [](const llama_token_data & a, const llama_token_data & b) {
        return a.logit > b.logit;
    });

    // apply softmax
    float max_l = cur[0].logit;
    float cum_sum = 0.0f;
    for (size_t i = 0; i < cur.size(); ++i) {
        float p = expf(cur[i].logit - max_l);
        cur[i].p = p;
        cum_sum += p;
    }
    for (size_t i = 0; i < cur.size(); ++i) {
        cur[i].p /= cum_sum;
    }

    return cur;
}
