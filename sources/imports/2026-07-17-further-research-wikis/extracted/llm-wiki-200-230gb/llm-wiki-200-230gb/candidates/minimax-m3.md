# MiniMax-M3

**Rank:** 8 · **Score:** 55/100 · **Disposition:** `experimental`

## Selected representation

| Field | Value |
|---|---|
| Architecture | minimax-m3 |
| Class | MoE + MiniMax Sparse Attention |
| Total parameters | 428B |
| Activated parameters/token | 23B |
| Layers | 60 |
| Experts / active | 128 / 4 |
| Native context | 1,048,576 |
| License | MiniMax Community License |
| Chosen quant | UD-IQ4_XS |
| Publisher size | 208.00 decimal GB |
| Conservative weight plan | 195 GiB |
| Artifact effective bits/parameter | 3.888 |

## Selection rationale

Capacity fits, but it is a research candidate until the exact runtime, MSA path, and cache behavior are measured.

## Runtime support

- **llama.cpp:** Not production-ready at snapshot: quant card points to a PR and says MSA falls back to dense attention.
- **ROCmFPX:** Yes — dedicated minimax-m3 implementation and smoke tests present.
- **Runtime risk:** High: preliminary mainline status and sparse-attention semantics are not yet a stable deployment baseline.

## Tokenizer and chat template

- **Tokenizer:** Embedded tokenizer expected; verify exact converter build.
- **Template:** MiniMax thinking/non-thinking template; exact GGUF/template pair must be validated.

## KV cache

**Formula status:** measurement-gate: MSA cache allocation not verified in pinned mainline

| Context tokens | Q8_0 exact GiB | Q8 planning GiB |
|---|---|---|
| 32,768 | measurement gate | — |
| 65,536 | measurement gate | — |
| 131,072 | measurement gate | — |
| 1,048,576 | measurement gate | — |

## Expert activation and buffers

The router activates 4 of 128 routed experts per token plus 1 shared expert(s). All expert weights remain resident.

Graph/work buffers are not predicted from parameter count. Use the package reserve profiles, then replace them with measured startup logs.

## Quality evidence

Official card reports coding, cowork, reasoning, and long-context results; deployment maturity remains the gating factor.

**Selected-quant assessment:** Moderate-high: IQ4_XS; no selected-quant independent evaluation located.

## Download provenance

- Upstream: `MiniMaxAI/MiniMax-M3`
- Quant repo: `unsloth/MiniMax-M3-GGUF`
- Revision: `41b3ee5f52f642949301cb1fc34cf8379ba22416`
- Path: `UD-IQ4_XS`
- Shard count: `6`

Known exact LFS pointers:

- No exact shard hash captured in this snapshot; use the revision/path manifest and refresh script.

## Acceptance gates

- Refresh and verify every shard hash.
- Load with the pinned runtime and exact embedded Jinja template.
- Run the production quality suite against a higher-bit reference.
- Capture per-device/node weights, KV, graph, and compute buffers.
- Benchmark the intended backend/topology; do not use parameter-count throughput extrapolation.

## Sources

- [minimax_upstream](https://huggingface.co/MiniMaxAI/MiniMax-M3)
- [minimax_quant](https://huggingface.co/unsloth/MiniMax-M3-GGUF)
