---
title: "Integrity and silent-corruption handling"
tags: ["integrity", "corruption", "crash-consistency"]
created: 2026-07-17
updated: 2026-07-17
status: design-proposal
sources: ["STORAGE-01", "STORAGE-02", "STORAGE-03", "FUZZ-05"]
related: ["Checkpoint-Commit-Protocol", "Security-Threat-Model", "Fuzzing-and-Fault-Injection"]
---

# Integrity and silent-corruption handling

## Layered integrity

Use different checks for different failure classes:

| Layer | Check | Purpose |
|---|---|---|
| Frame | CRC32C or equivalent over header and frame bytes | detect common transfer/storage damage quickly |
| Immutable object | SHA-256 over canonical header and full plaintext bytes | end-to-end object identity and malicious/random modification detection |
| Manifest | cryptographic digest plus authority MAC/signature | bind object graph, rank set, topology, position, and durability |
| Structure | strict lengths, shapes, offsets, dtype, coordinates, sorted/unique entries | prevent valid bytes from being misinterpreted |
| Semantics | optional canary/replay comparison at selected boundaries | detect wrong-but-self-consistent producer bugs |

TLS protects a connection; it does not detect corruption after TLS termination, in storage, during DMA, or from a buggy producer. At-rest authenticated encryption can add confidentiality and integrity, but the plaintext content digest remains useful for end-to-end verification inside the authorized trust domain.

## Crash-safe local publication

For a local filesystem implementation:

1. create a new temporary file in the destination filesystem;
2. write header and bytes while hashing;
3. `fdatasync`/`fsync` the file and check errors;
4. re-read or otherwise verify persisted length/digest under the storage contract;
5. atomically rename/link with no-replace semantics;
6. `fsync` the containing directory when required to persist the directory entry;
7. only then record the object in the durable rank manifest.

Atomic rename prevents readers from observing a missing replacement during publication, but durability still requires the relevant flushes. Network filesystems and object stores need their own documented conditional-create, read-after-write, and durability semantics; do not assume local POSIX behavior.

## Read-time gate

Before materialization, validate in this order:

1. bounded encoded size and parser limits;
2. canonical schema and required fields;
3. authority certificate/MAC and generation/epoch policy;
4. manifest/object digest chain;
5. exact topology and rank/shard ownership;
6. page length, shape, dtype, alignment, coordinate range, and uniqueness;
7. frame/object checksum from the bytes actually read;
8. optional semantic canary policy.

Verification results are cached only with storage generation/inode or object-version identity; a later mutation or version change invalidates the result.

## Corruption response

- Reject the page/checkpoint with `DATA_LOSS` or a domain-specific corruption code.
- Quarantine the object version and record its source, digest, storage version, reader, and failure type without logging tenant content.
- Mark dependent checkpoints temporarily unreadable; do not mix in an older page at the same coordinate unless an authoritative manifest names it.
- Repair from an independently verified copy or deterministic recomputation.
- Recompute the object digest and publish a new object/version; never overwrite bytes under an existing content digest.
- Trigger background scrub and incident thresholds when failures cluster by host, disk, transport, producer build, or topology.

## Wrong-but-self-consistent bytes

A producer bug can hash incorrect bytes and therefore pass ordinary integrity checks. Detection requires an independent oracle, such as:

- deterministic replay of sampled checkpoint boundaries and comparison of selected KV/logit canaries;
- dual serialization implementations or read-after-deserialize invariants;
- cross-build differential tests;
- tensor range/NaN/Inf checks and model-specific invariants;
- periodic full rebuild and hash comparison under a declared deterministic environment.

These checks are probabilistic or implementation-specific; the protocol must not claim cryptographic hashes prove numerical correctness.

## Scrubbing and retention

Scrub manifests more frequently than cold pages, sample page bodies continuously, and scrub all objects before promoting an asynchronous offload watermark to host-failure durability. Retain enough prior committed checkpoints to recover from latent corruption, subject to privacy and deletion policy. A corrupted newest checkpoint may fall back only to an older independently valid certificate, followed by force-replay of visible tokens.
