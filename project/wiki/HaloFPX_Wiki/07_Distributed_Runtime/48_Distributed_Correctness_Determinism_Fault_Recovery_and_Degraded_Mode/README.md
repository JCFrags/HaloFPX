---
section_id: "48"
title: "Distributed Correctness, Determinism, Fault Recovery, and Degraded Mode"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "ROCm/rccl@57e58688f44c77076ad536ef1f6b68741fc6e694"]
  software_versions: ["RCCL source snapshot 57e5868"]
  hardware_revisions: ["two matched AMD Strix Halo systems (exact BOM open)"]
related_sections: ["39", "42", "43", "45", "46", "53", "57", "58", "61", "63", "78", "80"]
---

# Distributed correctness, determinism, fault recovery, and degraded mode

**[VERIFIED]** llama.cpp warns that reused prompt state and different batch shapes need not yield bit-identical logits [S48-LLAMA-SERVER]. **[VERIFIED]** RCCL provides ordered collective APIs and asynchronous error inspection, but an API existing does not provide application-level recovery [S48-RCCL-API, S48-RCCL-SRC].

**[RECOMMENDATION]** HaloFPX correctness is a protocol property: immutable compatibility fingerprint, one sequence owner, monotonically increasing session/plan epochs and step IDs, deterministic collective ordering, atomic visibility of streamed tokens, and fail-closed state validation. A partial rank/cache result must never be accepted as valid continuation state.

**[OPEN]** The physical sampler owner and exact RNG/grammar handoff, cross-rank checkpoint commit point, transport integrity/completion contract, numerical oracle, one-link behavior, and full-state single-node fallback are not selected or proven. The coordinator-ownership recommendation must not be interpreted as evidence that rank-local sampling or transparent continuation is safe [S48-OQ-01 through S48-OQ-09].

## Recovery rule

When a required rank or link becomes uncertain, stop committing new visible output for the affected epoch. Abort/drain the communicator, invalidate uncommitted steps, and recover only from a mutually validated checkpoint plus input/output ledger. If that cannot be proven, restart from the original prompt. Single-node fallback begins a new epoch and is permitted only when a prevalidated full-model plan fits one node.

## Research split

- **Internet/source-code research completed:** pinned upstream nondeterminism warning, server controls, RCCL snapshot/error surface, and deterministic-mode precedent.
- **On-machine work required:** logits/tokens across batch/backend/topology; collective-order tests; cable/link/rank/cache/model faults; replay visibility; one-link and single-node fallback.
- **Contingent decisions:** numerical tolerances, checkpoint interval, replay contract, retry limits, one-link support, and which models fit degraded mode.

No **[MEASURED]** target-machine claim is made.
