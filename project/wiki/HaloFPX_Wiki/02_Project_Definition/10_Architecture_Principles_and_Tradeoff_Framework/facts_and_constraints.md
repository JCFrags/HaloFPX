---
section_id: "10"
title: "Architecture Evidence and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "fewtarius/CachyLLama", "ggml-org/llama.cpp", "local Agent_Harness"]
  software_versions: ["commits in sources.md"]
  hardware_revisions: ["two-node Strix Halo target"]
related_sections: ["09", "15", "38", "47", "48", "49", "60"]
---

# Evidence and constraints

## Source-backed observations

- **[VERIFIED]** ROCmFPX labels itself experimental and states that results depend on hardware, drivers, model, prompt, and quantization recipe [S10-01].
- **[VERIFIED]** Its own published Strix Halo tables show backend and workload differences, and qualify MTP gains as content-dependent [S10-01]. This supports model/workload-specific tuning, not a universal backend default.
- **[VERIFIED]** Upstream `llama-server` has continuous batching, speculative decoding, prompt caching, timings, metrics, and multiple API paths, so HaloFPX should measure/configure existing mechanisms before adding token-path machinery [S10-02].
- **[VERIFIED]** Upstream notes that prompt-cache reuse can change results because backend batch shapes may not be bit-for-bit identical [S10-02]. Correctness tolerances must therefore be explicit.
- **[VERIFIED]** CachyLLama’s persistent-cache and expert-tracking features are optional/runtime-controlled, and its results are machine/workload scoped [S10-03].
- **[VERIFIED]** The Agent Harness authority requires sources-to-wiki promotion, reversible changes, evaluation, and review rather than silent promotion [S10-04].

## Physical and project constraints

- **[ASSUMPTION]** Two matched APUs and two USB4 paths exist, but symmetry and link independence are not yet measured.
- **[INFERENCE]** Distributed execution introduces coordination, serialization, transport, failure, and versioning costs absent from single-node baselines.
- **[INFERENCE]** Rank-local state simplifies ownership and recovery reasoning; any cross-rank reuse requires an explicit transfer protocol and compatibility proof.
- **[OPEN]** Latency, throughput, capacity, power, reliability, quality, complexity, and maintenance weights have not been ratified.

## Hard invariants

1. No response is published from an invalid or stale distributed epoch.
2. Corrupt/incompatible cache state is never accepted.
3. The active mode, ranks, model, and fallback are observable.
4. Every promoted optimization has a matched baseline, raw evidence, rollback, and applicability key.

