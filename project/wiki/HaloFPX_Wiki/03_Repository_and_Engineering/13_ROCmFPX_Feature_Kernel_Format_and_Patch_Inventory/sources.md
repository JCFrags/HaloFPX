---
section_id: "13"
title: "ROCmFPX Sources"
status: "verified"
last_verified: "2026-08-12"
applies_to:
  repositories:
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "JCFrags/HaloFPX@4a156395db62604cf37e27e6459e3ee0e3949c48"
  software_versions: []
  hardware_revisions: []
related_sections: ["02", "11", "15", "16"]
---

# Sources

All repository links are pinned. Internet access date: 2026-07-16. Source count: **15**.

## S13-L05 — Current live target deployment authority

- Canonical source: [`../../../../../docs/halofpx/evidence/2026-08-12-strix-halo-live-authority/README.md`](../../../../../docs/halofpx/evidence/2026-08-12-strix-halo-live-authority/README.md).
- Capture: normalized read-only observations from both targets, 2026-08-12.
- Supports: current target OS/package tuple, node roles, services, executable hashes, coordinator source self-report, active conventional UD-Q6 model class, and volatile storage observations.
- Limitations: the RPC worker has no independent source-version report; this inventory does not qualify HaloFPX/ROCmFPX correctness, performance, or persistent cache behavior.

## S13-L04 — Current HaloFPX format/backend source audit

- Canonical source: repository source at `JCFrags/HaloFPX@4a156395db62604cf37e27e6459e3ee0e3949c48`.
- Capture: read-only exact-source audit, 2026-08-12.
- Supports: current tensor/file-type registration, block layouts, code ranges,
  quantizer and common application cache parsing, CPU/CUDA/HIP/Vulkan operator
  presence, Q2 omissions, and Q6 Vulkan device expansion.
- Limitations: static source inspection; it does not prove a model converts, loads, produces correct output, or runs faster on either target. Public/internal Q2 file-type numeric authority requires a separate focused review.

## S13-L01 — Live deployed comparison baseline

- Canonical source: [`../../../../sources/measurements/2026-07-17-strix-halo-live-inventory/README.md`](../../../../sources/measurements/2026-07-17-strix-halo-live-inventory/README.md)
- Revision/capture: both nodes, 2026-07-17; deployed `charlie12345/rocmfp4-llama@4860505ee322091f0f61eba77d6ad49be88cf4ea`.
- Supports: historical predecessor checkout, executable hashes, node roles, process configuration, artifact size, and service readiness on 2026-07-17.
- Limitations: different repository from ROCmFPX; no build reproduction, output correctness, or performance experiment was performed in this capture.

## S13-L02 — Matched two-node build/reference qualification

- Canonical source: [`../../../../experiments/2026-07-17-open-pin-01-build-qualification/RESULTS.md`](../../../../experiments/2026-07-17-open-pin-01-build-qualification/RESULTS.md)
- Revision/capture: control `a5605a7`, candidate `61f2f2d`, nimo-1 and nimo-2, 2026-07-17.
- Supports: offline source restoration, exact build times, cross-host artifact identity, CPU references, quant regression, TurboQuant unit results, and complete ROCm0 attention-matrix summaries.
- Limitations: build/reference lane only; no pin selection, model conversion, long context, MTP, RPC, Vulkan parity, quality, performance, persistence, or release proof.

## S13-L03 — Matched small-model ROCm0 runtime smoke

- Canonical source: [`../../../../experiments/2026-07-17-open-pin-01-runtime-smoke/RESULTS.md`](../../../../experiments/2026-07-17-open-pin-01-runtime-smoke/RESULTS.md)
- Revision/capture: same two revisions and nodes; Qwen3-4B-Q8_0 SHA-256 `8c2f07f...473300`; 2026-07-17.
- Supports: ROCm0 model load, F16/Turbo4 K/V-cache request completion, cross-node/commit output reproducibility, cache-mode divergence, telemetry, and clean teardown.
- Limitations: one short prompt and one seed; not a quality or performance qualification; SSD prompt-cache behavior was not exercised.

## S13-01 — ROCmFPX repository head and history

- Publisher: `charlie12345/ROCmFPX`
- URL: <https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394>
- Revision/date: `a5605a72768c6562241b248e268e33dc92787394`, commit 2026-07-16
- Supports: exact inventory baseline, history root/snapshot, feature tree.
- Limits/conflicts: repository is a disconnected snapshot lineage; commit presence does not prove runtime behavior.

