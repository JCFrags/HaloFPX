---
section_id: "13"
title: "ROCmFPX Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-08-12"
applies_to:
  repositories:
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "JCFrags/HaloFPX@4a156395db62604cf37e27e6459e3ee0e3949c48"
  software_versions: []
  hardware_revisions: ["AMD Strix Halo / gfx1151 (target only)"]
related_sections: ["11", "12", "15", "30", "31", "33", "36", "37"]
---

# Facts and constraints

<a id="s13-baseline"></a>
## Baseline and maturity legend

**[VERIFIED]** ROCmFPX head is `a5605a7`; llama.cpp head observed in the same research window is `788e07d` [S13-01, S13-10]. **[VERIFIED]** ROCmFPX starts from an orphan “Initial commit” (`ebee2649`) followed by a source snapshot (`4e8f35ae`), so Git finds no merge base with upstream. A documented older integration point is llama.cpp `b9438` / `22cadc194`, but that document no longer proves the ancestry of current head [S13-01, S13-09].

Maturity below means: **wired** = code dispatch exists; **scripted** = a test/runner exists; **fork-reported** = the repository says it passed; **Halo-unvalidated** = not reproduced here.

<a id="s13-formats"></a>
## Weight formats and serialization

| GGML/GGUF type | Layout at pinned head | Nominal BPW | CPU ref | CUDA/HIP | Vulkan | Maturity / limits |
|---|---|---:|---|---|---|---|
| `Q2_0_ROCMFPX` | 32 weights; 8 packed bytes + two UE4M3 scales; S40 mapping `-4,-1,+1,+4` | 2.50 | wired plus frozen binary64 reference | dequantization, `GET_ROWS`, MMVQ, and MMQ wired; generic same-type contiguous copy remains; no conversion/noncontiguous `CPY` or `SET_ROWS` | **absent from Vulkan dispatch/shaders** | very experimental; absent from common application cache CLI allowlist, wrapper, and agent presets; quality unproven |
| `Q3_0_ROCMFPX` | 32; 12 packed bytes + two UE4M3 scales; codes `0, ±1, ±2, ±4` | 3.50 | wired | wired | dequant/copy/rows/DMMV-MMV/MMQ wiring | experimental; lowest-bit documented coherency risk |
| `Q4_0_ROCMFP4` | 32; 16 nibbles + two UE4M3 scales | 4.50 | ref + vector dot | wired | wired | fork’s promoted baseline, but custom/non-upstream type |
| `Q4_0_ROCMFP4_FAST` | 32; 16 nibbles + one UE4M3 scale | 4.25 | ref + vector dot | wired | wired | speed layout; separate type prevents aliasing dual-scale files |
| `Q6_0_ROCMFPX` | 32; 24 packed bytes + two UE4M3 scales; signed range `[-32,31]` | 6.50 | wired | wired | wired; 26-byte GGUF blocks expand to 34 device bytes | experimental; endpoint semantics have multiple fix commits |
| `Q8_0_ROCMFPX` | 32 int8 values + one UE4M3 scale; clamp `[-127,127]` | 8.25 | wired | wired | wired | high-quality family reference, still custom/experimental |

**[VERIFIED]** Scale validation rejects sign-bit scale bytes and `0x7f`; layouts use finite unsigned UE4M3 scales [S13-02, S13-03]. **[VERIFIED]** Custom GGML type IDs occupy `100–107`, and custom llama file-type IDs occupy `100–119` with gaps [S13-02, S13-04]. These numeric assignments are compatibility and upstream-merge hazards.

<a id="s13-presets"></a>
## Quantization presets

**[VERIFIED]** The quantizer exposes the following custom names [S13-04]:

| Base tensor type | Presets / routing variants |
|---|---|
| ROCmFP2 | `Q2_0_ROCMFPX` |
| ROCmFP3 | `Q3_0_ROCMFPX`, `Q3_0_ROCMFPX_AGENT` |
| ROCmFP4 dual-scale | `Q4_0_ROCMFP4`, `_EVEN`, `_LEAN`, `_COHERENT` |
| ROCmFP4 single-scale | `Q4_0_ROCMFP4_FAST`, `_FAST_EVEN`, `_FAST_COHERENT`, `_STRIX`, `_STRIX_LEAN` |
| ROCmFP6 | `Q6_0_ROCMFPX`, `_LEAN`, `_AGENT`, `_AGENT_LEAN` |
| ROCmFP8 | `Q8_0_ROCMFPX`, `_AGENT` |

