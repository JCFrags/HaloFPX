---
section_id: "41"
title: "Remote Draft Protocol Design"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX design candidate"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo premise"]
related_sections: ["38", "39", "46", "48", "52", "55"]
---

# Design implications

## Protocol roles

- Target/coordinator: authoritative committed token history, sampling parameters/transforms, RNG stream/counters, target KV, verification, client output, acceptance metrics, fallback.
- Draft worker: draft model/KV, speculative branch positions, proposal generation, optional exact `q` metadata, rollback to committed position.

Message identity: `cluster_epoch, session_id, session_epoch, round_id, base_position, base_token_hash, algorithm_id, sampler_abi, max_draft, deadline`.

`PROPOSE` contains ordered token IDs, per-position draft metadata required by algorithm, draft timing, and resulting speculative position. `VERIFY_RESULT` contains accepted prefix length, committed replacement/bonus token(s), new committed position/hash, rollback target, and next depth/disable instruction.

**[RECOMMENDATION]** Keep one outstanding proposal round per session initially. Pipeline multiple sessions for utilization, but do not allow overlapping branches of one session until epoch/branch correctness is proven.

## Cache and rollback

Each model maintains its own KV; KV is not interchangeable. Draft advances a tentative branch and retains a checkpoint at `base_position`. After verification it commits the accepted prefix plus authoritative continuation token if compatible, otherwise truncates to the committed boundary and evaluates required correction tokens. Target removes unaccepted speculative KV entries.

**[RECOMMENDATION]** Cache corruption, missing branch, token-hash mismatch, or rollback failure disables speculation for that session and recomputes from authoritative tokens. It must never accept stale tentative state.

## Sampling profiles

| Profile | Wire metadata | Guarantee |
|---|---|---|
| `greedy-v1` | token IDs only | same as target greedy if target verifies every token |
| `exact-rejection-v1` | token IDs, exact draft distributions or lossless equivalent, centralized RNG contract | target distribution if implementation matches paper and tests |
| `compact-approx-v1` | token IDs plus bounded/top-k metadata | **[RECOMMENDATION]** experimental changed distribution; never label exact |

Sampler transforms (temperature, penalties, top-k/p, grammar) must be applied in the same defined order to distributions being compared. Session-history penalties use the same committed history.

## Adaptation and fallback

**[RECOMMENDATION]** Track proposed/accepted/rejected tokens, accepted-run histogram, draft/verify/round p99, bytes, rollback time, and ordinary-decode counterfactual buckets. Reduce depth or disable after sustained negative measured benefit, low acceptance, deadline pressure, node/fabric tail spikes, incompatibility, or any protocol error. Target-only decode continues without draft KV.

## Batching

Batch independent sessions on the draft node only when compatible model/sampler buckets align. Target verification may batch proposal blocks, but must preserve per-session causal order and RNG. Optimize accepted tokens per target verification and p99 ITL, not draft throughput alone.

## MTP comparison

**[INFERENCE]** Native MTP is preferable when the target/model runtime supports it efficiently because it avoids a separate general draft model and may share target state. Remote drafting is preferable only if independent-node compute plus minimal wire traffic wins after accounting for lower compatibility and two KV copies. This must be measured per model.
