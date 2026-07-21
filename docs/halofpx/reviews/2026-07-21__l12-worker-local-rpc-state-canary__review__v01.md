# L12 worker-local RPC state canary independent adversarial review

Date: 2026-07-21

Scope: read-only adversarial review of ADR-0040 and the Linux-only,
compile/runtime-default-off worker-local RPC state implementation based on
`78a102ac3212e4987486761983c336438cc3e7c0`.

Verdict: **ACCEPT — no blocking findings remain**

## Reviewed authority and claims

The review compared the implementation against the current RPC command/server
architecture, llama on-device sequence-storage path, L11 ownership blocker,
ADR-0039, ADR-0005, and ADR-0040. It examined framing and allocation limits,
HMAC and transcript binding, replay and timeout behavior, exact identity and
object selection, filesystem authority, descriptor/range validation,
validation-before-live-mutation, coordinator-local state, cold fallback,
feature-off behavior, and the focused qualification harnesses.

## Blocking findings reconciled

1. Feature-off linking initially left unconditional llama wrapper references
   to the optional RPC implementation. The wrappers are now compile-gated, and
   a true `GGML_RPC=OFF` / local-state-off Linux `llama` build passes.
2. Framing initially bounded the request only after vector allocation. The new
   command family now checks the declared length before resizing.
3. Untrusted tensor dimensions, strides, and range length could reach ggml
   constructors/backend reads. The parser now validates type, block
   divisibility, nonzero dimensions, canonical strides, overflow, logical
   extent, registry bounds, overlap, and aggregate caps first. Malformed shape
   and over-range probes reject without terminating the worker.
4. The canary initially reused visible fixed nonces and key material. Stage
   nonces are now kernel-random, a process-lifetime 4,096-entry ledger rejects
   replay or fails closed when full, and the two-host canary reads a freshly
   generated protected key/channel file shared with its disposable worker.
5. Responses initially lacked complete request and status-semantic binding.
   Every state-operation response now HMAC-binds the full authenticated request
   digest, while the client enforces exact status-specific component counts,
   byte counts, object digest, and worker-nonce rules. Commit/apply echoes the
   expected object and worker nonce.
6. Coordinator-local control/staging artifacts were initially raw vectors.
   A fixed 504-byte HMAC receipt now binds their content digests, token data,
   expected worker object, and the complete stable exact-key identity. Receipt
   or content failure cold-recomputes before worker stage. Hashing is not
   limited by the wire-request cap and every hash/HMAC failure propagates.
7. The component-manifest identity was initially a label constant. The adapter
   now hashes the actual canonical kinds, types, dimensions, strides, labels,
   offsets, and lengths, then verifies that digest before capture, stage, and
   commit.
8. A pending READY initially survived referenced-buffer destruction or legacy
   mutation. Before dispatch, allocation, free, clear, set/hash, copy, graph
   compute/recompute, and tensor initialization now discard pending state. A
   destroy/reallocate-before-commit probe returns authenticated `REJECTED` and
   leaves the worker active.

## Accepted limits

- The nonce ledger is process-local and permanently fails closed after 4,096
  accepted stages; restart is required to admit more attempts.
- The five-second timer is exactly READY-to-COMMIT, not a network receive or
  large-object validation deadline.
- Capability discovery is unauthenticated, discloses only exact version/limits,
  grants no state authority, and can only induce fail-closed denial.
- The coordinator canary writer is receipt-last rather than crash-atomic and
  remains a disposable qualification adapter, not a production store.
- Live apply is not crash-atomic. Any commit/apply uncertainty destroys the
  disposable context and cold-recomputes; no partially mutated context is
  reused.
- The review approves only the small two-rank attention-KV canary boundary. It
  does not admit the 160 GB model, production enablement, eviction, shared
  reuse, broad fault injection, cable faults, or performance claims.

No donor, GPL, CachyLlama, reference-clone, dependency, remote, or production
service change was part of the reviewed implementation.
