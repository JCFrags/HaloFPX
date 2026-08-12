---
section_id: "81"
title: "CI, Release Gate, Reproducibility, and Regression Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940", "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"]
  software_versions: ["SLSA 1.2", "Reproducible Builds definition accessed 2026-07-17"]
  hardware_revisions: ["dual-Strix-Halo target; exact revisions open"]
related_sections: ["73", "74", "75", "76", "77", "78", "79", "80"]
---

# Procedures and checks

## Pipeline construction

1. Define a machine-readable matrix keyed by runtime commit, backend, build mode, OS/toolchain, model/quant, cache format, topology/rank placement, and test tier.
2. Mark each cell required or optional and assign an evidence artifact. Reject unknown or duplicate matrix keys.
3. Reuse the pinned source's build, sanitizer, server, tokenizer-hash, backend-op, state, perplexity, benchmark, and release mechanisms; add HaloFPX wrappers only where the project contract differs.
4. Run cheap invariant checks before scarce hardware. Queue physical runners exclusively for measurement jobs and record runner health before/after.
5. Publish a signed manifest that links every gate result to its source, build, model, dataset, workload, runner, and raw artifacts.

## Performance regression procedure

1. Select the exact immutable baseline for the matrix key. A missing or stale baseline is “blocked/inconclusive,” never pass.
2. Verify matched machine state, power/cooling profile, topology, software stack, workload, model and telemetry.
3. Warm up using the declared exclusion policy; randomize/interleave candidate and baseline runs.
4. Preserve raw samples for TTFT, inter-token latency, prompt/decode throughput, tail latency, memory, power, temperature, errors, and correctness.
5. Compute candidate/baseline effect and uncertainty using the predeclared method. Apply practical thresholds learned from Section 73 pilot variance.
6. On a regression, rerun only under the documented confirmation rule; do not average away failures or silently replace the baseline.

## Release-candidate checklist

- Exact source/submodule/dependency/model/dataset hashes and clean-tree status.
- Release build plus tests for every required backend/topology/model/quant cell.
- Section 78 correctness/quality/state/protocol evidence with zero unclassified mismatches.
- Section 79 required soak, context, fairness, power, thermal, cache, link, and storage evidence.
- Section 80 fault matrix and post-recovery correctness.
- Cache/model/protocol migration and backward/forward compatibility decisions; corrupt/stale data rejects or recomputes.
- Security scanning, dependency review, licenses/notices, secret scan, SBOM, checksums, signatures, and provenance attestation.
- Two independent clean builds and hash comparison; explained variance if reproducibility is not claimed.
- Installation/cutover plan, health/readiness/API smoke, deployed artifact verification, and rehearsed rollback.
- Immutable evidence index and explicit approvers for all exceptions.

## Proposed policy defaults

[RECOMMENDATION] Target a roughly 15-minute change-gate budget by sharding, without dropping required invariants. Run physical target smoke after merge/queue, comprehensive hardware qualification nightly, stress periodically, and the full matrix for each release candidate. These are planning defaults only; runner capacity and failure-detection data must validate them.

[RECOMMENDATION] Retain release-candidate raw evidence and provenance immutably for the supported lifetime plus the rollback/audit period. Exact retention durations and storage classes remain [OPEN]. Never delete the last baseline needed to interpret a supported release.
