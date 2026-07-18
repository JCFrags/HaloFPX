# DeepSeek-V3.1

**Decision:** `ADVANCE_TO_LOCAL_PREFLIGHT`  
**Publisher identity:** `deepseek-ai/DeepSeek-V3.1@c0781d039fb7a1ba2abc4add0bdc293e92d2b8db`  
**Selected artifact:** `unsloth/DeepSeek-V3.1-GGUF@f69aacf1ee1e4f66737daebc7e71fa527dfda1bd/UD-IQ2_XXS`  
**Local validation:** `NOT_RUN`

## [PUBLISHER] Architecture and state

- Parameters: 671B total / 37B activated
- Architecture: DeepSeek-V3-family sparse MoE with Multi-head Latent Attention (MLA)
- State requirements: KV/latent attention state; routed and shared expert weights; YaRN/RoPE scaling metadata
- Context: `{"publisher_card": "128K", "config_max_position_embeddings": 163840, "discrepancy": "Preserved; runtime maximum requires qualification."}`
- Tokenizer: `{"vocab_size": 129280, "files": ["tokenizer.json", "tokenizer_config.json", "assets/chat_template.jinja"], "exact_file_hashes": null}`
- Chat template: Hybrid thinking/non-thinking. Non-thinking generation prefix closes thinking with </think>; thinking prefix opens <think>.
- Tool use: Publisher documents structured tool tokens and JSON arguments; non-thinking tool calls and search-agent templates are documented.
- License/gating: MIT; Repository was publicly readable without an access gate at capture time; future availability is not guaranteed.

## [CONVERTER] Selected artifact

- Quantization: `UD-IQ2_XXS`
- Display size: 226 GB (Hugging Face UI, rounded)
- Exact total bytes: `UNAVAILABLE`
- Known exact subtotal: `48771619904`
- Manifest completeness: `PARTIAL`
- Imatrix/calibration: `{"file": "imatrix_unsloth.gguf_file", "display_size": "about 1.01 GB", "sha256": null, "prompt_corpus": null, "method": "Unsloth Dynamic quantization; exact calibration corpus and recipe not captured", "provenance_status": "PARTIAL_PROVENANCE"}`

| File | Exact bytes | SHA-256 | Rounded display |
|---|---:|---|---|
| `DeepSeek-V3.1-UD-IQ2_XXS-00001-of-00005.gguf` | 48771619904 | `cfe213b5cd15f7f311b3e6e65e69193794d4d745f2984efa505e852bb7875b71` |  |
| `DeepSeek-V3.1-UD-IQ2_XXS-00002-of-00005.gguf` | UNAVAILABLE | `UNAVAILABLE` | 49.3 GB |
| `DeepSeek-V3.1-UD-IQ2_XXS-00003-of-00005.gguf` | UNAVAILABLE | `af31108fe922eb2b8559313f32c2795bb0e805c99b14244e575960517ee19a2a` | 49.4 GB |
| `DeepSeek-V3.1-UD-IQ2_XXS-00004-of-00005.gguf` | UNAVAILABLE | `UNAVAILABLE` | 49.3 GB |
| `DeepSeek-V3.1-UD-IQ2_XXS-00005-of-00005.gguf` | UNAVAILABLE | `UNAVAILABLE` | 29.1 GB |

## [STATIC-SOURCE] Runtime review

- llama.cpp architecture: `deepseek2`
- Graph: Concrete DeepSeek2 class and MLA/MoE graph present.
- MTP/speculative: No explicit decoder-MTP graph path identified in deepseek2.cpp at pinned revision.
- ROCmFPX: HIP/CUDA backend contains generic MoE, attention, and matrix paths; candidate execution not tested.

## [LOCAL-VALIDATION] Required evidence

No claim is made that this artifact fits, loads, runs correctly, avoids CPU fallback, supports its published maximum context, uses tools correctly, supports multimodal input, executes MTP/speculation, or meets quality targets.

## Risks / blockers

- Selected artifact exact bytes/hashes incomplete
- MLA memory behavior and long-context state unqualified
- MTP/speculative acceleration not established
- Quant quality not measured locally

## Source IDs

- `SRC-DSV31-PUBLISHER`
- `SRC-DSV31-CONFIG`
- `SRC-DSV31-CONVERSION`
- `SRC-LLAMACPP-MODELS`
- `SRC-ROCMFPX-OPS`
