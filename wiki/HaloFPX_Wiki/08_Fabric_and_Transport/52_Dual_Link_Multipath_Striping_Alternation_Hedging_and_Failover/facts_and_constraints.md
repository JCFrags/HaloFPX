---
section_id: "52"
title: "Dual-Link Multipath - Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["linux@fce2dfa"]
  software_versions: ["RFC 8684", "RFC 9000", "RFC 9002"]
  hardware_revisions: ["dual USB4 premise"]
related_sections: ["20", "49", "50", "53", "55"]
---

# Facts and constraints

## Primary-source constraints

**[VERIFIED]** MPTCP represents one application connection over multiple TCP subflows. It adds connection-level data sequence numbers so bytes arriving over paths with different delays can be reassembled, and permits retransmission or duplication of the same data sequence on another subflow [S52-01].

**[VERIFIED]** RFC 8684 does not mandate one retransmission/scheduling policy; aggressive reinjection can reduce delay while wasting bandwidth. A broken subflow need not terminate the whole MPTCP connection, but failure/fallback semantics are protocol-specific [S52-01].

**[VERIFIED]** Linux separates MPTCP path management from packet scheduling. The scheduler chooses available subflow(s); documented policy examples include bandwidth maximization or lower latency. Stale subflows are excluded according to configured loss intervals [S52-02, S52-03].

**[VERIFIED]** QUIC uses monotonically increasing packet numbers and duplicate suppression; its recovery specification explicitly accounts for packet reordering [S52-04, S52-05]. These are design references, not a recommendation to adopt QUIC.

## Policy candidates

| Policy | Intended use | Required receiver behavior | Principal risk |
|---|---|---|---|
| Best-link | small latency-sensitive message | normal ordering | unused aggregate bandwidth; stale score |
| Direction separation | simultaneous A->B and B->A bulk/collectives | independent directional lanes | assumes meaningful controller/duplex independence |
| Alternating operations | successive independent collectives/requests | correlation ID; no cross-operation order assumption | per-link imbalance and cache effects |
| Proportional striping | one large message | global message/chunk sequence and bounded reorder buffer | head-of-line at missing slow-path chunk |
| Hedging | small idempotent deadline-critical message | accept first valid `(epoch,message_id)`, discard duplicate | doubles load and may correlate congestion |
| Hot failover | any class | replay/reconcile only safe operations | ambiguous completion and stale in-flight data |

**[RECOMMENDATION]** Every chunk needs connection generation, message ID, total length, chunk offset/length, and integrity coverage. Arrival order is never authority.

## Health signals

**[RECOMMENDATION]** Score paths per direction and message-size class from rolling goodput, completion latency, queue delay, recent timeout/error rate, reconnect generation, and freshness. A score must include hysteresis and a minimum observation count.

**[RECOMMENDATION]** Link state should be `probing`, `healthy`, `suspect`, `draining`, `down`, or `recovering`. A recovered path returns through probes; it does not immediately receive production stripes.

**[OPEN]** Shared-controller, cable, IRQ, memory-bandwidth, and thermal correlations are unknown.

