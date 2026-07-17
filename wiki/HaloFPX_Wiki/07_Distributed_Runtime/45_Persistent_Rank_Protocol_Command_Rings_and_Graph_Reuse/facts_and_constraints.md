---
section_id: "45"
title: "Persistent Rank Protocol Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: ["HIP documentation 7.2.53210", "RCCL documentation 2.30.4"]
  hardware_revisions: []
related_sections: ["32", "39", "48", "53", "54"]
---

# Facts and constraints

## Source-backed baseline

- **[VERIFIED]** ROCmFPX RPC protocol `4.0.1` frames commands as one-byte opcode plus an eight-byte request size. Its HELLO response checks an equal major and a server minor not newer than the client; connection capabilities are negotiated separately [S45-01]. This does not establish backward-compatible rank-command semantics.
- **[VERIFIED]** The same RPC client sends `GRAPH_RECOMPUTE` only when the current nonzero `cgraph->uid` equals its last UID. Otherwise it recursively serializes graph tensors. The server reconstructs nodes and maps, may resize a byte vector, computes synchronously, and retains one graph pointer per device [S45-01].
- **[VERIFIED]** At llama.cpp `788e07d`, context initialization reserves prompt-processing and token-generation graphs. Decode still resets the backend scheduler, allocates the current graph, and submits it asynchronously; the scheduler retains allocation capacity, not a stable external distributed graph ABI [S45-02, S45-03].
- **[VERIFIED]** This llama.cpp revision exposes `GGML_HIP_GRAPHS` (default `ON`). Its HIP vendor layer maps CUDA graph calls to HIP, and its backend capture path can update, instantiate, and launch graph executables [S45-04]. That source behavior does not prove correctness or benefit on gfx1151.
- **[VERIFIED]** AMD HIP documents graph template creation/capture, executable instantiation, repeated launch, parameter/executable update, and destruction. AMD says setup is performed once and preallocation is usually better for tight loops [S45-05].
- **[VERIFIED]** Linux `io_uring` is a primary precedent for fixed ring entries, producer/consumer head-tail ownership, completion correlation, bounded-full behavior, and release/acquire publication [S45-06]. NVMe completion queues use a phase bit that changes on wrap so the host can distinguish new entries [S45-07]. These are design analogues, not HaloFPX compatibility claims.
- **[VERIFIED]** RCCL documents communicator abort/async-error facilities and warns that multi-communicator operations need explicit ordering unless launch-order serialization is enabled [S45-08]. The custom USB4 transport and gfx1151 support path remain unverified.

## Non-negotiable correctness constraints

| Constraint | Consequence |
|---|---|
| command identity | Every command and completion carries cluster epoch, ring epoch, sequence, request, session epoch, and iteration. |
| bounded ownership | A producer cannot overwrite an unconsumed slot; a buffer/table entry cannot be reused while referenced by an in-flight sequence. |
| publication order | Descriptor bytes and referenced payload metadata become visible before the producer publishes the tail/phase; the consumer uses acquire semantics. |
| no ambiguous replay | A duplicate sequence returns its cached terminal completion or `DUPLICATE`; it never launches work again. |
| cancellation fencing | Cancellation advances the session epoch. A late GPU/transport completion may release resources but cannot commit output. |
| collective consistency | All ranks execute the same plan/iteration/collective ordinal. A mismatch faults the coupled topology; it is not skipped. |
| graph identity | Reuse requires an exact graph key plus live graph-slot and buffer generations. A mismatch is a miss/rebuild, never approximate reuse. |
| bounded failure | Timeout does not imply device work stopped. Referenced memory remains quarantined until synchronized, communicator aborted, or worker process fenced. |

## Rank ownership and fallback

**[RECOMMENDATION]** Rank workers exclusively own device contexts, assigned weight shard, graph executables, staging/activation buffers, collective communicator, and rank-local KV/cache mutations. The coordinator owns cluster/ring epochs, session lease and epoch, sequence issuance, deadlines, output acceptance, and reset/fallback decisions.

**[INFERENCE]** Because graph launches and collectives are asynchronous, a timeout alone cannot safely authorize buffer reuse. The inference follows HIP stream execution and RCCL abort semantics [S45-05, S45-08].

**[RECOMMENDATION]** A failed two-rank iteration yields no accepted token. Single-node fallback requires a fresh topology/cluster epoch and either (a) a coordinator-held committed checkpoint both implementations validate, or (b) deterministic recomputation from committed tokens. Never continue from partially mutated distributed KV state.

## Explicit research split

- Completed now: source behavior and reusable queue/graph invariants above.
- Machine work: prove memory ordering across the selected transport, HIP graph coverage, reset behavior, and exact state mutation boundary.
- Contingent: ring mapping, graph-update policy, deadlines, and fallback checkpoint boundary.
