---
section_id: "45"
title: "Persistent Rank Protocol, Command Rings, and Graph Reuse"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: ["HIP documentation 7.2.53210"]
  hardware_revisions: ["dual Strix Halo premise"]
related_sections: ["32", "39", "42", "43", "44", "46", "48", "49", "53", "54"]
---

# 45 - Persistent Rank Protocol, Command Rings, and Graph Reuse

**[RECOMMENDATION]** Use a long-lived rank process with a slow, reliable control channel and a bounded command/completion-ring data path. Load and validate the shard once, preallocate named buffer generations, register a bounded graph table, then execute token iterations with fixed-size descriptors that refer only to existing objects. No model/graph serialization, graph construction, container growth, filesystem I/O, or device allocation belongs on the token path.

**[VERIFIED]** At the pinned ROCmFPX revision, ggml RPC can reuse the last graph UID through `RPC_CMD_GRAPH_RECOMPUTE`; a new graph is serialized and reconstructed, the server may resize its graph backing vector, and only one stored graph exists per device [S45-01]. This is a useful prototype, not the protocol specified here.

## Proposed contract at a glance

```text
reliable control: HELLO -> LOAD_PLAN -> BUFFER_TABLE -> GRAPH_REGISTER -> READY
token data path:  coordinator command ring -> rank executor -> completion ring
recovery:         QUIESCE -> RING_FAULT -> new ring_epoch -> re-register -> READY
```

The coordinator owns admission, global and session epochs, authoritative output commit, and fallback. Each rank owns its loaded shard, device context, local buffer/graph tables, rank-local KV state, execution stream/collective order, and completion production. A rank failure cannot be treated as success; single-node fallback starts only from state that section 48 proves reconstructable.

## Pages

- [Facts and constraints](facts_and_constraints.md)
- [Design implications and proposed ABI](design_implications.md)
- [Procedures and checks](procedures_and_checks.md)
- [Open questions](open_questions.md)
- [Sources](sources.md)

## Required three-way research split

1. **Internet/source-code research completed now:** pinned ROCmFPX graph-recompute/RPC framing, pinned llama.cpp scheduler reserve/allocation and HIP-graph backend paths, AMD HIP graph lifecycle/update API, Linux ring memory-ordering patterns, NVMe phase-tag precedent, and RCCL ordering/failure interfaces.
2. **Actual-machine inspection/measurement required:** descriptor transport and polling cost, ring depth/backpressure, graph capture/update coverage on gfx1151, graph-key stability, buffer-address stability, cancellation/timeout latency, and reset/fallback correctness on both nodes.
3. **Decisions contingent on those results:** descriptor/ring sizes, polling versus wakeup policy, graph buckets/capacity, whether HIP graph update is enabled, deadline values, collective implementation, and which session boundaries permit single-node fallback.

## Improvement review

This section turns the section 39 lifecycle sketch into a reviewable wire/data-path candidate and records exact prototype limitations. It deliberately leaves performance constants and transport mapping open until measured.
