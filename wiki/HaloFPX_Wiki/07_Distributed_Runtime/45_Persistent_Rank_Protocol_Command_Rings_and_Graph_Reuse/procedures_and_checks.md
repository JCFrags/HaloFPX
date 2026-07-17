---
section_id: "45"
title: "Persistent Rank Protocol Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["future HaloFPX implementation"]
  software_versions: []
  hardware_revisions: ["two matched Strix Halo systems; exact revisions pending"]
related_sections: ["39", "46", "48", "53", "54", "73", "75", "78", "80"]
---

# Procedures and checks

No benchmark is reported here. All experiments require a debug/trace build, exact binary and source commit, model/shard hashes, OS/kernel/amdgpu/firmware/ROCm versions, boot IDs, topology manifest, transport configuration, and raw monotonic event logs. Root is not normally required; document any transport/profiler exception.

## `DR-45-E1`: ring and protocol conformance

1. Generate golden 128-byte command and 64-byte completion vectors for every ABI version/opcode/status. Decode on both hosts and byte-compare canonical re-encoding.
2. Property/fuzz test lengths, reserved bits, CRC, numeric overflow, buffer ranges/generations, invalid opcodes, and version negotiation. Reject before device access.
3. At wrap boundaries, sweep ring depths and sequence values; inject full/empty, duplicate, gap, regression, stale cluster/ring/session epoch, reordered and delayed messages.
4. Verify release/acquire or equivalent transport publication with payload pattern checks under CPU migration and sustained load. Run ThreadSanitizer on a host-only ring model where supported.
5. Prove a full ring applies backpressure without overwrite; a duplicate executes zero additional graph/collective work; invalid input produces one bounded fault.

Acceptance: no silent acceptance, double execution, overwrite, or stale completion in at least the predefined deterministic test matrix. Performance thresholds remain unset until baseline data exists.

## `DR-45-E2`: allocation-free token path and graph reuse

1. Warm/load the exact shard; register fixed buffer and graph tables. Capture allocator hooks, ROCm/HIP trace, graph key/slot/generation, addresses, and scheduler events.
2. For eager and graph paths, sweep prefill/decode, batch/ubatch/context/output buckets, KV positions, model modes, and collective plans.
3. Compare token IDs and logits/partials under section 78 tolerances. Record graph instantiate/update/launch, hit/miss/rebuild/fallback, CPU time, GPU time, queue time, memory high-water, and all allocations.
4. Mutate one graph-key field at a time and one purportedly updateable scalar at a time. A key mismatch must miss; an update failure must rebuild off-path or use eager execution.
5. Assert zero graph/tensor serialization, heap/container growth, device allocation, model/cache I/O, and graph construction between descriptor acceptance and terminal completion after warmup.

Acceptance: eager/graph correctness matches the agreed oracle; stable-address and zero-allocation assertions hold for supported buckets. Latency benefit is **[MEASURED]** only after raw paired results are retained.

## `DR-45-E3`: cancellation, timeout, and desynchronization

Inject cancellation and expiry before dequeue, during staging, graph launch, collective, and completion publication. Inject CRC failure, sequence loss/duplication/reordering, ring overrun attempt, stale table generations, transport disconnect, worker kill/restart, coordinator restart, and collective hang.

Verify: no late output commits; referenced buffers are not reused early; reset obtains a new ring epoch; worker restart/topology change obtains a new cluster epoch; readiness requires revalidation; ambiguous distributed KV is discarded/recomputed; recovery is bounded or fails closed.

## `DR-45-E4`: depth, polling, and replay-window sweep

Under matched request traces, sweep ring depth, wakeup/busy-poll/adaptive policy, replay cache size, number of lanes, graph-table capacity, and telemetry sampling. Record CPU utilization, power, command-to-launch latency distributions, throughput, backpressure time, memory, and thermal state on both nodes. Compare only matched configurations per section 73.

## Failure triage bundle

Preserve protocol/build/model/topology hashes; worker boot and all epochs; last issued/accepted/completed sequence; ring indices and a redacted fixed window around the fault; payload/graph table generations; HIP/RCCL/transport status; reset state transitions; allocator events; and output-commit decision. Do not preserve secrets or prompt text.

## Research split and contingent gates

- Internet/source audit is complete for this draft; recheck pinned heads before implementation.
- `DR-45-E1..E4` require the actual two machines.
- ABI freeze, depths, polling, graph allowlist/capacity, deadlines, replay window, and degraded-mode boundary remain contingent on those results and sections 46/48/49/53/54.