## S13-02 — ROCmFPX family reference implementation

- Publisher: `charlie12345/ROCmFPX`
- URL: <https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx>
- Revision: `a5605a7`
- Supports: Q2/Q3/Q6/Q8 layouts, scale validation, quant/dequant, frozen Q2 reference/tests.
- Limits: reference code and unit fixtures do not establish model quality or GPU parity.

## S13-03 — ROCmFP4 implementation

- Publisher: `charlie12345/ROCmFPX`
- URL: <https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfp4>
- Revision: `a5605a7`
- Supports: Q4 dual/FAST block layouts, reference quant/dequant, vector dots, HIP helpers.
- Limits: “AMD-tuned” is repository scope, not a universal performance result.

## S13-04 — Type IDs, presets, and quantizer routing

- Publisher: `charlie12345/ROCmFPX`
- URLs: <https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/include/ggml.h>, <https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/include/llama.h>, <https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/quantize/quantize.cpp>, <https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/src/llama-quant.cpp>
- Revision: `a5605a7`
- Supports: numeric IDs, preset names, mixed tensor routing, pure/even behavior, requant rules.
- Limits: approximate BPW strings are descriptive, not per-model guarantees.

## S13-05 — CPU, HIP, and Vulkan backend wiring

- Publisher: `charlie12345/ROCmFPX`
- URLs: <https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cpu>, <https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cuda>, <https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-vulkan>
- Revision: `a5605a7`
- Supports: type traits, copy/rows, MMVQ/MMQ, Vulkan shaders/dispatch; absence of Q2 Vulkan symbols.
- Limits: shared `ggml-cuda` code is HIP-conditioned; static wiring is not runtime qualification.

## S13-06 — TurboQuant integration

- Publisher: ROCmFPX contributor integration
- URL: <https://github.com/charlie12345/ROCmFPX/commit/d859c9e67b0ba6cae4856be1a096ee368f746782>
- Revision/date: `d859c9e67b0ba6cae4856be1a096ee368f746782`, 2026-06-20; present in `a5605a7`
- Supports: Turbo3/4 cache origin, CPU/HIP/Vulkan scope, contributor attribution.
- Limits: does not validate Halo workloads or TurboQuant+ Python research claims.

## S13-07 — MTP and capability detection

- Publisher: `charlie12345/ROCmFPX`
- URLs: <https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/common/speculative.cpp>, <https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/rocmfpx-model-capabilities.py>, <https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/check-rocmfpx-model-capabilities.sh>
- Revision: `a5605a7`
- Supports: `draft-mtp`, marker detection, generated serving profiles, synthetic detector tests.
- Limits: detector does not parse GGUF; synthetic markers do not establish real-model accuracy.

## S13-08 — Serving wrappers, SSD prompt cache, and regression scripts

- Publisher: `charlie12345/ROCmFPX`
- URLs: <https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394/scripts>, <https://github.com/charlie12345/ROCmFPX/commit/c81c7c92233b6370b4eb7087398779a8dcb234a4>
- Revisions: `a5605a7`; SSD cache introduction `c81c7c92` (2026-07-13)
- Supports: build/quant/server/benchmark/test surface, asymmetric cache wrapper, disk cache patch.
- Limits: scripts encode local paths/defaults and often need uncommitted large model artifacts.

## S13-09 — Fork handoff, integration, and attribution documents

- Publisher: `charlie12345/ROCmFPX`
- URLs: <https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/docs/ROCmFPX-HANDOFF.md>, <https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/docs/ROCmFPX-EXPERIMENT.md>, <https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/docs/UPSTREAM-ATTRIBUTION.md>, <https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/docs/ROCmFPX-UPSTREAM-CREDITS.md>
- Revision: `a5605a7`; documents revised through 2026-07-16
- Supports: intended contract, repository-reported validation, historical `b9438/22cadc194` base, direct/manual port attribution.
- Limits/conflicts: prose lags code (notably Q2 omission in older handoff sections); reported measurements are not independently reproduced.

## S13-10 — Contemporaneous upstream llama.cpp head

- Publisher: `ggml-org/llama.cpp`
- URL: <https://github.com/ggml-org/llama.cpp/tree/788e07dc91d266ad3162a1ce9037665656269689>
- Revision: `788e07dc91d266ad3162a1ce9037665656269689`, observed 2026-07-16 America/Los_Angeles
- Supports: comparison head and evidence that upstream continues changing quant/backend surfaces.
- Limits: not asserted as a suitable integration base; selection belongs to sections 11/15.
