# Strix Halo optimizations

The parent/component pair includes gfx1151 detection, ROCm/UMA launch settings, HIP kernel experiments, Vulkan memory/graph work, and an engineering benchmark ledger. ROCmFPX already has newer Strix-specific quant, MTP, and backend work; port only missing narrow changes after matched correctness and performance tests.

| Decision | Count |
|---|---:|
| RETAIN | 6 |
| REDESIGN | 5 |
| REJECT | 0 |

<a id="f-094"></a>
### F-094 — gfx1151/RDNA3.5 detection

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M3` · **Confidence:** `High`

**Observed behavior.** Recognizes Strix Halo gfx1151 in deployment and HIP architecture classification.

**Implementation.** `scripts/detect-gpu.sh`, `ggml/src/ggml-cuda/vendors/hip.h`

**Dependencies.** AMD PCI/HSA identification; HIP compiler

**License.** Mixed: GPL parent detection, MIT backend

**ROCmFPX overlap.** ROCmFPX explicitly targets Strix Halo and already carries RDNA3.5 support.

**Porting rationale.** Use target-native detection; reconcile only missing device IDs or capability probes.

**Risks / caveats.** Hard-coded HSA overrides can hide real compiler/runtime incompatibility.

**Evidence.** [E-011](20-Evidence-Index.md#e-011), [E-111](20-Evidence-Index.md#e-111)

<a id="f-095"></a>
### F-095 — HSA and unified-memory launch environment

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Exports an HSA gfx override and unified-memory settings for Strix Halo ROCm execution.

**Implementation.** `scripts/detect-gpu.sh`, `llama-run.sh`, `scripts/rebuild.sh`

**Dependencies.** ROCm/HIP runtime; UMA APU

**License.** GPL-3.0 parent

**ROCmFPX overlap.** ROCmFPX quick start already documents HSA_OVERRIDE_GFX_VERSION and unified memory.

**Porting rationale.** Keep target-documented environment and add version-gated validation rather than copying shell code.

**Risks / caveats.** Overrides vary by ROCm release.

**Evidence.** [E-117](20-Evidence-Index.md#e-117), [E-119](20-Evidence-Index.md#e-119)

<a id="f-096"></a>
### F-096 — Large-context in-memory Halo profile

**Decision:** `REDESIGN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Chooses large contexts and may disable SSD checkpointing for selected dense/MoE Halo profiles because UMA can hold state in memory.

**Implementation.** `llama-run.sh`

**Dependencies.** hardware tier; model class; large UMA

**License.** GPL-3.0

**ROCmFPX overlap.** ROCmFPX targets the same APU and often runs large MoE models.

**Porting rationale.** Convert to a declarative memory estimator based on model/KV/context/parallelism, retaining SSD as an explicit resilience option.

**Risks / caveats.** Static profiles can exceed memory or disable useful restart persistence.

