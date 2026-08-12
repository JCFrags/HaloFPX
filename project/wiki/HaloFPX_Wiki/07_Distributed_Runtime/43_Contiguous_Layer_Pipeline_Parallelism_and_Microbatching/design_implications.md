---
section_id: "43"
title: "Pipeline Parallel Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX design candidate"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo premise"]
related_sections: ["32", "38", "39", "42", "44", "45", "46", "48", "51", "52", "58"]
---

# Pipeline parallel design implications

## Rank ownership and cut manifest

**[RECOMMENDATION]** Represent a dense two-stage plan as one immutable cut `k`:

| Component | Rank 0 / prefix stage | Rank 1 / suffix stage |
|---|---|---|
| Token input and embedding | physical owner | absent unless tied-weight policy duplicates it |
| Transformer blocks | `[0,k)` | `[k,L)` |
| Attention/recurrent state | KV/state for owned blocks | KV/state for owned blocks |
| Final norm and output head | absent | physical owner |
| Boundary buffers | send ring | receive ring |
| Sampling | logical authority remains coordinator | execute physical sampling fast path under coordinator-issued state |
| Output commit | receive/validate token result | return token, finish reason, and updated sampler/RNG record |

The plan manifest should include model/tokenizer/quantization hashes; architecture; `k`; exact tensor owners and hashes; tied-weight treatment; boundary tensor names, shapes, dtypes, and alignment; KV/cache ABI; maximum work-unit buckets; graph IDs; transport plan; and sampler protocol version.

**[RECOMMENDATION]** Keep sampler authority consistent with section 39 while colocating its numeric execution with the output head. Sending full vocabulary logits back across the fabric can cost `M*V*e_logits`; returning selected tokens is smaller. The coordinator should issue immutable sampler state/seed/counters, validate the result envelope, and remain the only component that commits user-visible output.

## Stage balancing

**[RECOMMENDATION]** Select `k` from measured candidate cuts, not half the layer count. For each prefill/decode bucket, minimize the larger stage service time subject to:

1. safe peak memory on both ranks, including layer-local KV and pipeline buffers;
2. one complete-block boundary with a supported tensor contract;
3. output-head/embedding/tied-weight and architecture-specific costs;
4. measured boundary transfer and graph/copy overhead;
5. stable p99 latency under the intended concurrency.

Use a weighted workload objective only after publishing the workload weights. Preserve per-phase results because a cut balanced for long prefill may be imbalanced for decode or MoE expert traffic.

## Boundary protocol and flow control

**[RECOMMENDATION]** Each boundary message must identify `(cluster_epoch, plan_id, session_id, session_epoch, work_id, phase, token_start, token_count, shape_bucket, tensor_manifest_hash)`. Use bounded credit-based rings. Rank 0 may reuse a send slot only after transfer completion; rank 1 may release a receive slot only after its graph no longer reads it. Section 45 owns wire encoding and commands; sections 51-54 own transport, integrity, and copy placement.

**[RECOMMENDATION]** Never reorder work units of the same session. Across sessions, allow reordering only before dispatch and retain an explicit mapping from batch row to session/position. Both stages consume the identical batch manifest; cancellation becomes an epoch-tagged tombstone, not silent row removal after rank 0 has run.

## Microbatch and continuous-batching policy

**[RECOMMENDATION]** Use a globally coordinated, forward-only schedule:

1. Section 46 forms a ready set at iteration boundaries.
2. Pack independent decode tokens first up to a measured token/sequence budget.
3. Admit at most one bounded prefill chunk per work unit initially; fill remaining budget with decode rows.
4. Dispatch work `j+1` to rank 0 only when a ring credit exists while rank 1 consumes work `j`.
5. Rank 1 samples only final rows, then returns compact results; the coordinator commits and makes their next decode rows eligible.

**[INFERENCE]** More sequences do not automatically mean useful overlap. Work-unit service-time variance, stage imbalance, boundary copies, and head-of-line blocking can leave bubbles even when concurrency is high [S43-06].

**[RECOMMENDATION]** Keep interactive and throughput classes separately budgeted. Bound how long a work unit may occupy either stage; do not let a long prompt monopolize rank 0 while decode rows wait.

## Long prefill

**[RECOMMENDATION]** Chunk long prompts by a token budget calibrated to near-uniform stage time. Every chunk carries absolute positions, sequence identity, prefix length, and the same causal semantics as an unsplit prefill. A later chunk starts only after both stages commit the prior chunk's KV. Mix decode rows with a prefill chunk only after correctness tests prove the ragged-position and attention metadata path.

**[RECOMMENDATION]** Reserve KV for the admitted maximum or use a validated paged allocator on both ranks. Admission must succeed atomically across both ranks; if either cannot reserve its layer-local share, reject/preempt under section 46 rather than leaving asymmetric state.

## Loading, readiness, and recovery

**[RECOMMENDATION]** Prefer rank-local loading of only assigned tensors from a common content-addressed model source. Current RPC local tensor caching may inform loading optimization, but its proof-of-concept status is not a production contract. Do not declare `READY` until both ranks report exact tensor hashes, allocation sizes, cut range, graph buckets, KV capacity, and a successful boundary self-test.

On stage/link failure:

1. stop admission and advance the affected session epoch;
2. cancel/tombstone in-flight work and discard uncommitted outputs;
3. invalidate any KV newer than the last coordinator-committed token/chunk on both ranks;
4. restart both stage workers into a new cluster epoch;
5. restore only a fully compatible two-rank checkpoint, otherwise replay the committed prompt/tokens;
6. use single-node fallback only when the exact model/context fits and section 48 has validated equivalence; otherwise return a bounded retriable failure or route to an approved smaller model.

**[RECOMMENDATION]** Do not attempt live repartition with active KV in the first implementation. Drain, checkpoint/replay, load a new immutable plan, validate, then admit new work.

## Break-even gate

Promote pipeline mode for a workload bucket only for capacity necessity or when matched end-to-end tests show its p99 SLO and throughput objective outperform eligible alternatives. Compare against single-node and replication when feasible and against tensor parallelism for models requiring two nodes. Report capacity, TTFT, ITL, throughput, energy, and recovery separately; do not call a capacity-only win a latency speedup.
