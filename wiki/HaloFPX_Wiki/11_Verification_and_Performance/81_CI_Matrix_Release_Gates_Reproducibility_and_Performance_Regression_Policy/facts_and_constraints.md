---
section_id: "81"
title: "CI, Release Gate, Reproducibility, and Regression Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940", "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"]
  software_versions: ["SLSA 1.2", "Reproducible Builds definition accessed 2026-07-17"]
  hardware_revisions: ["dual-Strix-Halo target; exact revisions open"]
related_sections: ["73", "74", "75", "76", "77", "78", "79", "80"]
---

# Facts and constraints

- **[VERIFIED]** Pinned `llama.cpp` contains platform/backend build workflows, sanitizer workflows, server tests, release automation, tokenizer hash checks, and third-party/vendor checks [S81-01].
- **[VERIFIED]** Pinned ROCmFPX contains an aggregate regression script and fork-specific model/runtime checks. Some checks can skip, so aggregate success is not equivalent to all required coverage passing [S81-02].
- **[VERIFIED]** Reproducible Builds defines a reproducible build as one where the same source, build environment, and instructions produce bit-for-bit identical artifacts [S81-05].
- **[VERIFIED]** SLSA provenance represents where, when, and how an artifact was produced; provenance complements rather than replaces test evidence [S81-06].
- **[VERIFIED]** GitHub documents artifact attestations, SBOM attestations, dependency review/CodeQL, and immutable-release attestations for supply-chain assurance [S81-07][S81-08][S81-09].
- **[RECOMMENDATION]** Per-commit CI cannot depend solely on scarce physical dual-node hardware; hardware suites require queued/nightly/RC tiers with explicit freshness.
- **[RECOMMENDATION]** Required skips, missing artifacts, stale baselines, runner faults, and telemetry gaps cannot silently become passes.
- **[RECOMMENDATION]** Performance comparisons require matched hardware/software/workloads, controlled warm-up/order, raw samples, and a variance-aware decision. A single throughput number is not a regression policy.
- **[INFERENCE]** Fork-local absolute speed floors or hardcoded model paths are scoped experience, not portable HaloFPX thresholds.
- **[VERIFIED]** Signed provenance supplies build-lineage evidence but does not itself establish runtime correctness, model quality, license compliance, or safe deployment [S81-06].
- **[RECOMMENDATION]** Release evidence must identify exact source commits, dependencies, model/dataset hashes, build environment, artifacts, signatures/attestations, tests, and deployed runtime.
- **[OPEN]** No CI duration, variance, performance threshold, reproducibility result, or release-gate result was measured in this research pass.
