# Kimi-K2-Thinking

**Rank:** 9 · **Score:** 35/100 · **Disposition:** `screened-out`

## Selected representation

| Field | Value |
|---|---|
| Architecture | deepseek2-derived |
| Class | MoE |
| Total parameters | 1000B |
| Activated parameters/token | 32B |
| Layers | 61 |
| Experts / active | 384 / 8 |
| Native context | 262,144 |
| License | Modified MIT / model-specific terms — verify upstream |
| Chosen quant | i1-IQ1_M community artifact |
| Publisher size | 228.00 decimal GB |
| Conservative weight plan | 214 GiB |
| Artifact effective bits/parameter | 1.824 |

## Selection rationale

Included as a boundary case only; it meets the file-size filter but fails the practical quality/provenance screen.

## Runtime support

- **llama.cpp:** Architecture family is supported, but the exact community conversion is not qualified here.
- **ROCmFPX:** Architecture-family support expected; exact artifact not qualified.
- **Runtime risk:** High: extreme size, community conversion, and low-bit kernels.

## Tokenizer and chat template

- **Tokenizer:** Community conversion; verify tokenizer and special tokens.
- **Template:** Community conversion; template provenance must be checked against upstream.

## KV cache

**Formula status:** not capacity-qualified in this package

| Context tokens | Q8_0 exact GiB | Q8 planning GiB |
|---|---|---|
| 32,768 | measurement gate | — |
| 65,536 | measurement gate | — |
| 131,072 | measurement gate | — |
| 262,144 | measurement gate | — |

## Expert activation and buffers

The router activates 8 of 384 routed experts per token plus 1 shared expert(s). All expert weights remain resident.

Graph/work buffers are not predicted from parameter count. Use the package reserve profiles, then replace them with measured startup logs.

## Quality evidence

No acceptable selected-quant quality evidence located.

**Selected-quant assessment:** Very high: about 1.82 effective bits/parameter; the community card labels the quant as suitable only for the mostly desperate.

## Download provenance

- Upstream: `moonshotai/Kimi-K2-Thinking`
- Quant repo: `mradermacher/Kimi-K2-Thinking-i1-GGUF`
- Revision: `main`
- Path: ``
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

- [kimi_upstream](https://huggingface.co/moonshotai/Kimi-K2-Thinking)
- [kimi_quant](https://huggingface.co/mradermacher/Kimi-K2-Thinking-i1-GGUF)
