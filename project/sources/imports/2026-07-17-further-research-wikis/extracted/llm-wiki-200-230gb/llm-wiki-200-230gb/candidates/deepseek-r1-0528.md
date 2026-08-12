# DeepSeek-R1-0528

**Rank:** 6 · **Score:** 68/100 · **Disposition:** `conditional`

## Selected representation

| Field | Value |
|---|---|
| Architecture | deepseek2 |
| Class | MoE + MLA |
| Total parameters | 671B |
| Activated parameters/token | 37B |
| Layers | 61 |
| Experts / active | 256 / 8 |
| Native context | 163,840 |
| License | MIT |
| Chosen quant | UD-IQ2_M |
| Publisher size | 229.00 decimal GB |
| Conservative weight plan | 215 GiB |
| Artifact effective bits/parameter | 2.730 |

## Selection rationale

Exceptional upstream reasoning evidence and small MLA cache, but the target-band representation reaches the band only via aggressive 2-bit quantization.

## Runtime support

- **llama.cpp:** Yes — deepseek2 implementation with MLA absorption.
- **ROCmFPX:** Yes — deepseek2 implementation present.
- **Runtime risk:** Medium: large artifact, specialized MLA graph, and reasoning-generation behavior require end-to-end validation.

## Tokenizer and chat template

- **Tokenizer:** Embedded DeepSeek tokenizer.
- **Template:** R1-0528 template; system prompts are supported in this release. Do not force a <think> prefix.

## KV cache

**Formula status:** verified-from-current-MLA-absorption-implementation

| Context tokens | Q8_0 exact GiB | Q8 planning GiB |
|---|---|---|
| 32,768 | 1.139 | 1.25 |
| 65,536 | 2.279 | 2.50 |
| 131,072 | 4.557 | 4.75 |
| 163,840 | 5.696 | 5.75 |

## Expert activation and buffers

The router activates 8 of 256 routed experts per token plus 1 shared expert(s). All expert weights remain resident.

Graph/work buffers are not predicted from parameter count. Use the package reserve profiles, then replace them with measured startup logs.

## Quality evidence

Official BF16 card reports MMLU-Pro 85.0, GPQA 81.0, AIME 2025 87.5, and SWE-bench Verified 57.6.

**Selected-quant assessment:** High: effective storage is only about 2.73 bits/parameter and no direct selected-quant evaluation was found.

## Download provenance

- Upstream: `deepseek-ai/DeepSeek-R1-0528`
- Quant repo: `unsloth/DeepSeek-R1-0528-GGUF`
- Revision: `main-observed-916bb7c`
- Path: `UD-IQ2_M`
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

- [deepseek_upstream](https://huggingface.co/deepseek-ai/DeepSeek-R1-0528)
- [deepseek_quant](https://huggingface.co/unsloth/DeepSeek-R1-0528-GGUF/tree/main/UD-IQ2_M)
- [llama_deepseek2](https://github.com/ggml-org/llama.cpp/blob/6bdd77f13cf11b264b4231d320afc404f48d576e/src/models/deepseek2.cpp)
