# Llama-3.1-Tulu-3-405B

**Rank:** 7 · **Score:** 66/100 · **Disposition:** `conditional`

## Selected representation

| Field | Value |
|---|---|
| Architecture | llama |
| Class | Dense |
| Total parameters | 405B |
| Activated parameters/token | 405B |
| Layers | 126 |
| Experts / active | dense |
| Native context | 131,072 |
| License | Llama 3.1 Community License |
| Chosen quant | IQ4_XS |
| Publisher size | 216.57 decimal GB |
| Conservative weight plan | 203 GiB |
| Artifact effective bits/parameter | 4.278 |

## Selection rationale

Useful dense 405B baseline with mature support, but all 405B parameters are active and the 128K Q8 cache upper bound is large.

## Runtime support

- **llama.cpp:** Yes — mature generic Llama implementation.
- **ROCmFPX:** Yes — generic Llama implementation inherited.
- **Runtime risk:** Low architecture risk, high compute and memory-pressure risk.

## Tokenizer and chat template

- **Tokenizer:** Llama 3 tokenizer embedded in GGUF.
- **Template:** Tulu template: <|system|>, <|user|>, <|assistant|>; prefer embedded Jinja.

## KV cache

**Formula status:** verified-standard-GQA

| Context tokens | Q8_0 exact GiB | Q8 planning GiB |
|---|---|---|
| 32,768 | 8.367 | 8.50 |
| 65,536 | 16.734 | 16.75 |
| 131,072 | 33.469 | 33.50 |

## Expert activation and buffers

All parameters participate in every token; there is no MoE activation reduction.

Graph/work buffers are not predicted from parameter count. Use the package reserve profiles, then replace them with measured startup logs.

## Quality evidence

Upstream Tulu card supplies instruction-following and preference-training evidence; quant card provides qualitative guidance only.

**Selected-quant assessment:** Moderate-high: IQ4_XS is an aggressive dense-model quant; card claims similar quality to Q4_K_S but lacks candidate-specific independent evaluation.

## Download provenance

- Upstream: `allenai/Llama-3.1-Tulu-3-405B`
- Quant repo: `bartowski/Llama-3.1-Tulu-3-405B-GGUF`
- Revision: `main`
- Path: `Llama-3.1-Tulu-3-405B-IQ4_XS`
- Shard count: `discover with refresh script`

Known exact LFS pointers:

- No exact shard hash captured in this snapshot; use the revision/path manifest and refresh script.

## Acceptance gates

- Refresh and verify every shard hash.
- Load with the pinned runtime and exact embedded Jinja template.
- Run the production quality suite against a higher-bit reference.
- Capture per-device/node weights, KV, graph, and compute buffers.
- Benchmark the intended backend/topology; do not use parameter-count throughput extrapolation.

## Sources

- [tulu_upstream](https://huggingface.co/allenai/Llama-3.1-Tulu-3-405B)
- [tulu_quant](https://huggingface.co/bartowski/Llama-3.1-Tulu-3-405B-GGUF)
- [llama_llama](https://github.com/ggml-org/llama.cpp/blob/6bdd77f13cf11b264b4231d320afc404f48d576e/src/models/llama.cpp)
