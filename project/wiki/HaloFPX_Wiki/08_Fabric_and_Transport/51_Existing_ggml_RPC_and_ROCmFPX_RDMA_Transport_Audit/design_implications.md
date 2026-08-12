---
section_id: "51"
title: "ggml RPC and RDMA Audit - Design Implications"
status: "draft"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ROCmFPX@a5605a7", "llama.cpp@788e07d"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo premise"]
related_sections: ["32", "39", "49", "50", "52", "53", "54", "55"]
---

# Design implications

## Reuse / replace matrix

| Component | Disposition | Reason |
|---|---|---|
| RPC device discovery and remote buffer lifecycle | reuse for bring-up | already maps ggml backend concepts |
| Tensor/graph serializer and server validation | reuse with compatibility hardening | useful semantic inventory; native ABI/pointer IDs need explicit contract |
| hash-before-large-set cache | reuse only after integrity hardening | avoids setup transfer, but 64-bit FNV is not a collision-resistant content identity |
| graph UID recompute | prototype pattern | single remembered graph and synchronous execution are too narrow for concurrent sessions |
| TCP transport | retain baseline/fallback | simple and diagnosable |
| verbs negotiation | experimental reference | capability exchange/fallback useful; carrier availability unproven |
| sequential SEND/RECV staging loop | replace for final bulk path | serialized completions and host copies limit pipelining |
| abort/assert failure policy | replace | distributed faults must isolate session/rank and return structured status |

**[RECOMMENDATION]** First establish an exact pinned RPC baseline on `thunderbolt-net`. Preserve wire captures, compatibility tuple, and latency/goodput as the comparison floor.

**[RECOMMENDATION]** Refactor semantic RPC from byte carrier through the Section 49 interface before adding USB4STREAM. The semantic layer should not know sockets, verbs, or stream FDs.

**[RECOMMENDATION]** Replace pointer-derived wire identity with connection-scoped opaque IDs; encode all integers in defined byte order; negotiate structure/operation features; cap every length before allocation/copy.

**[RECOMMENDATION]** Content-cache acceptance must use a strong digest plus exact length and model/artifact namespace. Cache corruption or mismatch must be a miss/retransfer, never accepted state.

**[RECOMMENDATION]** If verbs remain relevant, add multiple in-flight work requests, event/adaptive polling, cancellation, timeouts, bounds checks, and registered-buffer lifetime management. GPU-direct remains contingent on Section 54.

