# Qwen3.5-397B-A17B

**Decision:** `ADVANCE_TO_LOCAL_PREFLIGHT`  
**Publisher identity:** `Qwen/Qwen3.5-397B-A17B@8472618112abcbd45acbcdc58436aff4233c23f7`  
**Selected artifact:** `unsloth/Qwen3.5-397B-A17B-GGUF@0b7386368898883d58d445525ba8fa45e3f7289f/UD-Q4_K_S`  
**Local validation:** `NOT_RUN`

## [PUBLISHER] Architecture and state

- Parameters: 397B total / 17B activated
- Architecture: Multimodal causal model with vision encoder; 60-layer hybrid Gated DeltaNet + gated attention sparse MoE
- State requirements: recurrent DeltaNet state; attention KV cache; 512 routed-expert set plus shared expert; vision encoder/projector assets for multimodal use
- Context: `{"native": 262144, "extended": 1010000, "qualification": "Extension requires explicit RoPE/YaRN configuration and local validation."}`
- Tokenizer: `{"embedding_output_padded": 248320, "files": ["tokenizer.json", "tokenizer_config.json", "chat_template.jinja"], "exact_file_hashes": null}`
- Chat template: Thinking is default; disable with enable_thinking=false. Thinking history handling is template-dependent.
- Tool use: Publisher documents qwen3 reasoning parser, qwen3_coder tool parser, Qwen-Agent, and MCP integration.
- License/gating: Apache-2.0; Publicly readable at capture time.

## [CONVERTER] Selected artifact

- Quantization: `UD-Q4_K_S`
- Display size: 228 GB (Hugging Face UI, rounded)
- Exact total bytes: `UNAVAILABLE`
- Known exact subtotal: `196633537600`
- Manifest completeness: `PARTIAL_5_OF_6_EXACT`
- Imatrix/calibration: `{"file": null, "display_size": null, "sha256": null, "prompt_corpus": null, "method": "Converter states refreshed imatrix/improved algorithm; corpus and reproducible command not published in captured evidence", "provenance_status": "PARTIAL_PROVENANCE"}`

| File | Exact bytes | SHA-256 | Rounded display |
|---|---:|---|---|
| `Qwen3.5-397B-A17B-UD-Q4_K_S-00001-of-00006.gguf` | 10943552 | `0c281dd4cc77769c0945704f770dbf950950e23c45680c64caa8e680b76fc0cc` |  |
| `Qwen3.5-397B-A17B-UD-Q4_K_S-00002-of-00006.gguf` | 49649743104 | `817bc274bbc50b0b093e6a7c7f4a236304de9b5e572b1a20f74ce2aece58f292` |  |
| `Qwen3.5-397B-A17B-UD-Q4_K_S-00003-of-00006.gguf` | 48983522144 | `05f99ad3a60c3afc69911ddcdc36494bc50f26ba8825db66f64a0880ce3c05f0` |  |
| `Qwen3.5-397B-A17B-UD-Q4_K_S-00004-of-00006.gguf` | 48983522144 | `b3af6efac978262e7242f0ebfdf9dc438697e1dfd1b61c35dec848613ea4ba4e` |  |
| `Qwen3.5-397B-A17B-UD-Q4_K_S-00005-of-00006.gguf` | 49005806656 | `b2519d7617f8c8d350c07d37ae2fa46b56157fa11d197ea5d939e6f443418de5` |  |
| `Qwen3.5-397B-A17B-UD-Q4_K_S-00006-of-00006.gguf` | UNAVAILABLE | `UNAVAILABLE` |  |

## [STATIC-SOURCE] Runtime review

- llama.cpp architecture: `qwen35moe`
- Graph: Concrete hybrid recurrent/full-attention graph and recurrent memory are present.
- MTP/speculative: Explicit decoder-MTP graph path and NextN tensors are present.
- ROCmFPX: Pinned backend contains gated-delta-net and SSM source files/includes; no machine execution or fallback trace.

## [LOCAL-VALIDATION] Required evidence

No claim is made that this artifact fits, loads, runs correctly, avoids CPU fallback, supports its published maximum context, uses tools correctly, supports multimodal input, executes MTP/speculation, or meets quality targets.

## Risks / blockers

- Sixth selected shard exact bytes/hash unavailable
- Recurrent state semantics require sequence/reset/copy tests
- Multimodal projector/vision assets and operator coverage unqualified
- MTP must be tested separately from base decoding

## Source IDs

- `SRC-QWEN35-PUBLISHER`
- `SRC-QWEN35-CONVERSION`
- `SRC-LLAMACPP-MODELS`
- `SRC-ROCMFPX-OPS`
