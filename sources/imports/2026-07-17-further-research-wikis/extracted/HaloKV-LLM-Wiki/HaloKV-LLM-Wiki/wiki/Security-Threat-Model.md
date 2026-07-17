---
title: "Security threat model"
tags: ["security", "threat-model", "rpc"]
created: 2026-07-17
updated: 2026-07-17
status: design-proposal
sources: ["RPC-01", "RPC-07", "SEC-01", "STORAGE-03"]
related: ["Integrity-and-Corruption", "Epochs-Retries-Cancellation", "Fuzzing-and-Fault-Injection"]
---

# Security threat model

## Assets

- tenant prompts, embeddings, multimodal inputs, token history, and KV tensors;
- model/adapter identity and potentially model weights;
- session availability and output continuity;
- epoch/commit authority and signing/MAC keys;
- cache object integrity, topology metadata, and storage locators;
- capacity information and cross-tenant isolation.

## Trust boundaries

1. client/router to coordinator;
2. coordinator to rank processes;
3. rank-to-rank and rank-to-storage data plane;
4. execution hosts to epoch/commit authority;
5. process to local disk/GPU/runtime;
6. tenant namespace to shared cache/storage infrastructure.

Authenticated peers are still treated as potentially stale, compromised, or buggy.

## Threat table

| Threat | Example | Primary controls | Residual risk |
|---|---|---|---|
| Spoofed rank | client asserts `rank_id=1` | mTLS workload identity; server-derived role/rank authorization; bind cert identity to deployment | compromised host credential can impersonate its assigned role until revoked/fenced |
| Stale worker replay | old process sends prepare/commit after failover | durable highest epoch, authority proof, generation, instance boot nonce, terminal op records | authority compromise defeats fencing |
| Cross-tenant cache probing | attacker guesses prompt hash/page ID | tenant-scoped HMAC lookup, namespace authorization, default no cross-tenant dedupe, coarse errors | timing/capacity side channels require deployment hardening |
| Manifest substitution | valid page graph attached to another session/topology | signed/MACed certificate, canonical digest chain, embedded tenant/topology/rank/coordinates | compromised authority or key is catastrophic |
| Parser/resource exhaustion | huge counts, recursive fields, long varints, compression bomb | absolute wire/uncompressed limits, recursion and byte limits, streaming, quotas, pre-allocation checks | CPU asymmetry remains; rate-limit before expensive crypto/decompression |
| Integer/offset abuse | overlap, wraparound, out-of-range chunk | checked arithmetic, sorted non-overlap rules, exact total length, max page size | implementation language unsafe-code bugs |
| Storage locator injection | peer supplies filesystem path or arbitrary URL | opaque server-minted locator tokens; no client paths; allowlisted stores | SSRF/path traversal in locator resolver must be separately tested |
| Corrupt cache injection | malicious peer sends wrong bytes | content digest, frame CRC, structural validation, provenance, quarantine | wrong-but-self-consistent producer requires semantic oracle |
| Commit rollback | old certificate or checkpoint is replayed | monotonic generation/sequence/authority revision, anti-rollback storage, minimum accepted checkpoint | restoring authority backup incorrectly can reintroduce old state |
| Cancellation abuse | repeated cancels starve work | authorization, per-tenant rate limits, idempotent cancel sequence, audit | authorized tenant can still cancel its own availability |
| Information leakage in logs | prompts/page bytes appear in traces | structured metadata-only logs, field allowlist, secret scanning, retention controls | stack dumps/core files require host policy |
| Data theft at rest/in transit | network or disk observer | TLS/mTLS; encryption at rest with tenant/key scope; least privilege | compromised execution process sees plaintext needed for compute |

## Hostile RPC validation

Before allocating based on a request:

- enforce maximum encoded bytes, uncompressed bytes, recursion depth, repeated-field counts, string lengths, manifest entries, page size, chunk size, and in-flight totals;
- reject invalid UTF-8 where strings are used, non-canonical IDs, unknown required enum values, duplicate coordinates, unsorted/overlapping ranges, zero/negative-equivalent dimensions, and arithmetic overflow;
- require fixed lengths for UUIDs, SHA-256 digests, nonces, MACs, and topology fingerprints;
- compare self-asserted tenant/session/rank with authenticated authorization context;
- bind `op_id` to a request digest; reject reuse with different content;
- disable or tightly limit compression on already compressed/cache data and account by declared uncompressed size;
- never dereference client-provided paths or arbitrary URLs;
- perform cheap syntax/quota checks before cryptographic verification, storage calls, or GPU allocation.

## Authentication and authorization

Use TLS with mutual authentication for service-to-service calls. Short-lived workload identities should map to narrowly scoped actions: coordinator, rank for a specific deployment/rank set, storage reader/writer, or operator. Authorize tenant, session, generation, operation kind, and object scope. Credential rotation does not change the protocol epoch, but revoked/stale identities must be denied even if their epoch fields look current.

## Key management

Separate transport keys, certificate/manifest signing or MAC keys, tenant cache-key HMAC keys, and at-rest encryption keys. Include key IDs and rotation generations in metadata without exposing keys. Rotation must preserve read access to retained checkpoints or deliberately invalidate/delete them. Authority signing keys should be hardware- or service-protected and audited.

## Security fail states

Authentication failure is `UNAUTHENTICATED`; authorization failure is `PERMISSION_DENIED`; malformed/resource-abusive requests are `INVALID_ARGUMENT` or `RESOURCE_EXHAUSTED`; stale or topology-incompatible authenticated peers are `FAILED_PRECONDITION`; integrity failure is `DATA_LOSS`. Avoid detailed cross-tenant existence disclosures.

## Out of scope but material

A fully compromised kernel, hypervisor, GPU firmware, authority quorum, or model runtime can violate protocol assumptions. Confidential computing, remote attestation, Byzantine control-plane replication, and side-channel-resistant GPU execution are separate designs.
