#include "llama-context.h"

#include <array>
#include <cstdint>
#include <cstring>

int main() {
    static constexpr const char * admitted[] = {
        "attn_inp_kq_mask", "attn_inp_k_rot", "attn_inp_v_rot",
        "attn_scale", "inp_embd", "inp_k_idxs", "inp_out_ids", "inp_pos",
        "inp_tokens", "inp_v_idxs",
    };
    for (size_t i = 0; i < sizeof(admitted) / sizeof(admitted[0]); ++i) {
        std::array<uint8_t, 16> baseline {};
        std::array<uint8_t, 16> perturbed {};
        baseline[0] = static_cast<uint8_t>(i);
        perturbed = baseline;
        perturbed[15] ^= 0x5a;
        std::array<uint8_t, 32> baseline_digest {};
        std::array<uint8_t, 32> perturbed_digest {};
        if (!llama_halofpx_graph_input_content_digest(
                admitted[i], baseline.data(), baseline.size(), baseline_digest.data())) return 1;
        if (!llama_halofpx_graph_input_content_digest(
                admitted[i], perturbed.data(), perturbed.size(), perturbed_digest.data())) return 2;
        if (std::memcmp(
                baseline_digest.data(), perturbed_digest.data(), baseline_digest.size()) == 0) return 3;
    }

    std::array<uint8_t, 1> sentinel { 1 };
    std::array<uint8_t, 32> digest {};
    if (llama_halofpx_graph_input_content_digest(
            "unknown_mutable_input", sentinel.data(), sentinel.size(), digest.data())) return 4;
    if (llama_halofpx_graph_input_content_digest(
            "inp_tokens", nullptr, sentinel.size(), digest.data())) return 5;
    if (llama_halofpx_graph_input_content_digest(
            "inp_tokens", sentinel.data(), 0, digest.data())) return 6;
    return 0;
}
