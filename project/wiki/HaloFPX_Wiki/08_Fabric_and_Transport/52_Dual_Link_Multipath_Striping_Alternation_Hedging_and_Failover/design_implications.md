---
section_id: "52"
title: "Dual-Link Multipath - Design Implications"
status: "draft"
last_verified: "2026-07-17"
applies_to:
  repositories: ["linux@fce2dfa"]
  software_versions: ["Linux MPTCP or HaloFPX application multipath"]
  hardware_revisions: ["two-link premise"]
related_sections: ["38", "41", "42", "43", "49", "50", "53", "55"]
---

# Design implications

## Recommended staged policy

1. **[RECOMMENDATION] Baseline:** one logical connection per physical link, best-link for latency, other link warm and probed; control traffic remains available during data-path changes.
2. **[RECOMMENDATION] Safe utilization:** assign independent operations or opposite directions to different links. No single-message reorder machinery is required.
3. **[RECOMMENDATION] Bulk striping:** split only payloads above measured threshold `T_bulk`; allocate chunks proportionally to predicted deliverable bytes before deadline, not nominal line rate.
4. **[RECOMMENDATION] Selective hedge:** duplicate only whitelisted idempotent messages when predicted first-path completion threatens a deadline. Cancel/ignore the loser by message ID.

## Scheduler sketch

```text
if no healthy path: fail boundedly
if message is non-idempotent control: best healthy path; reconcile after ambiguity
if message is hedge-eligible and deadline risk exceeds gate: race two paths
if size < T_bulk: best path for this size/direction class
if two paths healthy and independence gate passed: weighted chunk striping
else: single healthy path
```

**[RECOMMENDATION]** Weight path `i` by conservative recent goodput after subtracting queued bytes and a tail-latency penalty. Cap weight change per interval to prevent oscillation. Exact formula and windows remain experiment outputs.

**[RECOMMENDATION]** The receiver maintains bounded sparse chunk state keyed by `(peer, connection_generation, message_id)`. It validates range/non-overlap, discards exact duplicates, rejects conflicting duplicates, verifies whole-message integrity, and releases bytes only when completion semantics allow.

**[RECOMMENDATION]** On either path failure under HaloFPX wire v1: stop new assignment, barrier all rails, fail or drain outstanding work at an identified upper-layer commit boundary, terminate the global epoch, and reject every late old-epoch chunk, credit, ACK, nonce, or rank-state update. Renegotiate keys, counters, credits, topology, and rail generations before traffic resumes. Only an idempotent, uncommitted **whole upper-layer operation** may be retried in the new epoch; missing chunks and partial records never migrate across epochs. A same-epoch failover design would require a distinct protocol version plus correctness, security, and fault proof and remains **[OPEN]**.

## MPTCP versus application multipath

**[INFERENCE]** MPTCP is valuable as a low-development TCP baseline with standardized connection-level sequencing and subflow failover. Application multipath remains necessary if HaloFPX needs message-class scheduling, explicit per-message hedging, carrier mixing, or direct USB4STREAM.

**[RECOMMENDATION]** Benchmark MPTCP, two ordinary TCP connections with application scheduling, and direct-stream policies under the same workload before choosing.
