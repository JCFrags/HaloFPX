---
section_id: "29"
title: "Model facts and support stages"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "ggml-org/llama.cpp"]
  software_versions: ["ROCmFPX a5605a7", "llama.cpp 788e07d"]
  hardware_revisions: []
related_sections: ["30", "31", "33", "34", "35", "36"]
---

# Candidate catalog

Parameter counts and active counts below are publisher labels unless a config directly exposes them. Approximate weight size is `parameters * nominal bits / 8`; mixtures, metadata, embeddings, alignment, and split overhead make actual GGUF size different. KV bytes/token is derived only when ordinary GQA dimensions are unambiguous: `layers * kv_heads * (K_head_dim * K_bytes + V_head_dim * V_bytes)`.

| Candidate and pinned source | Kind / intended role | Config facts | Nominal weights | Derived F16 KV B/token | License | Current evidence state |
|---|---|---|---:|---:|---|---|
| Qwen2.5-Coder-32B-Instruct, `381fc969` [S29-02] | dense GQA; coding/tool-use | 64 layers, hidden 5120, 40 Q / 8 KV heads, head 128, native config context 32,768 | F16 ~64 GB; 8.25 bpw ~33 GB; 4.5 bpw ~18 GB | 262,144 | Apache-2.0 | **[VERIFIED]** config; **[OPEN]** fork conversion/backend/quality |
| Qwen3-30B-A3B, `ad44e777` [S29-03] | MoE GQA; reasoning/tool candidate | 48 layers, hidden 2048, 32 Q / 4 KV heads, 128 experts, top-8, context 40,960; publisher name says 30B/A3B | F16 ~60 GB; 8.25 bpw ~30.9 GB; 4.5 bpw ~16.9 GB | 98,304 | Apache-2.0 | **[VERIFIED]** config and upstream Qwen3-MoE enum; **[OPEN]** target GGUF |
| DeepSeek-V3, `e815299b` [S29-04] | MoE + MLA + one next-token-prediction layer; scale/transport stressor | 61 layers, hidden 7168, 256 routed + 1 shared experts, top-8, MLA ranks 512/1536, context 163,840 | publisher: 671B/37B active; nominal FP8 ~671 GB, 4-bit ~336 GB | not ordinary GQA; latent-cache layout required | model-specific license; verify before use | **[VERIFIED]** config and DeepSeek enums; **[INFERENCE]** whole-model local fit is unlikely; **[OPEN]** exact shard plan |
| Mistral-Small-3.1-24B-Instruct-2503, `68faf511` [S29-05] | dense GQA, multimodal, tool/long-context | text: 40 layers, hidden 5120, 32 Q / 8 KV heads, head 128, context 131,072; separate Pixtral config | F16 ~48 GB; 8.25 bpw ~24.8 GB; 4.5 bpw ~13.5 GB | 163,840 | Apache-2.0 | **[VERIFIED]** config and Mistral3 enum; **[OPEN]** mmproj plus backends |
| NVIDIA Nemotron-3-Nano-30B-A3B-BF16, `cbd3fa9f` [S29-06] | hybrid Mamba/attention MoE; SSM-state and long-context test | 52 layers, hidden 2688, pattern contains M/E/* layers, 128 routed experts top-6, 32 Q / 2 KV heads, SSM state 128, context 262,144 | publisher: 30B/A3B; F16 ~60 GB | ordinary formula is invalid across recurrent layers | NVIDIA Open Model License (publisher metadata says `other`) | **[VERIFIED]** config and Nemotron-H enum; **[OPEN]** recurrent-state fidelity |

## Coverage still requiring a pinned target

| Required class | Current catalog coverage | Gap |
|---|---|---|
| MHA | architecture family is represented upstream, but no production candidate pinned | **[OPEN] OQ29-01** choose a modern MHA baseline if needed |
| GQA | Qwen, Mistral | machine validation |
| MLA / MTP | DeepSeek-V3 | model exceeds likely aggregate memory; evaluate smaller MLA/MTP candidate |
| sliding-window/global | no pinned candidate in this batch | **[OPEN] OQ29-02** pin Gemma-family or other primary checkpoint |
| Mamba/SSM/hybrid | Nemotron-3-Nano | verify GGUF conversion and recurrent-state serialization |
| coding/tool-use | Qwen2.5-Coder and instruction candidates | define local task suite and chat-template contract |
| long context | Mistral 131K, DeepSeek 163K, Nemotron 262K configs | advertised/configured context is not measured usable context |

## Support-stage rubric

1. `CONFIG_PINNED`: immutable publisher config and license recorded.
2. `ARCH_RECOGNIZED`: target commit has architecture metadata and graph class.
3. `CONVERTED`: converter completes and metadata/tensor/tokenizer audit passes.
4. `CPU_SMOKE`: deterministic short prompt and state round-trip pass.
5. `BACKEND_SMOKE`: HIP and Vulkan each load and generate without fallback surprises.
6. `QUALITY_GATED`: source/BF16/quant comparisons pass Section 31 gates.
7. `HALOFPX_VALIDATED`: both machines, intended context/cache/distributed mode, hashes and raw evidence recorded.

No candidate currently reaches stage 7 in this workspace.