**Evidence.** [E-007](20-Evidence-Index.md#e-007), [E-118](20-Evidence-Index.md#e-118)

<a id="f-097"></a>
### F-097 — RDNA3.5 MMVQ launch tuning

**Decision:** `REDESIGN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Uses RDNA3.5-specific warp choices for quantized matrix-vector generation paths.

**Implementation.** `ggml/src/ggml-cuda/mmvq.cu`, `STRIX_HALO_NOTES.md`

**Dependencies.** HIP/ROCm; quantized kernels; gfx1151

**License.** MIT

**ROCmFPX overlap.** ROCmFPX has newer custom ROCmFPX kernel formats and target-specific tuning.

**Porting rationale.** Benchmark-port individual launch heuristics per ROCmFPX type; do not wholesale replace target kernels.

**Risks / caveats.** Tuning for Q/K/IQ types may not transfer to ROCmFPX formats.

**Evidence.** [E-110](20-Evidence-Index.md#e-110), [E-112](20-Evidence-Index.md#e-112)

<a id="f-098"></a>
### F-098 — RDNA3.5 gated-delta-net tuning

**Decision:** `REDESIGN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Selects a larger warp count for a narrow recurrent gated-delta-net shape on RDNA3.5.

**Implementation.** `ggml/src/ggml-cuda/gated_delta_net.cu`, `STRIX_HALO_NOTES.md`

**Dependencies.** HIP recurrent kernel; specific tensor shape

**License.** MIT

**ROCmFPX overlap.** ROCmFPX includes MTP and may host recurrent/hybrid models.

**Porting rationale.** Cherry-pick concept only after target-side correctness and end-to-end model benchmarks.

**Risks / caveats.** Shape-specific tuning can regress other models.

**Evidence.** [E-110](20-Evidence-Index.md#e-110), [E-113](20-Evidence-Index.md#e-113)

<a id="f-099"></a>
### F-099 — Vulkan AMD architecture routing

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M3` · **Confidence:** `High`

**Observed behavior.** Classifies AMD Vulkan devices using subgroup and integer-dot capabilities to select architecture-specific paths.

**Implementation.** `ggml/src/ggml-vulkan/ggml-vulkan.cpp`

**Dependencies.** Vulkan feature/property queries

**License.** MIT

**ROCmFPX overlap.** ROCmFPX recommends Vulkan for several tested Strix decode workloads and has newer Vulkan kernels.

**Porting rationale.** Keep target-native implementation; reconcile any missing capability logic narrowly.

**Risks / caveats.** Capability inference can conflate RDNA generations.

**Evidence.** [E-114](20-Evidence-Index.md#e-114)

<a id="f-100"></a>
### F-100 — Integrated Vulkan memory accounting

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M3` · **Confidence:** `High`

**Observed behavior.** Counts all memory heaps for integrated GPUs rather than only device-local heaps when reporting capacity.

**Implementation.** `ggml/src/ggml-vulkan/ggml-vulkan.cpp`

**Dependencies.** VK_EXT_memory_budget when available; integrated GPU detection

**License.** MIT

**ROCmFPX overlap.** Directly relevant to Strix Halo UMA and likely present in the newer target.

**Porting rationale.** Retain target implementation and add pressure-aware reservations rather than treating all UMA as available.

**Risks / caveats.** Reported budget is not equivalent to safe allocatable memory.

**Evidence.** [E-115](20-Evidence-Index.md#e-115)

<a id="f-101"></a>
### F-101 — Vulkan graph reordering and fusion preservation

**Decision:** `REDESIGN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Reorders graph nodes around MoE, normalization, recurrent, and view patterns to improve execution while preserving dependencies.

**Implementation.** `ggml/src/ggml-vulkan/ggml-vulkan.cpp`

**Dependencies.** ggml graph optimizer; Vulkan pipelines

**License.** MIT

**ROCmFPX overlap.** ROCmFPX has a newer Vulkan backend with ROCmFPX-specific shaders.

**Porting rationale.** Diff at symbol level and port only missing optimizer cases with backend-op correctness tests.

**Risks / caveats.** Large backend files have high merge-conflict and regression risk.

**Evidence.** [E-114](20-Evidence-Index.md#e-114), [E-116](20-Evidence-Index.md#e-116)

<a id="f-102"></a>
### F-102 — Experiment ledger with accepted/rejected variants

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Records exact environment, canary models, correctness totals, benchmark commands, and rejected kernel experiments.

**Implementation.** `STRIX_HALO_NOTES.md`

**Dependencies.** local benchmark harness; test-backend-ops; llama-bench

**License.** MIT documentation in component repo

**ROCmFPX overlap.** ROCmFPX already publishes Strix/MTP benchmarks and experimental status.

**Porting rationale.** Retain the engineering practice: every kernel change needs correctness gates, matched baselines, and a rejection record.

**Risks / caveats.** Notes mix clean commits and dirty local builds; provenance must be normalized.

**Evidence.** [E-110](20-Evidence-Index.md#e-110)

<a id="f-103"></a>
### F-103 — Vulkan-first decode recommendation

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Local evidence favors Vulkan over HIP for selected Strix Halo decode workloads, while both backends are supported.

**Implementation.** `STRIX_HALO_NOTES.md`, `ROCmFPX/README.md`

**Dependencies.** Vulkan; HIP/ROCm; model-specific benchmark

**License.** Mixed MIT repositories

**ROCmFPX overlap.** ROCmFPX independently reports Vulkan as the tested decode starting point on Strix Halo.

**Porting rationale.** Keep backend choice workload-driven and benchmark both for each format/model.

**Risks / caveats.** Results are hardware, driver, model, context, and quant dependent.

**Evidence.** [E-110](20-Evidence-Index.md#e-110), [E-202](20-Evidence-Index.md#e-202), [E-212](20-Evidence-Index.md#e-212)

<a id="f-104"></a>
### F-104 — MoE-focused tuning priority

**Decision:** `REDESIGN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Prioritizes MoE throughput and uses Qwen/GPT-OSS canaries while accepting some dense-model tradeoffs when correctness is intact.

**Implementation.** `STRIX_HALO_NOTES.md`, `llama-run.sh`

**Dependencies.** MoE models; benchmark matrix; profile resolver

**License.** Mixed MIT/GPL

**ROCmFPX overlap.** ROCmFPX has strong MoE and agent-format focus.

**Porting rationale.** Separate kernel/profile presets by workload rather than one global Strix default.

**Risks / caveats.** A MoE-biased default can regress dense models.

**Evidence.** [E-110](20-Evidence-Index.md#e-110), [E-118](20-Evidence-Index.md#e-118)


