---
title: "HaloKV: two-node persistent KV-cache protocol"
tags: ["halokv", "distributed-systems", "llm-serving", "home"]
created: 2026-07-17
updated: 2026-07-17
status: design-proposal
sources: ["WIKI-01", "KV-01", "KV-03", "COORD-01"]
related: ["Executive-Summary", "Protocol-Overview", "Degraded-Mode-Behavior", "References"]
---

# HaloKV: two-node persistent KV-cache protocol

> [!danger] Hard limit
> Two active model-parallel ranks are not a fault-tolerant compute quorum. When one rank disappears, the surviving rank must stop generation unless it can independently run the entire model and reconstruct or obtain the entire logical cache.

## What this wiki answers

This wiki defines cache identity, topology compatibility, coordinated commits, epoch fencing, idempotent retries, cancellation races, byte-credit backpressure, reconnect negotiation, partial-rank failure handling, corruption detection, hostile-input defenses, delta recovery, formal modeling, and fuzzing.

The protocol’s central abstraction is a **committed checkpoint**:

```text
CommitCertificate
  -> GlobalManifest(session, generation, epoch, checkpoint_seq, topology_fp)
       -> RankManifest(rank 0, logical_position, page IDs...)
       -> RankManifest(rank 1, logical_position, page IDs...)
            -> immutable content-addressed pages
```

No reader may materialize a checkpoint unless the certificate is authoritative, every expected rank is present, all ranks agree on the logical position and topology fingerprint, and every used object passes integrity and structural validation.

## Reading order

1. [[Executive-Summary]]
2. [[System-Model]]
3. [[Protocol-Overview]]
4. [[Rank-Local-Cache-Keys]] and [[Topology-Fingerprints]]
5. [[Checkpoint-Commit-Protocol]]
6. [[Epochs-Retries-Cancellation]] and [[Backpressure-and-Flow-Control]]
7. [[Reconnect-and-Recovery]] and [[Partial-Rank-Failure]]
8. [[Integrity-and-Corruption]], [[Security-Threat-Model]], and [[Degraded-Mode-Behavior]]
9. [[Formal-Modeling]] and [[Fuzzing-and-Fault-Injection]]

See [[References]] for the research catalog and source notes.
