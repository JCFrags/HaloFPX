# DeepSeek-R1

**Decision:** `ADVANCE_TO_LOCAL_PREFLIGHT`  
**Publisher identity:** `deepseek-ai/DeepSeek-R1@56d4cbbb4d29f4355bab4b9a39ccb717a14ad5ad`  
**Selected artifact:** `unsloth/DeepSeek-R1-GGUF@4dab99fdb3fea560e5a402b49c6ef11e025321a8/DeepSeek-R1-Q2_K_XS`  
**Local validation:** `NOT_RUN`

## [PUBLISHER] Architecture and state

- Parameters: 671B total / 37B activated
- Architecture: DeepSeek2/DeepSeek-V3-family MLA sparse MoE reasoning model
- State requirements: MLA latent attention state; routed/shared expert weights; reasoning template discipline
- Context: `{"publisher_card": "128K", "qualification": "No local maximum-context claim."}`
- Tokenizer: `{"vocab_size": 129280, "files": ["tokenizer.json", "tokenizer_config.json"], "exact_file_hashes": null}`
- Chat template: Reasoning-oriented DeepSeek template. Publisher usage guidance recommends avoiding a system prompt in benchmark-style use.
- Tool use: No publisher-authoritative structured tool contract was accepted as established for this shortlist.
- License/gating: MIT; Publicly readable at capture time.

## [CONVERTER] Selected artifact

- Quantization: `Q2_K_XS`
- Display size: 221 GB-class
- Exact total bytes: `221253686944`
- Known exact subtotal: `221253686944`
- Manifest completeness: `EXACT_SELECTED_SHARDS`
- Imatrix/calibration: `{"file": "imatrix_unsloth.dat", "display_size": "about 987 MB", "sha256": null, "prompt_corpus": null, "method": "Unsloth imatrix; exact prompt corpus and command not captured", "provenance_status": "PARTIAL_PROVENANCE"}`

| File | Exact bytes | SHA-256 | Rounded display |
|---|---:|---|---|
| `DeepSeek-R1-Q2_K_XS-00001-of-00005.gguf` | 49797465088 | `df8e381870dd3aa2ea20b698f5484a1493fffe5503cae9dd4dcdd6ac56319e6d` |  |
| `DeepSeek-R1-Q2_K_XS-00002-of-00005.gguf` | 49174169600 | `87b29033b0988ebfdce71cc0678b4cbb0954c00562be6c71f529001b9ab7244f` |  |
| `DeepSeek-R1-Q2_K_XS-00003-of-00005.gguf` | 49174169600 | `03dfc0f75f94710ccace9e9ef39cd93f691cc05a33dbc425f0d4237719218a68` |  |
| `DeepSeek-R1-Q2_K_XS-00004-of-00005.gguf` | 49174169600 | `cb3125f56b745364cb552660fcee089163f99aa8e2d8ebbbff70722b3684e7de` |  |
| `DeepSeek-R1-Q2_K_XS-00005-of-00005.gguf` | 23933713056 | `84d95c85fb5ae9be44540a0491e80bdf8557665cafbb94d65dd948333b3f3e5f` |  |

## [STATIC-SOURCE] Runtime review

- llama.cpp architecture: `deepseek2`
- Graph: Concrete MLA/MoE graph present.
- MTP/speculative: No explicit decoder-MTP graph identified in pinned deepseek2.cpp.
- ROCmFPX: Generic attention/MoE paths present; no local run.

## [LOCAL-VALIDATION] Required evidence

No claim is made that this artifact fits, loads, runs correctly, avoids CPU fallback, supports its published maximum context, uses tools correctly, supports multimodal input, executes MTP/speculation, or meets quality targets.

## Risks / blockers

- Aggressive Q2 quality must be measured against workload reference
- Tool-call suitability not publisher-established
- MLA and long-context memory behavior unqualified
- MTP acceleration not established

## Source IDs

- `SRC-DSR1-PUBLISHER`
- `SRC-DSR1-CONVERSION`
- `SRC-LLAMACPP-MODELS`
- `SRC-ROCMFPX-OPS`
