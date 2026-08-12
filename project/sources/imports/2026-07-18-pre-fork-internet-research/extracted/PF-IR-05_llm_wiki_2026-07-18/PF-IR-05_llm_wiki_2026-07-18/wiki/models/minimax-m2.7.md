# MiniMax-M2.7

**Decision:** `HOLD_LICENSE_AND_CONVERSION_PREFLIGHT`  
**Publisher identity:** `MiniMaxAI/MiniMax-M2.7@d494266a4affc0d2995ba1fa35c8481cbd84294b`  
**Selected artifact:** `unsloth/MiniMax-M2.7-GGUF@04ab68de18da5486aa9869e3bc72f0249ef51f89/UD-Q6_K_XL`  
**Local validation:** `NOT_RUN`

## [PUBLISHER] Architecture and state

- Parameters: Publisher repository/model metadata: about 229B/230B total; pinned llama.cpp static type label: 230B-A10B
- Architecture: MiniMax-M2 sparse MoE transformer; config exposes one MTP transformer layer
- State requirements: attention KV cache; routed expert weights; MTP layer state if used
- Context: `{"config_max_position_embeddings": 204800, "qualification": "No local maximum-context claim."}`
- Tokenizer: `{"files": ["tokenizer.json", "tokenizer_config.json", "chat template/custom-code files"], "trust_remote_code": "Publisher examples use trust_remote_code=True", "exact_file_hashes": null}`
- Chat template: Default system prompt and sampling recommendations are documented; exact template file must be hashed locally.
- Tool use: Publisher markets agent teams, tool use, skill use, and dynamic tool search. Workload quality remains unqualified.
- License/gating: Custom non-commercial license; commercial use requires prior written authorization and attribution; prohibited-use appendix.; Publicly readable at capture time; legal terms are the blocking gate.

## [CONVERTER] Selected artifact

- Quantization: `UD-Q6_K_XL`
- Display size: 207 GB (Hugging Face UI, rounded)
- Exact total bytes: `UNAVAILABLE`
- Known exact subtotal: `0`
- Manifest completeness: `NAMES_AND_PARTIAL_ROUNDED_SIZES_ONLY`
- Imatrix/calibration: `{"file": "imatrix_unsloth.dat", "display_size": "about 492 MB", "sha256": null, "prompt_corpus": null, "method": "Unsloth imatrix; exact corpus and command not captured", "provenance_status": "PARTIAL_PROVENANCE"}`

| File | Exact bytes | SHA-256 | Rounded display |
|---|---:|---|---|
| `MiniMax-M2.7-UD-Q6_K_XL-00001-of-00006.gguf` | UNAVAILABLE | `UNAVAILABLE` | 8.24 MB |
| `MiniMax-M2.7-UD-Q6_K_XL-00002-of-00006.gguf` | UNAVAILABLE | `UNAVAILABLE` | 49.0 GB |
| `MiniMax-M2.7-UD-Q6_K_XL-00003-of-00006.gguf` | UNAVAILABLE | `UNAVAILABLE` | 49.7 GB |
| `MiniMax-M2.7-UD-Q6_K_XL-00004-of-00006.gguf` | UNAVAILABLE | `UNAVAILABLE` | 49.7 GB |
| `MiniMax-M2.7-UD-Q6_K_XL-00005-of-00006.gguf` | UNAVAILABLE | `UNAVAILABLE` | 49.7 GB |
| `MiniMax-M2.7-UD-Q6_K_XL-00006-of-00006.gguf` | UNAVAILABLE | `UNAVAILABLE` | UNAVAILABLE |

## [STATIC-SOURCE] Runtime review

- llama.cpp architecture: `minimax-m2`
- Graph: Concrete attention + MoE graph present.
- MTP/speculative: No explicit decoder-MTP graph identified in pinned minimax-m2.cpp.
- ROCmFPX: Generic MoE/attention source present; no run.

## [LOCAL-VALIDATION] Required evidence

No claim is made that this artifact fits, loads, runs correctly, avoids CPU fallback, supports its published maximum context, uses tools correctly, supports multimodal input, executes MTP/speculation, or meets quality targets.

## Risks / blockers

- Commercial deployment prohibited absent written authorization
- Selected shard hashes/bytes incomplete
- Custom code/template lineage must be reviewed
- MTP mismatch and backend behavior unqualified

## Source IDs

- `SRC-MINIMAX-PUBLISHER`
- `SRC-MINIMAX-LICENSE`
- `SRC-MINIMAX-CONVERSION`
- `SRC-LLAMACPP-MODELS`
- `SRC-ROCMFPX-OPS`
