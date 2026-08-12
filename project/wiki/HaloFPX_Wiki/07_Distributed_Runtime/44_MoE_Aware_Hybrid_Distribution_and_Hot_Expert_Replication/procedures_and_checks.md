---
section_id: "44"
title: "MoE Hybrid Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: ["vLLM@9354f222042986addf20709e5274fc26e0d09745", "Megatron-LM@740c16e6b80a753bea26232148d9bb2d7f0c827a", "DeepEP@dd758caf451848bd150e1046af3d0a73e5fff38d"]
  hardware_revisions: ["dual AMD Strix Halo; exact BOM and USB4 fabric measurements unresolved"]
related_sections: ["34", "45", "52", "73", "76", "78", "80"]
---

# Procedures and checks

All procedures are non-destructive by default. Root access is not required for offline tensor/trace analysis; device profiling or transport setup may require privileges documented by sections 24, 27, and 52. Store raw commands, environment manifests, and outputs under `experiments/`, not in this section.

## E44-01 - Exact model and memory inventory

**Prerequisites:** pinned GGUF/model hash, exact runtime commit/build flags, both machines idle.

1. Record model SHA-256, GGUF metadata, tensor names, dimensions, types, and allocated bytes.
2. Classify per layer: router, routed expert, shared expert, attention/dense, embeddings/head, and state/cache.
3. Record runtime allocations after load: weights, KV/recurrent state, graph arenas, scratch, transport buffers, and OS reserve.
4. Construct candidate static owner maps and exact replica byte totals.
5. Reject any map that depends on nominal parameter count or leaves safety headroom undefined.

**Output:** machine-readable tensor inventory, per-rank memory ledger, candidate map digests.

## E44-02 - Representative expert routing trace

Instrument the router path described by [section 34](../../06_Models_Quantization_and_Inference/34_MoE_Routing_Expert_Telemetry_and_Expert_Placement_Inputs/procedures_and_checks.md).

1. Use frozen prompt sets for code, prose, tool use, long context, and multi-session traffic.
2. Record prefill and decode separately: layer, token position, selected logical expert IDs, router weights, batch/session ID, and timestamps.
3. Repeat across seeds only where sampling changes subsequent tokens; preserve generated token IDs.
4. Calculate per-window assignment counts, slowest-rank predicted load, remote fraction, rank correlation across windows, and hot-set churn.
5. Treat expert choices as sensitive derived data; minimize retention and follow section 71 before exposing traces.

**Gate:** no replica proposal without stable workload-specific evidence and a recorded observation horizon.

## E44-03 - Fabric curves at actual MoE shapes

Coordinate with section 52. Measure one-way and bidirectional latency/throughput for:

- router metadata and tiny decode payloads;
- `N * H * b` activation blocks for observed decode/continuous-batch sizes;
- top-k expanded and destination-coalesced layouts;
- dispatch followed by combine;
- one link, the other link, both links, and single-link failover.

Record p50/p95/p99, CPU/GPU copy time, synchronization time, padding, retries, and effective payload bytes. **[RECOMMENDATION]** Do not extrapolate from bulk TCP bandwidth to token-dispatch latency.

## E44-04 - Local expert kernel and packing baseline

For each observed per-expert token count and quantization:

1. measure pack/permutation, expert compute, weighting, unpermute/combine, and empty-expert overhead;
2. compare HIP and Vulkan only where both implement the exact operation;
3. retain warmup policy, clocks, power, thermals, and raw profiler traces;
4. verify output against the unmodified single-node path within section 78's tolerance policy.

## E44-05 - Matched end-to-end topology comparison

Compare these plans with identical model bytes and request schedule:

1. single-node reference where feasible;
2. whole-layer contiguous pipeline;
3. static cold-expert partition without replicas;
4. static partition plus telemetry-selected replicas;
5. dynamic map change only after the static cases pass.

For each, report TTFT, inter-token p50/p95/p99, throughput, energy if available, per-rank busy/idle time, link bytes, remote assignment fraction, memory high-water marks, and errors. Separate prefill, decode, and concurrent serving. Run randomized repeated trials under matched thermal conditions per sections 73 and 76.

**Correctness gate:** same tokenization, router logical IDs/weights, final logits or agreed numerical tolerance, sampled tokens under controlled RNG, and cache/state ownership. A performance win with changed logical routing does not pass.

## E44-06 - Reconfiguration and fault safety

1. Prepare a new map while the old epoch serves traffic; verify inactive-slot checksums.
2. Attempt activation with one rank missing, wrong digest, insufficient memory, and stale epoch; each must fail closed.
3. Activate at a drained boundary and verify no command or buffer crosses epochs.
4. Inject link loss before dispatch, between dispatch/combine, and after combine acknowledgment.
5. Verify the affected step aborts and cannot commit partial output.
6. Test replay from the last safe boundary and full single-node reload only where the complete model fits.

**Expected behavior:** missing/corrupt expert data causes rejection or recomputation, never accepted partial state.

## Static review checklist

- [ ] Every logical expert has at least one physical owner in every layer.
- [ ] Shared experts and router semantics match the pinned model graph.
- [ ] Replica weights/layout fingerprints are identical or equivalence-tested.
- [ ] Exact memory plus safety headroom fits on both ranks.
- [ ] Dispatch and combine name token origin, destination, counts, and plan epoch.
- [ ] Backpressure replaces token dropping.
- [ ] Rank loss behavior and single-node fallback are explicit.
- [ ] Source, model, runtime, transport, and hardware revisions are pinned.

## Research split

- **Completed now:** source contracts and test design.
- **Must run on the two machines:** E44-01 through E44-06; none has been run in this research pass.
- **Contingent:** topology, replica budget, reconfiguration cadence, and fallback promotion wait for those artifacts.
