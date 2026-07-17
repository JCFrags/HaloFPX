# Step-3.7-Flash

**Rank:** 2 · **Score:** 90/100 · **Disposition:** `shortlist`

## Selected representation

| Field | Value |
|---|---|
| Architecture | step35 |
| Class | MoE + hybrid attention |
| Total parameters | 198B |
| Activated parameters/token | 11B |
| Layers | 45 |
| Experts / active | 288 / 8 |
| Native context | 262,144 |
| License | Apache-2.0 |
| Chosen quant | Q8_0 |
| Publisher size | 209.00 decimal GB |
| Conservative weight plan | 196 GiB |
| Artifact effective bits/parameter | 8.444 |

## Selection rationale

Highest-fidelity target-band artifact and only ~11B active parameters; selected Q8 artifact has direct quant evidence.

## Runtime support

- **llama.cpp:** Yes — step35 implementation, hybrid SWA cache, and MTP paths present.
- **ROCmFPX:** Yes — step35 plus a released ROCmFPX Q3 QualityPlus recipe.
- **Runtime risk:** Medium: newer architecture and multimodal/MTP paths make revision pinning important.

## Tokenizer and chat template

- **Tokenizer:** Embedded tokenizer; verify BOS/EOS and multimodal special tokens.
- **Template:** Embedded Step Jinja; vision requires the matching mmproj sidecar.

## KV cache

**Formula status:** verified-from-config-and-current-iswa-cache-code

| Context tokens | Q8_0 exact GiB | Q8 planning GiB |
|---|---|---|
| 32,768 | 0.865 | 1.00 |
| 65,536 | 1.662 | 1.75 |
| 131,072 | 3.256 | 3.50 |
| 262,144 | 6.443 | 6.50 |

## Expert activation and buffers

The router activates 8 of 288 routed experts per token plus 1 shared expert(s). All expert weights remain resident.

Graph/work buffers are not predicted from parameter count. Use the package reserve profiles, then replace them with measured startup logs.

## Quality evidence

Official card supplies broad benchmark evidence; AesSedai reports Q8_0 PPL 1.894568 and KLD 0.005301 for its evaluation setup.

**Selected-quant assessment:** Low for Q8_0; direct quant-specific PPL/KLD evidence is available.

## Download provenance

- Upstream: `stepfun-ai/Step-3.7-Flash`
- Quant repo: `stepfun-ai/Step-3.7-Flash-GGUF`
- Revision: `713961b`
- Path: `Q8_0`
- Shard count: `5`

Known exact LFS pointers:

- No exact shard hash captured in this snapshot; use the revision/path manifest and refresh script.

## Acceptance gates

- Refresh and verify every shard hash.
- Load with the pinned runtime and exact embedded Jinja template.
- Run the production quality suite against a higher-bit reference.
- Capture per-device/node weights, KV, graph, and compute buffers.
- Benchmark the intended backend/topology; do not use parameter-count throughput extrapolation.

## Sources

- [step_upstream](https://huggingface.co/stepfun-ai/Step-3.7-Flash)
- [step_quant](https://huggingface.co/stepfun-ai/Step-3.7-Flash-GGUF/tree/713961b/Q8_0)
- [step_quant_eval](https://huggingface.co/AesSedai/Step-3.7-Flash-GGUF)
- [llama_step35](https://github.com/ggml-org/llama.cpp/blob/6bdd77f13cf11b264b4231d320afc404f48d576e/src/models/step35.cpp)
- [rocmfpx_step_recipe](https://github.com/charlie12345/ROCmFPX/blob/61f2f2d7bc4955e9bca821095ef69125837133b5/docs/recipes/step35-rocmfpx-q3-qualityplus.md)
