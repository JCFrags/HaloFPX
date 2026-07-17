---
section_id: "81"
title: "CI, Release Gate, Reproducibility, and Regression Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940", "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"]
  software_versions: ["SLSA 1.2", "Reproducible Builds definition accessed 2026-07-17"]
  hardware_revisions: ["dual-Strix-Halo target; exact revisions open"]
related_sections: ["73", "74", "75", "76", "77", "78", "79", "80"]
---

# Design implications

## Tiered matrix

| Tier | Trigger | Proposed mandatory scope |
|---|---|---|
| Change | each change/PR | formatting/schema/link checks; unit/golden tests; CPU reference; affected backend builds; sanitizer/static checks; small protocol/model smoke; license/dependency diff |
| Hardware smoke | merge or queued change | exact target build; HIP/ROCmFPX and Vulkan smoke; one/two-node load, decode, state/cache and distributed failure smoke |
| Nightly | scheduled physical nodes | Sections 74–78 matrix slices; model/quant/backend/topology rotation; quality, migration, long-context, performance and artifact reproducibility |
| Periodic stress | scheduled exclusive nodes | Section 79 representative soak/stress and storage/cache/link characterization |
| Release candidate | immutable candidate | complete Sections 78–80; rollback/cutover; security/license; two-build reproducibility; signed artifacts, SBOM and provenance |

## Required versus optional

Every matrix entry has owner, rationale, supported platforms, required/optional state, timeout, artifacts, and escalation. A required skip or unavailable required runner leaves the candidate blocked, not green. Optional skips remain visible and may not satisfy release coverage.

## Performance decision

[RECOMMENDATION] Maintain immutable baselines by exact hardware revision, firmware, OS/kernel/driver, runtime, model, backend, topology, power profile, and workload. Interleave baseline and candidate runs in randomized order after declared warm-up. Preserve every repetition and environment trace.

[RECOMMENDATION] Compare a robust central estimate and a confidence interval for the candidate/baseline ratio. Fail only when the interval crosses a preapproved practical-effect threshold; mark inconclusive when uncertainty straddles it and rerun. Thresholds and repetition counts must come from Section 73 pilot variance, not the fork's local absolute speed floors.

Correctness, quality, memory, power, thermal, TTFT, inter-token latency, throughput, and tail latency are separate gates. A throughput win cannot compensate for a correctness or quality loss.

## Reproducibility and provenance

[RECOMMENDATION] Build the candidate from two clean, independently provisioned environments using locked toolchains/dependencies and identical instructions. Compare artifact hashes; if they differ, retain a normalized diff and block “reproducible” status until explained. Emit a build manifest, SBOM, checksums, signatures, and provenance attestation tied to the exact artifact.

## Deployment boundary

Qualification produces a release artifact. Cutover is a distinct, reversible operation that records the prior deployed identity, new identity, migration result, health/readiness, model/API smoke, topology, and rollback test. Source qualification alone never asserts that the listener changed.
