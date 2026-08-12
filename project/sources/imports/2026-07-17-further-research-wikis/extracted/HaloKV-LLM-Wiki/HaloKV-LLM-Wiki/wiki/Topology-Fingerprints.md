---
title: "Topology fingerprints"
tags: ["topology", "compatibility", "cache-abi"]
created: 2026-07-17
updated: 2026-07-17
status: design-proposal
sources: ["KV-01", "KV-04", "KV-05"]
related: ["Rank-Local-Cache-Keys", "Reconnect-and-Recovery", "Partial-Rank-Failure"]
---

# Topology fingerprints

## Purpose

A topology fingerprint prevents numerically plausible but semantically wrong cache reuse after model upgrades, re-sharding, kernel/layout changes, or rank relocation. Equality means the implementation has promised exact byte reuse; inequality means reject or invoke a named conversion path.

## Exact-reuse fingerprint

```text
topology_fingerprint = SHA-256(CanonicalTopologyDescriptor)
```

The descriptor should include:

| Domain | Required fields |
|---|---|
| Model | model architecture/config digest, weights Merkle root or immutable revision, adapter set and digests |
| Parallelism | mode (`TP`, `PP`, `EP`, hybrid), world size, logical ranks, layer/head/expert shard mapping, collective group IDs |
| Cache ABI | format version, tensor order, dtype, quantization/calibration, block/page dimensions, shape, strides, alignment, endianness |
| Attention semantics | attention backend ABI, RoPE/scaling, sliding-window policy, masks, prefix-sharing policy, speculative-decoding cache contract |
| Runtime | engine cache-serialization ABI, kernel feature flags that change values/layout, relevant compiler/build revision |
| Input contract | prompt-embedding/multimodal preprocessing contract and tokenizer revision where material |

Physical hostname, IP address, process ID, GPU UUID, and storage path are inventory attributes. Include hardware capability only when serialized bytes or numerical semantics depend on it.

## Two compatibility levels

Use separate digests rather than weakening one fingerprint:

- `exact_reuse_fingerprint`: permits direct page materialization with no transformation.
- `transport_compatibility_fingerprint`: means peers understand the same wire object and can store/forward it, but not necessarily execute it.

A third optional `conversion_contract_id` identifies a reviewed converter from one exact topology to another. Conversion output receives new page IDs and a new exact-reuse fingerprint.

## Canonical descriptor example

```json
{
  "schema": "halokv-topology-v1",
  "model": {
    "config_sha256": "...",
    "weights_merkle_root": "...",
    "adapters": []
  },
  "parallelism": {
    "mode": "TP",
    "world_size": 2,
    "rank_map": [
      {"rank": 0, "heads": "0..15"},
      {"rank": 1, "heads": "16..31"}
    ]
  },
  "cache": {
    "format": "engine-abi-17",
    "dtype": "bf16",
    "page_tokens": 256,
    "layout": "layer-major-kv-head-token-dim"
  },
  "attention": {
    "backend": "backend-x@3",
    "rope": "config-sha256",
    "sliding_window": null
  }
}
```

Arrays with semantic set meaning must be sorted; strings must have a fixed Unicode normalization policy; integers must reject non-canonical representations; floating-point configuration should be represented by canonical strings or exact bit patterns.

## Negotiation and rejection

Handshake advertises protocol ranges, wire formats, and fingerprints. The coordinator never picks “closest” topology. Outcomes are exact match, known conversion, rebuild, or rejection. An epoch cannot override compatibility.

Error classification:

- `FAILED_PRECONDITION/TOPOLOGY_MISMATCH`: authenticated peer is valid but cache is not reusable.
- `UNIMPLEMENTED/CONVERSION_UNAVAILABLE`: a known source/target pair has no converter.
- `DATA_LOSS/FINGERPRINT_OBJECT_MISMATCH`: an object’s embedded fingerprint conflicts with its manifest or digest header.

## Upgrade procedure

Drain or checkpoint at the old topology, publish a new session generation, bump the authoritative epoch, launch the new topology, then either rebuild from tokens or run an offline verified converter. Never allow old and new topologies to mutate one generation concurrently.
