#pragma once

#include "llama.h"

#include <cstdint>
#include <vector>

// Pure row-to-probabilities helper used by get_token_probabilities() and its
// focused regression test. A sampled row is accepted only as a complete
// logits/ids/count tuple. The raw row is used only when no sampled row exists.
std::vector<llama_token_data> server_build_token_probabilities(
        const float * sampled_logits,
        const llama_token * sampled_ids,
        uint32_t sampled_logits_count,
        const float * raw_logits,
        int32_t n_vocab);
