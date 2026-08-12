# MiMo-V2-Flash

**Rank:** 3 · **Score:** 86/100 · **Disposition:** `shortlist`

## Selected representation

| Field | Value |
|---|---|
| Architecture | mimo2 |
| Class | MoE + hybrid attention |
| Total parameters | 309B |
| Activated parameters/token | 15B |
| Layers | 48 |
| Experts / active | 256 / 8 |
| Native context | 262,144 |
| License | MIT (model weights; verify code components separately) |
| Chosen quant | Q5_K_M |
| Publisher size | 219.20 decimal GB |
| Conservative weight plan | 206 GiB |
| Artifact effective bits/parameter | 5.675 |

## Selection rationale

Strong active-compute efficiency and exceptionally slow KV growth because most layers use a 128-token sliding window.

## Runtime support

- **llama.cpp:** Yes — dedicated mimo2 implementation and hybrid SWA cache.
- **ROCmFPX:** Yes — mimo2 implementation present.
- **Runtime risk:** Medium-low: implementation is current, but the model is newer and MTP tensors are preserved/disabled in the main pass.

## Tokenizer and chat template

- **Tokenizer:** Embedded tokenizer in GGUF.
- **Template:** Embedded MiMo Jinja; use --jinja and validate reasoning/tool modes against upstream examples.

## KV cache

**Formula status:** verified-from-config-and-current-iswa-cache-code

| Context tokens | Q8_0 exact GiB | Q8 planning GiB |
|---|---|---|
| 32,768 | 0.449 | 0.50 |
| 65,536 | 0.823 | 1.00 |
| 131,072 | 1.570 | 1.75 |
| 262,144 | 3.064 | 3.25 |

## Expert activation and buffers

The router activates 8 of 256 routed experts per token. All expert weights remain resident.

Graph/work buffers are not predicted from parameter count. Use the package reserve profiles, then replace them with measured startup logs.

## Quality evidence

Official model card reports benchmark results for coding, reasoning, and agents; use only as upstream evidence.

**Selected-quant assessment:** Moderate: Q5_K_M is materially safer than 4-bit options, but no candidate-specific independent quant benchmark was located.

## Download provenance

- Upstream: `XiaomiMiMo/MiMo-V2-Flash`
- Quant repo: `bartowski/XiaomiMiMo_MiMo-V2-Flash-GGUF`
- Revision: `6b8a0ba`
- Path: `XiaomiMiMo_MiMo-V2-Flash-Q5_K_M`
- Shard count: `6`

Known exact LFS pointers:

- `XiaomiMiMo_MiMo-V2-Flash-Q5_K_M-00002-of-00006.gguf` — SHA-256 `1014b460405b8f1db647ce974ebc5ea7c5466ecfd2a486b40eb3c8a749abd8e2`, `39809416864` bytes

## Acceptance gates

- Refresh and verify every shard hash.
- Load with the pinned runtime and exact embedded Jinja template.
- Run the production quality suite against a higher-bit reference.
- Capture per-device/node weights, KV, graph, and compute buffers.
- Benchmark the intended backend/topology; do not use parameter-count throughput extrapolation.

## Sources

- [mimo_upstream](https://huggingface.co/XiaomiMiMo/MiMo-V2-Flash)
- [mimo_quant](https://huggingface.co/bartowski/XiaomiMiMo_MiMo-V2-Flash-GGUF/tree/6b8a0ba/XiaomiMiMo_MiMo-V2-Flash-Q5_K_M)
- [llama_mimo2](https://github.com/ggml-org/llama.cpp/blob/6bdd77f13cf11b264b4231d320afc404f48d576e/src/models/mimo2.cpp)
