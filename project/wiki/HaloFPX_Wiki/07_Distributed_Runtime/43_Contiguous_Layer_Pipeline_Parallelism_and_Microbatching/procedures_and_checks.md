---
section_id: "43"
title: "Pipeline Parallel Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX design candidate", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"]
  software_versions: []
  hardware_revisions: ["exact machines must be recorded"]
related_sections: ["32", "38", "39", "45", "46", "48", "51", "52", "58", "73", "76"]
---

# Pipeline parallel procedures and checks

## 1. Internet and source-code research completed now

Pinned and inspected ROCmFPX and llama.cpp layer placement, scheduler prerequisites, RPC limitations, and current revisions. Pinned and inspected vLLM's contiguous partition, supported-model gate, intermediate-tensor handoff, and chunked-prefill guidance. Anchored microbatch scheduling in PyTorch's versioned pipeline API and inference-specific scheduling constraints in Orca and Sarathi. No source establishes HaloFPX performance or production readiness.

Re-run source review whenever a pinned commit changes. Check diffs in loader placement, graph scheduler, RPC transport, KV layout, output-head placement, sampling, and supported architectures before reusing this plan.

## 2. Measurements and inspection required on the two machines

### `DR-43-E1`: placement, load, and correctness oracle

Prerequisites: exact machine/BIOS/kernel/firmware/ROCm/runtime commits; model SHA-256 and GGUF metadata; no root required for inference, though counters may require elevation.

1. Enumerate every model tensor with logical role, layer, shape, packed type, bytes, source hash, and proposed owner.
2. Generate candidate contiguous cuts. Include embedding, norm, head, tied weights, KV bytes per token, graph copies, and safety margin.
3. Load each cut twice and record actual per-rank weight, KV, compute-buffer, host-staging, graph, and peak memory.
4. Compare boundary tensors, final logits, greedy tokens, and fixed-seed stochastic outputs against a single-node oracle under section 48 tolerances.
5. Cover prefill/decode, cache hit/miss, context shift, cancellation, ragged batches, quant types, and each target architecture. Unsupported layouts fail closed.

### `DR-43-E2`: boundary transport matrix

For real boundary bundle shapes, sweep decode batch/concurrency and prefill chunk sizes. Test each physical link, both-link strategies, warm/cold buffers, aligned/misaligned storage, host-staged and any GPU-visible path actually supported. Retain p50/p95/p99 transfer latency, effective bytes, copy-engine/CPU/GPU time, queue delay, retransmits, checksums, ordering errors, and power. Do not extrapolate from USB4 nominal rate.

### `DR-43-E3`: stage and microbatch sweep

Sweep candidate cut, work-unit token budget, sequence count, ring depth, graph/eager mode, prompt/output buckets, and offered load. Timestamp enqueue, rank-0 start/end, transfer start/end, rank-1 start/end, sampling, commit, and stream delivery. Report stage idle/busy time and observed bubbles, not only aggregate tokens/s. Compare single-node, replication, TP, and pipeline where each is feasible and correctness-gated.

### `DR-43-E4`: long-prefill and continuous-batching test

Mix short interactive decodes, new short prompts, and prompts up to the supported context limit. Sweep chunk size and decode/pre-fill priority. Validate absolute positions, logits, KV contents, fairness, cancellation, TTFT, ITL, throughput, and memory. Confirm no session advances until both rank-local KV halves commit the prior chunk.

### `DR-43-E5`: fault and recovery matrix

Inject rank delay/exit/restart, one-link and all-link loss, corrupt/truncated/duplicate/stale/wrong-shape boundary messages, coordinator restart, cancellation races, OOM, and incompatible cache/checkpoint identity. Required behavior: bounded detection; no uncommitted token exposure; both ranks converge on one epoch; invalid cache becomes miss/replay; no hang or cross-session data use. Measure replay/recovery time and client-visible error semantics.

Minimum machine-readable row:

`run_id,timestamp,node_ids,commits,model_hash,plan_id,cut,phase,prompt_tokens,output_tokens,chunk_tokens,sequences,ring_depth,work_id,stage0_ms,transfer_ms,stage1_ms,sample_ms,queue_ms,ttft_ms,itl_ms,e2e_ms,bytes,link_plan,rank0_peak_bytes,rank1_peak_bytes,correct,error,fallback`

## Acceptance gates

- **[MEASURED]** is forbidden until raw rows and full environment metadata are linked.
- All target-model tensors have exactly one documented owner or an explicit justified replica.
- Boundary manifest and epoch validation fail closed; corrupt data is never accepted.
- Oracle tolerances pass before performance comparison.
- No deadlock, leaked ring credit, asymmetric KV commit, or post-cancel output occurs in fault tests.
- Pipeline is described as capacity-only unless matched results establish a latency or throughput benefit.
- p99 comparisons use end-to-end samples; do not add independently measured component p99 values.

## 3. Decisions contingent on those measurements

The production cut, boundary dtype/bundle, transport/link plan, ring depth, work-unit token budget, prefill chunk size, scheduling weights, graph buckets, sampler placement, safe timeout, cache/checkpoint recovery, and break-even policy remain **[OPEN]** until `DR-43-E1` through `DR-43-E5` and sections 46/48/51-54/58 provide evidence.
