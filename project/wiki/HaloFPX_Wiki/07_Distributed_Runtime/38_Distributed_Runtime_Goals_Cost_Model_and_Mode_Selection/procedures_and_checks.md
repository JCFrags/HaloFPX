---
section_id: "38"
title: "Mode Selection Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX design candidate"]
  software_versions: []
  hardware_revisions: ["exact machines must be recorded"]
related_sections: ["47", "48", "51", "66", "67"]
---

# Mode selection procedures and checks

## Internet/source-code research completed

Pinned current upstream heads, inspected RPC/speculation/server/data-parallel behavior, and anchored TP/speculation/scheduling claims in primary papers. Recheck pinned sources because all repositories are fast-moving.

## `DR-38-E1`: matched mode matrix

Prerequisites: both nodes isolated from unrelated load; exact BIOS, kernel, ROCm, firmware, runtime commit, model SHA-256, GGUF metadata, power policy, link mapping, and ambient conditions recorded. Root is not required for client traffic; hardware-counter collection may require elevated access.

1. Validate single-node correctness and collect ordinary-decode baselines on each node.
2. Measure each physical link separately, then both together, for 64 B through largest expected collective payload; record one-way/round-trip p50/p95/p99, effective payload throughput, CPU/GPU copy time, retransmits, and outliers.
3. Run fixed prompt/output-length buckets for prefill-heavy, decode-heavy, multi-session, and cache-hit/miss workloads.
4. Test replication, remote speculation, TP, pipeline, and MoE hybrid only when implemented and correctness-gated.
5. Sweep admitted concurrency; hold input set, seeds, sampler, model hashes, and warmup policy constant.
6. Repeat randomized mode order across at least three runs; retain raw per-request timestamps rather than summaries alone.
7. Inject one-link and one-rank loss; record detection, in-flight disposition, fallback, and recovery.

Minimum machine-readable row: `run_id, timestamp, node_ids, commits, model_hash, mode, phase, prompt_tokens, output_tokens, batch, concurrency, cache_state, link_state, ttft_ms, itl_ms, e2e_ms, accepted_tokens, bytes_by_link, queue_ms, error, fallback`.

## Acceptance checks

- **[MEASURED]** labels are forbidden until raw data and environment metadata are linked.
- Compare end-to-end p99 directly and include sample count/confidence method.
- Confirm outputs under section 48's correctness policy before comparing speed.
- Treat corruption or incompatible cache identity as a miss/recompute, never as a hit.
- Do not extrapolate from theoretical USB4 lane rate.

## Decisions contingent on results

Numeric SLOs, safe memory headroom, switching guard bands, speculation acceptance threshold, TP payload cutoff, pipeline microbatch threshold, and MoE replication set all remain contingent on `DR-38-E1` and downstream experiments.
