# GLM-4.7

**Rank:** 4 · **Score:** 83/100 · **Disposition:** `shortlist`

## Selected representation

| Field | Value |
|---|---|
| Architecture | glm4moe |
| Class | MoE |
| Total parameters | 358B |
| Activated parameters/token | 32B |
| Layers | 92 |
| Experts / active | 160 / 8 |
| Native context | 202,752 |
| License | MIT |
| Chosen quant | Q4_K_M |
| Publisher size | 218.52 decimal GB |
| Conservative weight plan | 205 GiB |
| Artifact effective bits/parameter | 4.883 |

## Selection rationale

Strong official coding/agent evidence and mature dual-runtime support; main drawback is Q4 weight precision and faster KV growth.

## Runtime support

- **llama.cpp:** Yes — dedicated glm4-moe implementation.
- **ROCmFPX:** Yes — glm4-moe implementation present.
- **Runtime risk:** Medium: long context and parser/template semantics deserve an exact-build acceptance test.

## Tokenizer and chat template

- **Tokenizer:** Embedded GLM tokenizer in GGUF.
- **Template:** Embedded GLM Jinja; reasoning/tool parser behavior is revision-sensitive.

## KV cache

**Formula status:** verified-from-config/current-implementation

| Context tokens | Q8_0 exact GiB | Q8 planning GiB |
|---|---|---|
| 32,768 | 6.109 | 6.25 |
| 65,536 | 12.219 | 12.25 |
| 131,072 | 24.438 | 24.50 |
| 202,752 | 37.802 | 38.00 |

## Expert activation and buffers

The router activates 8 of 160 routed experts per token plus 1 shared expert(s). All expert weights remain resident.

Graph/work buffers are not predicted from parameter count. Use the package reserve profiles, then replace them with measured startup logs.

## Quality evidence

Official card reports SWE-bench Verified 73.8, multilingual 66.7, TerminalBench 2.0 41, and HLE 42.8.

**Selected-quant assessment:** Moderate-high versus Q5/Q6/Q8 candidates because the chosen artifact is Q4_K_M.

## Download provenance

- Upstream: `zai-org/GLM-4.7`
- Quant repo: `bartowski/zai-org_GLM-4.7-GGUF`
- Revision: `75abf8a`
- Path: `zai-org_GLM-4.7-Q4_K_M`
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

- [glm_upstream](https://huggingface.co/zai-org/GLM-4.7)
- [glm_quant](https://huggingface.co/bartowski/zai-org_GLM-4.7-GGUF/tree/75abf8a/zai-org_GLM-4.7-Q4_K_M)
- [llama_glm4moe](https://github.com/ggml-org/llama.cpp/blob/6bdd77f13cf11b264b4231d320afc404f48d576e/src/models/glm4-moe.cpp)
