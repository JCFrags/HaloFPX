---
section_id: "53"
title: "Framed transport procedures and checks"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: []
  software_versions: ["HaloFPX wire protocol v1 proposal"]
  hardware_revisions: ["Two-host Strix Halo USB4 cluster; exact revisions OPEN"]
related_sections: ["48", "50", "52", "54", "55", "71", "75", "80"]
---

# Procedures and checks

No transport probe implementation exists in this workspace. Commands labeled **planned CLI** are an interface contract to implement and pin before execution, not commands that were run. No result below is **[MEASURED]**.

## Common prerequisites and evidence

- Root: **not required** for off-node codec/fuzz tests or ordinary TCP tests. USB4STREAM provisioning is privileged and requires separate section-50 authorization; the long-running worker must not remain root.
- Record UTC time, host IDs, source/build hashes, compiler/sanitizer/crypto-library versions, kernel/config/module hashes, security profile, negotiated transcript hash, limits, seeds, exit codes, raw counters and before/after logs.
- Preserve USB4NET management/recovery. No procedure here authorizes kernel installation, ConfigFS mutation, cable pulls, firewall changes, service changes, model loading, or secret logging.

## S53-EXP-001 — Codec conformance, property tests, and fuzzing

Implement deterministic golden vectors for every header/type/security profile and independent encoder/decoder cross-checks. Planned CLI:

```bash
./halofpx-transport-probe selftest --vectors testdata/wire-v1.json --json out/selftest.json
./halofpx-transport-probe fuzz-replay --corpus testdata/fuzz-corpus --seed 53001 \
  --iterations 1000000 --json out/fuzz.json
```

Required cases: every split/coalescing point; zero/max lengths; integer overflow; bad CRC/HMAC/AEAD tag; unknown flags/types; duplicate fields; stale/cross-epoch records; record-counter wrap boundary; credit duplicate/gap/overflow; fragmented/reordered/duplicate/overlapping chunks; malformed upper-layer descriptors; wrong rank/session/correlation/step binding; cancellation races; `EINTR`, `EAGAIN`, EOF and short I/O. Run ASan/UBSan and coverage-guided fuzzing. Pass: golden vectors and the Section 49 trace map match, no crash/UB/leak/unbounded allocation, malformed input fails at the specified boundary, corpus/coverage receipts retained.

## S53-EXP-002 — Identical-codec TCP flow-control/liveness test

Prerequisite: pinned probe binary on both hosts and a dedicated bound USB4NET/TCP address; root no. Planned CLI shape:

```bash
./halofpx-transport-probe receive --backend tcp --listen 10.44.0.2:5353 \
  --profile auth-integrity --limits limits-v1.json --json out/receiver.json
./halofpx-transport-probe send --backend tcp --connect 10.44.0.2:5353 \
  --profile auth-integrity --manifest corpus-v1.json --json out/sender.json
```

Sweep declared receive windows, descriptor limits, record sizes, batching caps and queue depths one factor at a time. Artificially delay consumption and control processing. Pass: sender never exceeds credits, resident/reassembly memory remains bounded, control/heartbeat progresses under bulk saturation, every accepted message has exact coverage/digest, and graceful drain returns all credits.

## S53-EXP-007 — Proposed finite-state protocol model gate

**[RECOMMENDATION]** Before wire-v1 implementation freeze, create a small TLA+ model and run TLC from pinned TLA+ Tools `v1.7.4` (`5a47802b5c391f59ecdd44117981f4ff8c0656ba`). Model two rails, loss/duplication/reordering, DATA and CREDIT in flight, cancellation, crash/restart, global epoch reset, whole-operation retry, control reserve, and stale records. Assert credit conservation, no refund on DATA loss, idempotent credit application, epoch isolation, safe retry, bounded queues, and conditional control progress. Retain `.tla`, `.cfg`, tool/JAR SHA-256, command, state/depth/runtime, results, and deliberate broken variants that produce expected counterexamples [S53-SRC-011, S53-SRC-012].

