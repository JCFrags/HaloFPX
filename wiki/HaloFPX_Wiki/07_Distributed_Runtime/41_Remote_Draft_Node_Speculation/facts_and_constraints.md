---
section_id: "41"
title: "Remote Speculation Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "vllm-project/vllm@9354f222042986addf20709e5274fc26e0d09745"]
  software_versions: []
  hardware_revisions: []
related_sections: ["31", "38", "48", "55"]
---

# Facts and constraints

- **[VERIFIED]** Speculative decoding can preserve the target distribution while using a faster approximation to propose multiple tokens, provided the published accept/reject/correction algorithm is followed [S41-01, S41-02].
- **[VERIFIED]** At commit `788e07d`, `llama.cpp` documents draft-model, EAGLE-3, DFlash, MTP, and n-gram variants; draft depth defaults/options and draft-specific device/KV controls exist [S41-03]. This is local source behavior, not a remote protocol.
- **[VERIFIED]** vLLM distinguishes an external draft model from MTP, where a model natively supplies multi-token prediction and no ordinary separate drafter is required [S41-04, S41-05]. Some MTP paths share target KV/hidden-state structure and are therefore not automatically suitable for an independent remote node.

## Exact stochastic semantics

Let target distribution be `p_i`, draft distribution `q_i`, and proposed token `x_i` at speculative position `i`.

1. Accept `x_i` with probability `min(1, p_i(x_i)/q_i(x_i))`, in order.
2. On first rejection, sample the replacement from normalized `max(0, p_i - q_i)`.
3. If all proposals are accepted, sample the additional target token from the target distribution as defined by the algorithm.

**[VERIFIED]** This correction is why exact stochastic execution needs access to `q_i` over the vocabulary at rejection, not merely `q_i(x_i)` [S41-01]. A lossless compressed representation or target-side deterministic draft recomputation can substitute, but must be proven equivalent.

For greedy decode, target verifies the proposed IDs against target argmax in order; no draft probabilities are needed. Greedy equality is not stochastic-distribution equivalence.

## Compatibility

**[RECOMMENDATION]** Handshake identity includes target/draft model hashes, vocab/tokenizer and special-token mapping, chat template, context limit, positional/rope settings, sampler-transform order, draft algorithm/version, KV ABI, and session epoch. Heterogeneous vocab mapping is a separate algorithm; current vLLM documents a token-intersection option with restrictions [S41-04].

## Cost/traffic terms

Per round cost is `T_draft(k) + T_msg_proposal + T_target_verify(k+1) + T_msg_result + T_rollback + jitter`. Benefit exists only if accepted tokens per round amortize target verification and network tails below ordinary sequential target decode.

Traffic minimums:

- greedy: round/session IDs plus `k` token IDs; response accepted count/replacement token;
- exact stochastic: above plus sufficient `q_i` information for exact correction and shared RNG/transform semantics;
- EAGLE-like: may require target hidden states, greatly increasing coupling;
- native MTP: target-local auxiliary heads/state; remote placement may erase its intended sharing advantage.
