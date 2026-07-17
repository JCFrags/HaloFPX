---
section_id: "49"
title: "Fabric Requirements - Design Implications"
status: "draft"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ROCmFPX@a5605a7", "llama.cpp@788e07d"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo premise"]
related_sections: ["38", "39", "50", "51", "52", "53", "54", "55"]
---

# Design implications

## Carrier-independent requirements

| Requirement | Minimal contract | Why HaloFPX needs it |
|---|---|---|
| Versioning | negotiate major, minor, features, limits; fail closed on incompatible major | RPC layouts and stream carriers evolve independently |
| Progress | completion queue plus explicit/cooperative progress mode | inference workers must not block their only compute thread |
| Buffers | opaque registration token, memory kind, lifetime, alignment, revoke | enables reuse without falsely promising zero-copy |
| Flow control | receiver-advertised bytes/messages per lane | prevents prefill from starving control/decode |
| Multipath | stable logical operation over one or more path IDs | Section 52 can change policy without graph code changes |
| Fallback | one-link and TCP-capable baseline | diagnosis and continued operation after degradation |
| Observability | per-path/lane bytes, queue time, RTT, errors, retries, copies, cancellations | policy must be evidence-driven |

**[RECOMMENDATION]** Use separate logical control, latency, and bulk lanes even when they initially share one TCP socket. This preserves semantics and metrics before carrier specialization.

**[RECOMMENDATION]** Define `local_complete` (caller buffer reusable), `remote_received`, and `remote_applied` separately. A send completion must never ambiguously mean all three.

**[RECOMMENDATION]** Setup and cache-control operations require operation IDs and generation checks. After reconnect, retry only operations declared idempotent or reconciled by a query.

**[INFERENCE]** Because current RPC aborts on malformed/crashed-server responses and serializes most exchanges synchronously [S49-02], it is useful for baseline bring-up but cannot be the final latency-isolated multipath API without refactoring.

## Contingent decisions

- Choose direct USB4STREAM for bulk only if it improves matched-size p99/goodput after accounting for framing and copies.
- Choose application striping only above a measured crossover and with bounded reorder memory.
- Add GPU-visible registration only after Section 54 proves a supported memory path.
- Set deadlines from measured service distributions, not cable line rate.

