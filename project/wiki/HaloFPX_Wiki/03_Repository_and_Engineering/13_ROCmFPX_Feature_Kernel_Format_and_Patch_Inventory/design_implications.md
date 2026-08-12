---
section_id: "13"
title: "ROCmFPX Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"]
  software_versions: []
  hardware_revisions: ["AMD Strix Halo / gfx1151 (target only)"]
related_sections: ["15", "23", "30", "33", "36", "37", "66", "74", "78"]
---

# Design implications

<a id="s13-adoption"></a>
## Adoption posture

**[RECOMMENDATION]** Freeze `a5605a7` as a research candidate, not a production baseline. First reconstruct its relationship to an exact upstream snapshot and turn feature clusters into a reviewable patch series (section 15). The orphan history makes “merge upstream” an unsafe description of the work.

**[RECOMMENDATION]** Use a staged admissibility ladder:

1. `Q4_0_ROCMFP4` / `FAST` for first end-to-end validation because both backends and the most scripts exist.
2. Q6/Q8 as quality/reference lanes.
3. Q3 only after agent/tool/quality gates.
4. Q2 only in an isolated experimental lane until Vulkan exists (if required), parity passes, and model-quality loss is characterized.

This is a risk order, not a performance claim.

<a id="s13-identity"></a>
## Format identity and cache fingerprints

**[VERIFIED]** Custom numeric type IDs and block serialization are fork-local [S13-02, S13-03, S13-04]. **[RECOMMENDATION]** HaloFPX model/cache compatibility fingerprints must include repository commit, GGUF type number and name, block-layout revision, quant preset/routing manifest, converter commit, model hash, and backend build identity. Never infer compatibility from a filename alone.

**[INFERENCE]** Upstream can allocate the same enum range or change serialization assumptions. A patch-stack rebase must fail closed on collisions; silent reinterpretation could produce plausible but wrong tensors.

<a id="s13-backend-selection"></a>
## Backend selection

**[RECOMMENDATION]** Preserve HIP and Vulkan as independently qualified backends. Static kernel coverage does not justify a universal default. Select per model, operation shape, context, and software tuple using section 74’s matched matrix.

**[RECOMMENDATION]** Backend fallback must be explicit in logs and experiment data. A test that completes after silently moving unsupported work to CPU does not prove GPU kernel coverage.

**[OPEN]** Q2 cannot be a cross-backend artifact at this head because Vulkan support is absent. Decide whether to implement it, constrain Q2 to HIP/CPU, or exclude it.

**[OPEN current-source reconciliation]** The shared backend source also builds
for CUDA, so the current implementation boundary is CPU plus selected
CUDA/HIP operations, with no Vulkan. Decide whether to implement the missing
Vulkan and CUDA/HIP operation surfaces, keep that narrower scope, or exclude
Q2 from promoted artifacts.

<a id="s13-mtp-distributed"></a>
## MTP and distributed execution

**[INFERENCE]** MTP changes graph state, KV state, acceptance accounting, and checkpoint restore. In a two-rank design, the plan must state which rank owns the draft head, how accepted-token state is committed, how failure rolls back, and how single-node fallback reconstructs state. Existing single-process wrappers do not answer those questions.

**[RECOMMENDATION]** Keep speculative parameters in a versioned per-model profile only after greedy equivalence, acceptance accounting, and fault-recovery tests. Do not reuse hard-coded Qwen/Strix profiles for other architectures.

<a id="s13-cache-design"></a>
## TurboQuant and persistence

**[INFERENCE]** Asymmetric K/V (`q8_0` K, `turbo4` V) is a sensible candidate because the fork explicitly treats K as more quality-sensitive, but it remains an unverified local policy [S13-06, S13-08].

**[RECOMMENDATION]** Persisted cache identity must encode K and V type per layer, boundary overrides, draft-cache types, graph/MTP identity, and backend layout. Corrupt or mismatched cache material must cause a miss/recompute, never acceptance.

**[RECOMMENDATION]** Treat the fork’s SSD prompt cache as a separate candidate feature. Evaluate it against HaloKV’s durability, atomicity, privacy, rank ownership, and invalidation contract before reuse; do not layer two independent cache authorities by default.

<a id="s13-serving"></a>
## Capability detection and serving

**[RECOMMENDATION]** Replace marker scanning as the authority with a real GGUF metadata/tensor parser. The current script may remain a fast hint only if output carries `heuristic: true`, the probe limit, evidence markers, and a safe fallback profile.

**[RECOMMENDATION]** Serving wrappers should generate a manifest and command without launching by default, then record resolved model hash, backend devices, cache types, MTP settings, context/batch sizes, and source commit. This makes comparisons reproducible and prevents filename-based policy from silently becoming configuration.

<a id="s13-upstream-strategy"></a>
## Upstream strategy

**[RECOMMENDATION]** Separate patches into: (A) serialized formats/type registration, (B) CPU references, (C) HIP, (D) Vulkan, (E) quantizer/presets, (F) model conversion/MTP, (G) server/cache, and (H) scripts/docs. Rebase and validate in that order. It localizes failures and permits omission of server/cache experiments without losing weight kernels.

**[OPEN]** The live upstream head already contains its own `Q2_0` evolution and thousands of commits outside the fork’s disconnected history. Section 15 must choose a new upstream base and perform semantic, not line-count, comparison.
