# Llama-3.1-Nemotron-Ultra-253B-v1

**Rank:** 5 · **Score:** 76/100 · **Disposition:** `shortlist`

## Selected representation

| Field | Value |
|---|---|
| Architecture | deci |
| Class | Dense NAS-pruned |
| Total parameters | 253B |
| Activated parameters/token | 253B |
| Layers | 126 |
| Experts / active | dense |
| Native context | 131,072 |
| License | NVIDIA Open Model License + Llama 3.1 terms |
| Chosen quant | Q6_K |
| Publisher size | 207.88 decimal GB |
| Conservative weight plan | 195 GiB |
| Artifact effective bits/parameter | 6.573 |

## Selection rationale

The strongest dense candidate inside the band at Q6, but all 253B parameters participate and decode compute will be high.

## Runtime support

- **llama.cpp:** Yes — Deci implementation supports per-layer attention/FFN topology arrays and 253B type.
- **ROCmFPX:** Yes — Deci implementation present.
- **Runtime risk:** Medium: custom NAS topology means the exact GGUF metadata and current Deci implementation must match.

## Tokenizer and chat template

- **Tokenizer:** Llama 3 tokenizer embedded in GGUF.
- **Template:** Use the NVIDIA-provided template/system instructions; reasoning mode is controlled by the documented system prompt.

## KV cache

**Formula status:** conservative-upper-bound; actual lower because some NAS blocks skip attention

| Context tokens | Q8_0 exact GiB | Q8 planning GiB |
|---|---|---|
| 32,768 | 8.367 | 8.50 |
| 65,536 | 16.734 | 16.75 |
| 131,072 | 33.469 | 33.50 |

## Expert activation and buffers

All parameters participate in every token; there is no MoE activation reduction.

Graph/work buffers are not predicted from parameter count. Use the package reserve profiles, then replace them with measured startup logs.

## Quality evidence

NVIDIA model card provides reasoning, chat, RAG, and tool-use evaluation; no selected-quant evaluation was located.

**Selected-quant assessment:** Moderate-low: Q6_K gives high weight fidelity.

## Download provenance

- Upstream: `nvidia/Llama-3_1-Nemotron-Ultra-253B-v1`
- Quant repo: `bartowski/nvidia_Llama-3_1-Nemotron-Ultra-253B-v1-GGUF`
- Revision: `9195f67`
- Path: `nvidia_Llama-3_1-Nemotron-Ultra-253B-v1-Q6_K`
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

- [nemotron_upstream](https://huggingface.co/nvidia/Llama-3_1-Nemotron-Ultra-253B-v1)
- [nemotron_config](https://huggingface.co/nvidia/Llama-3_1-Nemotron-Ultra-253B-v1/blob/main/config.json)
- [nemotron_quant](https://huggingface.co/bartowski/nvidia_Llama-3_1-Nemotron-Ultra-253B-v1-GGUF/tree/9195f67/nvidia_Llama-3_1-Nemotron-Ultra-253B-v1-Q6_K)
- [llama_deci](https://github.com/ggml-org/llama.cpp/blob/6bdd77f13cf11b264b4231d320afc404f48d576e/src/models/deci.cpp)