**[VERIFIED]** `_EVEN` implies `--pure`. Agent/coherent/lean/Strix names are tensor-routing recipes: they mix the underlying custom type with higher-precision embeddings, attention projections, or selected FFN tensors; they are not new block formats [S13-04]. **[INFERENCE]** Approximate BPW descriptions for mixed presets cannot be treated as file-size guarantees because tensor mix depends on architecture and quantizer routing.

<a id="s13-kernels"></a>
## Kernel and operation inventory

The first claim below is the pinned-fork family summary from S13-02/03/05.
The dated reconciliation that follows narrows its generalized backend wording
for Q2 without replacing the preserved source-scoped claim.

**[VERIFIED]** CPU registers quantize/dequantize/type-trait paths and custom vector dots. HIP uses the shared `ggml-cuda` backend with custom decode, copy, `GET_ROWS`, MMVQ, MMQ, and type traits. Vulkan has custom dequant shaders for ROCmFP3/4/4-fast/6/8 and Turbo3/4 plus copy, row, matvec/matmul dispatch [S13-02, S13-03, S13-05].

**[VERIFIED]** At HaloFPX `4a156395` on 2026-08-12, Q2 has shared CUDA/HIP
dequantization, `GET_ROWS`, MMVQ, and MMQ wiring. Generic same-type contiguous
device copy remains available, but conversion/noncontiguous `CPY`, `SET_ROWS`,
and Vulkan are absent. The other listed weight families retain their
CPU/CUDA/HIP/Vulkan paths [S13-L04].

| Operation surface | Q3/Q4/Q6/Q8 | Q2 | Turbo3/4 |
|---|---|---|---|
| CPU quant/dequant | yes | yes + frozen ref | yes |
| CUDA/HIP copy / dequant / rows | wired | dequant + `GET_ROWS`; generic same-type contiguous copy only; no conversion/noncontiguous `CPY` or `SET_ROWS` | wired |
| CUDA/HIP MMVQ/MMQ | wired | wired | cache attention/vector paths |
| Vulkan copy / dequant / rows | wired | no Q2 symbols found | wired |
| Vulkan matvec/matmul | wired | no Q2 symbols found | cache/FA-related use |
| `SET_ROWS` | backend-dependent custom wiring; must test | open | Vulkan `set_rows_turbo` exists |

**[OPEN]** Static dispatch is not proof of numerical parity, graph placement, or performance on gfx1151. Fork documents a generic HIP `F16 x F16 -> F32 MUL_MAT` failure that it marks unsupported for fallback; reproduce against the chosen build [S13-09].

<a id="s13-turboquant"></a>
## TurboQuant K/V support

**[VERIFIED]** `GGML_TYPE_TURBO3_0` (3.5 bpw) and `GGML_TYPE_TURBO4_0` (4.5 bpw) were integrated at local commit `d859c9e6`; CPU reference, HIP, Vulkan, and cache type parsing are present [S13-06]. They are K/V cache types, never GGUF weight presets.

**[VERIFIED]** The fork includes an asymmetric wrapper (`q8_0` K, `turbo4` V, including draft cache) and optional boundary-layer protection through environment variables [S13-08]. **[RECOMMENDATION]** Treat those values as candidate policy only; do not promote until long-context perplexity/task-quality and MTP state tests pass.

<a id="s13-mtp"></a>
## MTP and model support

**[VERIFIED]** The tree carries `draft-mtp`, request-level speculative overrides, Qwen/Gemma/Step adaptations, multi-head Step MTP, M-RoPE draft-batch fixes, on-device checkpoint fixes, HY V3 conversion/runtime support, and strict HY3 greedy verification [S13-07, S13-09]. MTP is a runtime/model-graph feature, not a quant block feature.

**[VERIFIED]** `rocmfpx-model-capabilities.py` scans a bounded prefix of raw GGUF bytes and the filename for marker substrings, then emits a recommended serving profile [S13-07]. **[INFERENCE]** This is a convenience heuristic, not authoritative GGUF parsing: it can miss metadata beyond the probe, accept misleading filenames or byte sequences, and hard-codes machine-specific profiles. Its synthetic marker test proves internal routing, not real-model detection accuracy.

