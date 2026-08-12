---
title: "Coordinated checkpoint commit protocol"
tags: ["checkpoint", "two-phase-commit", "durability"]
created: 2026-07-17
updated: 2026-07-17
status: design-proposal
sources: ["COORD-01", "COORD-02", "STORAGE-01", "STORAGE-02"]
related: ["Protocol-Overview", "Epochs-Retries-Cancellation", "Integrity-and-Corruption"]
---

# Coordinated checkpoint commit protocol

## Commit object model

A checkpoint operation produces immutable rank manifests, then an immutable global manifest, then one small authoritative commit certificate. The certificate is the only commit point.

```text
CheckpointName = (tenant, session, generation, checkpoint_seq)
CertificateKey = CheckpointName
CertificateValue = (epoch, op_id, global_manifest_id, durability_class,
                    authority_revision, committed_at, certificate_mac)
```

Publishing uses compare-and-swap against the expected prior checkpoint and operation state. A conflicting value is not overwritten.

## State machine

| State | Meaning | Durable? | Legal exits |
|---|---|---:|---|
| `ABSENT` | No operation record | no | `OPEN` |
| `OPEN` | Begin accepted under current epoch | authority record | `PREPARED_PARTIAL`, `ABORTED` |
| `PREPARED_PARTIAL` | One or both rank preparations recorded | rank objects durable at declared local class | `COMMITTED`, `ABORTED` |
| `COMMITTED` | Certificate names global manifest | yes | terminal |
| `ABORTED` | Abort tombstone fences late work | yes | terminal |

A rank’s local state may be `SNAPSHOTTING` or `PREPARED`; these are not global states.

## Prepare procedure per rank

At a coherent barrier:

1. Freeze the checkpoint view and record logical position.
2. Split changed cache state into immutable pages; reuse existing page IDs where valid.
3. Write new bytes to a fresh temporary file/object while computing fast frame checksum and cryptographic page digest.
4. Verify length, header, coordinates, and digest from the persisted bytes.
5. Flush file data and required metadata; atomically publish with no-replace semantics; flush the containing directory where local filesystem semantics require it.
6. Build and durably publish the immutable rank manifest.
7. For `HOST_FAILURE_DURABLE`, ensure the manifest and every newly referenced page are independently durable off the originating host.
8. Send `RankPrepared` with rank manifest ID, logical position, byte/page counts, and a bounded opaque durability proof.

If any write, flush, verification, or quota check fails, the rank reports failure and does not claim `Prepared`.

## Coordinator validation

Before building the global manifest, require:

- exactly one preparation from each expected logical rank;
- identical tenant/session/generation/epoch/checkpoint/op identity;
- identical exact topology fingerprint and logical position;
- non-overlapping and complete shard descriptors for the declared topology;
- rank-manifest digests and durability proofs valid under policy;
- no authoritative cancellation/abort record;
- coordinator still owns the authoritative epoch/fencing token.

The global manifest records the expected rank set explicitly. A future deployment with a different world size uses a new generation/topology and schema policy; it does not infer rank count from received messages.

## Publication and ambiguity

The coordinator conditionally publishes the certificate. Three outcomes matter:

- CAS succeeds: committed, even if the response or later notice is lost.
- CAS returns an existing identical certificate: idempotent success.
- CAS times out or connection fails: outcome is ambiguous; query by checkpoint name and `op_id` before retrying.

An existing conflicting terminal record is returned as `ABORTED`, `ALREADY_EXISTS`, or `FAILED_PRECONDITION` with the authoritative identity. Never issue a second checkpoint sequence to “work around” ambiguity until the first is resolved.

## Cancellation race

Cancellation can transition `OPEN` or `PREPARED_PARTIAL` to `ABORTED`. Once certificate CAS linearizes, cancellation returns `ALREADY_COMMITTED`; it cannot uncommit. The authority serializes cancel and commit on the same operation record.

## Orphans and garbage collection

Prepared pages and manifests may outlive an aborted operation or coordinator crash. They are safe because immutability prevents reinterpretation. Garbage collection:

1. traces references from retained certificates and pinned manifests;
2. observes a minimum orphan grace period exceeding retry/reconnect windows;
3. checks operation terminal state and epoch;
4. deletes only unreferenced objects;
5. remains idempotent and quota-bounded.

Content-addressed pages shared by other retained checkpoints are not deleted.

## Why not two-node consensus

A two-member majority is two. Losing either member or partitioning them removes the ability to safely choose a new writer. Embedding commit authority only in the two execution nodes therefore gives no fault-tolerant epoch service. A designated primary without an external fence may be acceptable only when all uncertainty causes a hard stop and recovery is operator-controlled; it is not an available failover design.
