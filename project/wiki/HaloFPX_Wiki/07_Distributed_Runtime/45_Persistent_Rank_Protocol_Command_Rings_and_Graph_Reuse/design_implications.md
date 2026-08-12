---
section_id: "45"
title: "Persistent Rank Protocol Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX design candidate", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"]
  software_versions: []
  hardware_revisions: ["two-node Strix Halo premise"]
related_sections: ["39", "42", "43", "44", "46", "48", "49", "53", "54"]
---

# Design implications and proposed ABI

Everything in this page is a **[RECOMMENDATION]** unless marked otherwise; no HaloFPX wire ABI is implemented yet.

## Two-plane protocol

Use a reliable, ordered, integrity-protected control connection for `HELLO`, capability/version negotiation, model-plan validation, buffer/graph registration, drain, reset, and diagnostics. Use one logical SPSC command ring and one logical SPSC completion ring per coordinator-rank lane. “Ring” specifies bounded indices and ownership; it does **not** assume cross-host shared memory. Section 49/53 maps it onto messages, shared mappings, or doorbells.

Startup is `DISCONNECTED -> NEGOTIATING -> LOADING -> REGISTERING -> READY`. Execution is allowed only in `READY`. `QUIESCING`, `RESETTING`, and `FAULTED` reject new work.

## Fixed descriptors

Proposed command entry: 128 bytes, little-endian, naturally aligned.

| Offset | Bytes | Field |
|---:|---:|---|
| 0 | 16 | magic, ABI major/minor, opcode, flags, header/entry length |
| 16 | 24 | `cluster_epoch`, `ring_epoch`, `command_seq` |
| 40 | 24 | `request_id`, `session_id`, `session_epoch`, `iteration` |
| 64 | 16 | `graph_slot`, `graph_generation`, monotonic `deadline_ns` |
| 80 | 8 | payload-table ID, input/output reference counts |
| 88 | 24 | opcode-specific scalar arguments; zero unused bytes |
| 112 | 16 | trace ID, descriptor CRC32C, reserved-zero |

Proposed completion entry: 64 bytes containing magic/version/status/flags, cluster and ring epochs, command sequence, request ID, result-table reference/count, rank-state code, detail code, and CRC32C. Full diagnostics travel on the control plane, not in the ring.

Payload tables are preallocated arrays of 32-byte references: `{buffer_id, generation, offset, length, access, integrity}`. Raw pointers never cross the wire. Registration validates range, alignment, access, shape/type metadata, and lifetime. A generation change invalidates every older reference.

## Opcodes and ordering

Keep the token-path opcode set closed and small: `EXEC_PREFILL`, `EXEC_DECODE`, `EXEC_AUX`, `BARRIER`, and `CANCEL_SESSION`. Model loading, allocation, graph registration, and reset remain control-plane operations.

- Consume `command_seq` strictly in order within a ring epoch; gaps, regression, bad CRC/reserved bits, or invalid generations are protocol faults.
- Completion order may differ only if the negotiated lane explicitly permits overlap; sequence always correlates it. First implementation should complete in order.
- Backpressure is structural: `(tail - head) == depth` refuses submission. Admission/scheduler section 46 decides wait, shed, or reroute; it may not overwrite.
- Cache terminal completions for a negotiated replay window. A duplicate returns the same terminal status without execution.

## Epoch, cancellation, and timeout semantics

`cluster_epoch` changes on topology, coordinator leadership, worker boot identity, model plan, or communicator replacement. `ring_epoch` changes on ring reset. `session_epoch` changes on cancellation, rewind, restore, or migration.

Cancellation is cooperative: publish a new session epoch and a cancel command; the worker stops before the next safe launch boundary where possible. Already-enqueued GPU work may finish, but its old-epoch output is discarded. Deadline expiry similarly fences acceptance; it does not assert physical cancellation.

## Graph and buffer reuse

Register a bounded graph table outside request execution. A strict graph key includes:

`model/shard hash + build/backend/driver + rank/topology/collective plan + phase/op + batch/ubatch/context/output buckets + KV/recurrent layout + tensor shapes/types + kernel/quant plan + buffer-table generations`.

Each graph slot has a generation, refcount/in-flight watermark, exact key digest, executable handle, stable input/output addresses, creation/update status, and eager fallback. Command descriptors select only slot and generation. Scalar inputs are copied into stable staging memory before launch.

**[RECOMMENDATION]** First pre-register common decode/prefill buckets using eager execution as the correctness oracle. Permit `hipGraphExecUpdate` only for a measured allowlist of field changes. Any unsupported shape, invalid generation, failed update, or launch error is a graph miss: execute eager if safe and schedule rebuild off-path. Never reuse a stale executable.

## Desynchronization recovery

1. Consumer stops at the first invalid entry and emits `RING_FAULT(expected_seq, observed_seq, epochs, reason, last_completed)` over control.
2. Coordinator stops admission to the coupled topology, fences affected session epochs, and waits only to the configured quiesce deadline.
3. Workers synchronize or abort streams/communicators; all ambiguous outputs and payload references remain quarantined.
4. Peers exchange last-issued/accepted/completed sequence and table generations. They do not “repair” by skipping entries.
5. Coordinator creates a new `ring_epoch`; worker clears rings/replay cache, validates/re-registers live buffers and graphs, then returns `READY` with a nonce.
6. If worker boot/topology/model/communicator changed, also create a new `cluster_epoch` and deterministically recompute or enter the section 48 degraded mode.

## Required telemetry

Per rank/lane expose ring head/tail/depth/high-water, issued/accepted/completed/duplicate/gap/CRC counts, queue and execution latency, deadline/cancel/late-completion counts, buffer-generation faults, graph hit/miss/update/rebuild/eager/failure counts, graph memory, collective ordinal/bytes/error, reset reason/duration, worker boot ID, epochs, plan/hash IDs, and last-progress monotonic timestamp. Logs correlate by trace/request/session/sequence without storing prompt text by default.

## Research split

- Source-complete: proposed ABI is grounded in pinned code and official queue/graph primitives.
- Machine-required: validate sizes, atomics/transport publication, graph key/update allowlist, depths, latency, and failure fencing.
- Contingent: freeze ABI v1 only after experiments and sections 46/48/49/53/54 converge.