**[OPEN]** No TLA+/TLC model or model-checking run has been executed for HaloFPX. A bounded pass would validate only the reviewed abstraction/configuration; it would not prove C++ conformance, cryptography, performance, or unbounded behavior. Model-derived protocol changes require human review and corresponding simulator/fault tests.

## S53-EXP-003 — USB4STREAM short-I/O and correctness soak

Prerequisites: section 50 proves exact service/device pairing, permissions, clean rollback and compatible kernel on both nodes; explicit maintenance authority; root only for provisioner. Run the identical codec/corpus over one rail, the other, then both. Force small user-space read buffers, nonblocking operation and batch caps; record every short read/write, `EAGAIN`, poll event and driver/kernel error.

Pass before any integration proposal: at least 1,000,000 application records on every candidate topology, zero corruption/loss/duplicate/conflicting overlap/cross-epoch acceptance/deadlock/unbounded growth, exact whole-corpus digest, clean descriptor/ConfigFS teardown, and unchanged USB4NET recovery state. This threshold comes from scoped decision S53-SRC-001; it is not a general industry requirement.

## S53-EXP-004 — Dual-rail fault, reconnect, timeout, and cancellation

Root/physical change: **requires separate section-80 authorization**. First inject process/FD faults without a cable change: receiver kill, sender kill, half-close, stalled consumer, dropped control writes, one rail FD error, and delayed chunk. Then, only if authorized, test each physical rail disconnect/reconnect.

Pass: bounded explicit error; no new submissions during barrier; old epoch/keys/counters/credits rejected; surviving rail does not silently resume partial records; new global epoch established across all rails; ACKed whole messages are not duplicated; cancellation returns only `CANCELLED`, `TOO_LATE`, or `UNKNOWN`; no management-LAN bulk fallback; ten clean reconnect cycles per promoted failure mode.

## S53-EXP-005 — Authentication, replay, downgrade, and encryption

With disposable test keys, exercise wrong PSK, reflected roles, repeated nonces/transcripts, stale/repeated epochs, topology/UUID mismatch, feature downgrade, modified header/payload/tag, record reordering, implicit-counter mismatch, and key-usage boundary. Run both profiles with RFC test vectors for HMAC/HKDF/ChaCha20-Poly1305.

Pass: no DATA before mutual CONFIRM; every DATA mutation fails HMAC or AEAD before application; all negative cases fail closed without diagnostic oracle; secrets/tags/plaintext are absent from logs; keys are direction/rail separated and zeroized on close; HMAC and AEAD vectors interoperate with an independent implementation; nonce/key pairs are unique in captured metadata (never log key material).

## S53-EXP-006 — Protocol-cost and batching comparison

Prerequisites: EXP-001 through EXP-005 correctness passes and section 73/75 methodology. Compare TCP and USB4STREAM using the identical codec, corpus, profiles and limits on the same kernel. Report logical payload, header, control, tag, syscall-submitted and delivered bytes separately; retain full latency observations or lossless histograms. Measure CRC/HMAC/AEAD cost, credit stalls, control latency, syscalls, CPU cycles and memory.

Pass criteria for USB4STREAM promotion remain the accepted local decision/feasibility gates and section 55/75 statistical plan. Synthetic wire wins do not establish inference improvement.

## Experiment register

| ID | Resolves |
|---|---|
| S53-EXP-001 | parser/codec safety and deterministic wire compatibility |
| S53-EXP-002 | credit, priority, batching and liveness over default TCP carrier |
| S53-EXP-003 | USB4STREAM short-I/O semantics and million-record correctness |
| S53-EXP-004 | cancellation, timeout, dual-rail epoch and recovery behavior |
| S53-EXP-005 | peer authentication, replay/downgrade rejection and optional AEAD |
| S53-EXP-006 | measured protocol overhead and carrier comparison |
| S53-EXP-007 | finite-state epoch, retry, credit-conservation and liveness gate; proposed, not executed |