<a id="s13-tooling"></a>
## Conversion, serving, tests, and benchmarks

| Surface | Inventory | Limit |
|---|---|---|
| Conversion | inherited HF-to-GGUF scripts; modular DeepSeek/Diffusion helpers; HY V3 split MTP; Step metadata checks; NVFP4→ROCmFP4 requant path | model-specific; split metadata and tokenizer invariants require validation |
| Quant wrappers | `quantize-rocmfpx-agent.sh`, from-K-quant smoke path, ranked policy generator | best-quality input is BF16/F16; requant is lossy |
| Serving | upstream `llama-server`; ROCmFPX MTP/TurboQuant wrappers; production preflight; server SSD prompt cache patch | wrapper defaults are hardware/model-specific; disk cache is not HaloKV |
| Tests | C reference tests; backend ops; quant/copy/FA/runtime scripts; MTP/EAGLE3/model/tool/JSON/long-context smokes | many scripts require local models, Linux, ROCm/Vulkan and do not run in generic CI |
| Benchmarks | `llama-bench`, model comparison scripts, decode/prefill/profile/sweep scripts, historical tables in docs | repository tables lack a complete committed raw-artifact/environment bundle for HaloFPX reuse |

**[VERIFIED]** `c81c7c92` adds an SSD prompt cache for MTP; later commits add portable sync, UTF-8 paths, and disk-failure tests [S13-08]. **[INFERENCE]** It overlaps the future HaloKV/server cache surface and is a likely semantic conflict, but is not evidence of crash-safe rank-local persistent KV pages.

<a id="s13-patches"></a>
## Patch inventory and conflict heat

| Patch cluster / key commit | Scope | Heat |
|---|---|---|
| `4e8f35ae` source snapshot | full llama.cpp-derived tree | critical: orphan history blocks normal merge-base workflows |
| `864f263c`, `d2aa5f7e` | imatrix scale search and quant speed | high: `ggml` quant core |
| `6af462e1`, `f0065a04`, `22686453` | Vulkan FP3/FP6, CPU dot, HIP FA, conversion | high: active backend files/shaders |
| `d859c9e6`, `d0141e86` | Turbo3/4 CPU/HIP/Vulkan | high: enum/type/cache/FA surfaces |
| `c0eb33b9`, `b3414b78` | profiles, capability/ranked policy | medium: scripts/policy; low binary conflict |
| `e766769d`, `db09e3ed`, `e0eefaf2` | Qwen/Gemma/Step MTP and M-RoPE fixes | high: model graph/speculative core |
| `c81c7c92`, `bb7d9cb5`, `756121a5` | disk prompt cache | high: server state/durability semantics |
| `9d1090e5`, `120227d3`, `a8b5fa9` | ROCmFP2 kernels and corrections | critical/immature; no Vulkan path at head |
| `630fa5a0`, `f9614045`, `7d7b06bc` | HY3 MTP conversion/runtime/verification | high and model-specific |

**[VERIFIED]** Attribution files identify direct upstream cherry-picks and manual ports, but do not define a complete machine-readable patch stack [S13-09].

## Historical deployed comparison baseline — 2026-07-17

- **[MEASURED]** Both nodes had clean detached checkouts of `charlie12345/rocmfp4-llama@4860505ee322091f0f61eba77d6ad49be88cf4ea`; nimo-1 ran its RPC server and nimo-2 its model/API server [S13-L01].
- **[MEASURED]** The RPC executable SHA-256 was `7f7cb7f0b2217ed714e32d028c210059d78dc932caf2b1a78055d23b59b99d9a`; the coordinator `llama-server` SHA-256 was `ab9c0275289857811154e17fdffd35bb857ce20a1b0fdcf00e3c85e82de5a479` [S13-L01].
- **[MEASURED]** The deployed model file was 121,861,632,736 bytes and the server API reported 228,689,764,864 parameters; its configuration used `RPC0,ROCm0`, layer split `1,1`, two 4096-token slots, Q4_0 K/V, and `--fit off --no-mmap --no-warmup` [S13-L01].
- **[RECOMMENDATION]** Use this installation only as an operational comparison and rollback reference. The requested canonical fork must build and validate ROCmFPX separately before replacing it.
