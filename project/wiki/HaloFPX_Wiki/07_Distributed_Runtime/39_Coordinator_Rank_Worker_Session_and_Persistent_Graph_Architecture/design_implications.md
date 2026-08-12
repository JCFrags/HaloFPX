---
section_id: "39"
title: "Coordinator and Rank Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX design candidate"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo premise"]
related_sections: ["38", "45", "46", "48", "55"]
---

# Design implications

## Process responsibilities

| Coordinator | Rank worker |
|---|---|
| API/auth, tokenize/detokenize, admission, deadlines | device and transport initialization |
| model registry and signed manifest selection | validate/load assigned shard and allocate buffers |
| session lease/epoch and mode/rank ownership | execute prefill/decode/collective commands in epoch order |
| sampler and RNG authority | own rank-local KV/cache and graph family |
| routing, fallback, cancellation, output commit | heartbeat, counters, fault/error reporting |

**[RECOMMENDATION]** Keep the coordinator off the per-layer data path after issuing an iteration, except where it must receive final logits/tokens. Data-plane rank-to-rank traffic should use section 45/transport contracts, not JSON control messages.

## Startup handshake

1. Coordinator creates `cluster_epoch` and sends `HELLO(protocol range, build ID, nonce, expected rank)`.
2. Worker replies with rank identity, host/boot ID, backend/device/driver versions, capabilities, memory, transport endpoints, and supported graph/collective features.
3. Coordinator sends immutable model manifest: whole-file hashes, GGUF metadata hash, tokenizer/vocab/chat-template hashes, shard mapping, tensor shape/type/hash records, KV/cache ABI, and graph-plan ID.
4. Worker validates before allocation; after load it returns observed hashes, buffer sizes, and readiness nonce.
5. Coordinator commits topology only if every required rank agrees; otherwise unload and remain unavailable or choose an explicitly valid degraded mode.

**[RECOMMENDATION]** Never accept a basename, size, or model name as shard identity. A mismatch is fatal to the coupled topology and must not be silently coerced.

## Command and session state machine

`UNLOADED -> VALIDATING -> LOADING -> READY -> DRAINING -> UNLOADED`, with `FAULTED` reachable from every active state.

Session: `NEW -> PREFILLING -> DECODING -> {CHECKPOINTING, IDLE} -> CLOSING -> CLOSED`; cancellation increments the session epoch and invalidates late completions.

Every command carries `cluster_epoch, session_id, session_epoch, request_id, iteration, command_seq, deadline, graph_key, payload_refs, expected_outputs`. Every response echoes those values plus status and trace timing. Duplicate command IDs return the prior terminal result or a deterministic duplicate status; they never execute twice.

## Health and shutdown

**[RECOMMENDATION]** Distinguish liveness (heartbeat/control loop), readiness (exact model and graph plan usable), and data-plane health (transport/collective progress). Mark a rank suspect after a configurable missed-heartbeat window, but abort a timed-out collective immediately at its deadline; section 48 determines fallback.

Graceful shutdown sequence: stop admission, drain to deadline, checkpoint only compatible sessions, cancel remainder, synchronize streams/collectives, flush rank-local metadata atomically, destroy graph executables, free buffers/model, close transport, acknowledge shutdown. Crash recovery must treat all unacknowledged outputs as uncommitted.

## Persistent graph policy

**[RECOMMENDATION]** Prebuild only high-frequency prefill/decode buckets after eager correctness succeeds. Use stable preallocated addresses, separate graph families by phase and mode, bound the cache, expose hit/rebuild/failure counters, and fall back to eager execution on any unsupported shape or failed graph update. Do not graph-capture filesystem cache I/O or failure-prone control operations.
