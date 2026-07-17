---
section_id: "49"
title: "Fabric Requirements - Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ROCmFPX@a5605a7", "llama.cpp@788e07d"]
  software_versions: ["Linux 7.2-rc3 snapshot"]
  hardware_revisions: ["dual Strix Halo premise"]
related_sections: ["20", "38", "50", "51", "53", "55"]
---

# Facts and constraints

## Traffic classes

| Class | Examples | Required semantics | Performance emphasis |
|---|---|---|---|
| Control | session, rank, graph, cancel, error | reliable, ordered within session, idempotency key | tail latency and bounded timeout |
| Decode collective | logits, reductions, token-step metadata | step/epoch identity, deadline, no stale delivery | small-message p99 latency |
| Prefill/bulk | activations, tensor shards | integrity, chunking, backpressure, completion | sustained goodput and low copy count |
| Setup | model identity, buffer/graph creation | transactional or resumable; explicit compatibility | correctness and diagnosability |
| Cache coordination | manifest/digest, lease, invalidate | rank ownership and generation; corruption is miss | correctness before speed |
| Health/diagnostics | heartbeat, counters, trace correlation | must not block data progress | observability under failure |

**[VERIFIED]** Current ggml RPC frames commands as a one-byte command plus native-width request size and payload; calls receive a size-prefixed response. Its backend declares no async tensor operations and `synchronize` is a no-op because operations are synchronous [S49-02].

**[VERIFIED]** Linux USB4STREAM is a raw bidirectional character-device stream. Its driver passes user bytes as-is, uses mandatory fabric end-to-end flow control, and provides DATA/CLOSE packets, but it does not supply HaloFPX message boundaries, authentication, cancellation, or request identity [S49-03].

**[VERIFIED]** RFC 9621 treats properties such as reliability, preservation of order, message boundaries, multipath, racing, and connection-grouping as transport-service features rather than universal guarantees [S49-04].

## Required API contract

```text
endpoint = open(peer, protocol_range, security_policy)
lane     = endpoint.open_lane(class, ordering_domain, priority)
region   = endpoint.register_buffer(ptr, bytes, memory_kind, access)
op       = lane.submit_send(message_header, iov_or_region, deadline, flags)
op       = lane.submit_recv(match, iov_or_region, deadline, flags)
endpoint.cancel(op, reason)
endpoint.progress(budget) / completion_queue.wait(deadline)
endpoint.paths() / endpoint.set_policy(policy_generation)
```

**[RECOMMENDATION]** A message header must carry protocol major/minor, peer/session/rank IDs, lane, message type, message and correlation IDs, epoch/step, payload length, flags, checksum policy, and optional chunk coordinates. Multi-byte encoding must be explicitly endian-stable.

**[RECOMMENDATION]** “Carry” is a logical protocol requirement, not a demand that every value consume bytes in the fixed transport header. Section 53 maps connection-scoped identities into its authenticated negotiation transcript, record-routing values into the fixed header, and operation-scoped rank/correlation/step values into a canonical authenticated upper-layer descriptor. A conforming implementation must expose that mapping in logs and negative tests; inference from an unauthenticated socket address is insufficient.

**[RECOMMENDATION]** Ordering is per lane and ordering-domain, not global. Reliability means a positive remote acceptance/completion signal where semantics require it; reconnect never implies that a non-idempotent operation was not executed.

**[RECOMMENDATION]** Cancellation is best-effort and observable: states are `requested`, `locally_stopped`, `peer_acknowledged`, or `too_late`. Cancellation cannot revoke already-visible writes.

**[OPEN]** Latency, bandwidth, queue-depth, and timeout numbers are intentionally unset pending `FT-49-E2` and Section 55.
