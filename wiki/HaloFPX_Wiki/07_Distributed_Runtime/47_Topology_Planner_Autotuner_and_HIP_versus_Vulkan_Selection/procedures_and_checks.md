---
section_id: "47"
title: "Planner Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX proposed runtime"]
  software_versions: []
  hardware_revisions: ["two matched AMD Strix Halo systems (exact BOM open)"]
related_sections: ["37", "55", "73", "74", "75", "76", "78", "79", "80"]
---

# Procedures and checks

## Internet/source-code follow-up

1. Freeze the actual integration commit and enumerate supported ops/build flags for HIP and Vulkan.
2. Pin ROCm, Mesa/Vulkan, RCCL, kernel, and firmware documentation to installed versions.
3. Map every plan field to a code path or explicitly mark it future work.

## On-machine tuning run

Prerequisites: exact machine inventory; pinned model hash; synchronized clocks; stable cooling; raw JSON/CSV retained. Root is needed only for host/power changes.

1. Run backend correctness before performance. Reject missing/fallback operators or quality failures.
2. Measure memory capacity and high-water marks for each backend, batch/ubatch/context/cache state.
3. Measure kernels, single-link/dual-link transport, and collectives by message size.
4. Benchmark single-node HIP and Vulkan, replication, remote draft, layer splits, supported tensor splits, pipeline microbatches, and supported MoE layouts.
5. Cross product only feasible candidates with cache layout, MTP/spec settings, concurrency, and power profile. Randomize and repeat.
6. Validate winners on held-out traces, cold boot, thermal soak, one-link operation, link loss, and worker restart.
7. Generate manifest, evidence hashes, confidence interval, safety margins, and incumbent comparison. Canary, then promote or roll back.

## Mandatory validity checks

- all manifest fingerprints reproduce current runtime state;
- rank ownership and single-node fallback are explicit;
- raw evidence exists for every **[MEASURED]** field;
- no candidate wins by violating quality, memory, thermal, or recovery constraints;
- stale or mismatched manifest fails closed and invokes a safe baseline, not guessed parameters.
