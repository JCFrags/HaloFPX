# GLM-4.7

**Decision:** `ADVANCE_TO_LOCAL_PREFLIGHT`  
**Publisher identity:** `zai-org/GLM-4.7@602d01efcdd332c5238ca4bcede555defbe83eb7`  
**Selected artifact:** `unsloth/GLM-4.7-GGUF@a030db4a327b2dece3f0e5c4ff82c5f4feab4f26/Q4_K_M`  
**Local validation:** `NOT_RUN`

## [PUBLISHER] Architecture and state

- Parameters: Publisher card: 358B total / 32B activated; pinned llama.cpp static type label: 355B-A32B
- Architecture: GLM4 MoE transformer with interleaved/preserved thinking and a NextN layer
- State requirements: standard attention KV cache; routed/shared expert weights; thinking-history/template state
- Context: `{"publisher_card": "128K operating guidance", "config_max_position_embeddings": 202752, "discrepancy": "Preserved; no local max-context claim."}`
- Tokenizer: `{"files": ["tokenizer.json", "tokenizer_config.json", "chat_template.jinja"], "exact_file_hashes": null}`
- Chat template: Thinking is enabled by default in documented engines; preserved-thinking and per-turn thinking controls are documented.
- Tool use: OpenAI-style tools; publisher documents thinking before actions and tool calls.
- License/gating: MIT; Publicly readable at capture time.

## [CONVERTER] Selected artifact

- Quantization: `Q4_K_M`
- Display size: 216 GB-class
- Exact total bytes: `216455572576`
- Known exact subtotal: `216455572576`
- Manifest completeness: `EXACT_SELECTED_SHARDS`
- Imatrix/calibration: `{"file": "imatrix_unsloth.dat", "display_size": "about 688 MB", "sha256": null, "prompt_corpus": null, "method": "Unsloth imatrix; corpus and exact generation command not captured", "provenance_status": "PARTIAL_PROVENANCE"}`

| File | Exact bytes | SHA-256 | Rounded display |
|---|---:|---|---|
| `GLM-4.7-Q4_K_M-00001-of-00005.gguf` | 50006655232 | `85b45d0fe56295af49a080dc498882173ac8e90f60e6c5eab9a995c3564e754a` |  |
| `GLM-4.7-Q4_K_M-00002-of-00005.gguf` | 49705685664 | `732329c8d48b5468bf3a54355bf5d155471640041a9963433f0931b7fc5226b1` |  |
| `GLM-4.7-Q4_K_M-00003-of-00005.gguf` | 49631164960 | `e6d241ebfeb59a09a25237a4b2bf8efc05806b4b067aa51c7ca1c9f530ffc736` |  |
| `GLM-4.7-Q4_K_M-00004-of-00005.gguf` | 49501239424 | `96e609ec3c6259ab82be1ee322424588fde98816431030175f82af561e1457ec` |  |
| `GLM-4.7-Q4_K_M-00005-of-00005.gguf` | 17610827296 | `2ffdc358f1f71f7f96952408e1e0202f17ecf0a527783018d0b83b186d125f31` |  |

## [STATIC-SOURCE] Runtime review

- llama.cpp architecture: `glm4moe`
- Graph: Concrete GLM4-MoE model graph present.
- MTP/speculative: NextN tensors are loaded with skip flags and excluded from forward execution in pinned source.
- ROCmFPX: Generic attention/MoE source is present; no local dispatch trace.

## [LOCAL-VALIDATION] Required evidence

No claim is made that this artifact fits, loads, runs correctly, avoids CPU fallback, supports its published maximum context, uses tools correctly, supports multimodal input, executes MTP/speculation, or meets quality targets.

## Risks / blockers

- MTP speculative path absent in pinned graph
- Publisher parameter total differs from static runtime type label
- Long-context memory and quality unqualified

## Source IDs

- `SRC-GLM47-PUBLISHER`
- `SRC-GLM47-CONVERSION`
- `SRC-LLAMACPP-MODELS`
- `SRC-ROCMFPX-OPS`
