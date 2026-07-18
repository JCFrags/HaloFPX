# NVIDIA Nemotron 3 Ultra 550B-A55B

**Decision:** `ADVANCE_TO_LOCAL_PREFLIGHT`  
**Publisher identity:** `nvidia/NVIDIA-Nemotron-3-Ultra-550B-A55B-BF16@624ba927cfbef0427354998700de3d51173c8c04`  
**Selected artifact:** `unsloth/NVIDIA-Nemotron-3-Ultra-550B-A55B-GGUF@05c80881664d32be9b26bab9d0360be00230ade4/UD-IQ3_XXS`  
**Local validation:** `NOT_RUN`

## [PUBLISHER] Architecture and state

- Parameters: 550B total / 55B activated
- Architecture: Mamba-2/Transformer hybrid LatentMoE with selected attention layers and MTP
- State requirements: Mamba-2 recurrent state and convolution state; attention KV cache on attention layers; LatentMoE projections and expert weights; MTP state if accelerated
- Context: `{"maximum": "up to 1M tokens", "qualification": "No local long-context or memory claim."}`
- Tokenizer: `{"files": ["tokenizer.json", "tokenizer_config.json", "chat_template.jinja"], "exact_file_hashes": null}`
- Chat template: Configurable thinking on/off through chat template.
- Tool use: Publisher positions model for agentic workflows and tool use; exact workload behavior remains a local quality gate.
- License/gating: OpenMDW-1.1; Publicly readable at capture time.

## [CONVERTER] Selected artifact

- Quantization: `UD-IQ3_XXS`
- Display size: 223 GB (Hugging Face UI, rounded)
- Exact total bytes: `UNAVAILABLE`
- Known exact subtotal: `0`
- Manifest completeness: `NAMES_AND_ROUNDED_SIZES_ONLY`
- Imatrix/calibration: `{"file": null, "display_size": null, "sha256": null, "prompt_corpus": null, "method": "Unsloth Dynamic 2.0; selected artifact calibration material not captured", "provenance_status": "UNAVAILABLE"}`

| File | Exact bytes | SHA-256 | Rounded display |
|---|---:|---|---|
| `NVIDIA-Nemotron-3-Ultra-550B-A55B-UD-IQ3_XXS-00001-of-00006.gguf` | UNAVAILABLE | `UNAVAILABLE` | 7.87 MB |
| `NVIDIA-Nemotron-3-Ultra-550B-A55B-UD-IQ3_XXS-00002-of-00006.gguf` | UNAVAILABLE | `UNAVAILABLE` | 48.1 GB |
| `NVIDIA-Nemotron-3-Ultra-550B-A55B-UD-IQ3_XXS-00003-of-00006.gguf` | UNAVAILABLE | `UNAVAILABLE` | 48.3 GB |
| `NVIDIA-Nemotron-3-Ultra-550B-A55B-UD-IQ3_XXS-00004-of-00006.gguf` | UNAVAILABLE | `UNAVAILABLE` | 48.1 GB |
| `NVIDIA-Nemotron-3-Ultra-550B-A55B-UD-IQ3_XXS-00005-of-00006.gguf` | UNAVAILABLE | `UNAVAILABLE` | 48.4 GB |
| `NVIDIA-Nemotron-3-Ultra-550B-A55B-UD-IQ3_XXS-00006-of-00006.gguf` | UNAVAILABLE | `UNAVAILABLE` | 30.5 GB |

## [STATIC-SOURCE] Runtime review

- llama.cpp architecture: `nemotron_h_moe`
- Graph: Concrete Mamba2 recurrent, attention, and MoE graph paths are present.
- MTP/speculative: No explicit decoder-MTP graph identified in pinned nemotron-h model source.
- ROCmFPX: Pinned backend includes SSM scan/conv sources; execution and performance are untested.

## [LOCAL-VALIDATION] Required evidence

No claim is made that this artifact fits, loads, runs correctly, avoids CPU fallback, supports its published maximum context, uses tools correctly, supports multimodal input, executes MTP/speculation, or meets quality targets.

## Risks / blockers

- Selected artifact lacks exact shard pointers in capture
- Hybrid recurrent state is backend-sensitive
- MTP not established in pinned llama.cpp graph
- OpenMDW terms require legal review despite publisher commercial-use statement

## Source IDs

- `SRC-NEMOTRON-PUBLISHER`
- `SRC-NEMOTRON-CONVERSION`
- `SRC-OPENMDW`
- `SRC-LLAMACPP-MODELS`
- `SRC-ROCMFPX-OPS`
