---
section_id: "57"
title: "Compatibility manifest, migration, and rejection design"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["HaloFPX candidate architecture"]
  software_versions: ["fingerprint schema candidate v1"]
  hardware_revisions: ["dual gfx1151 Strix Halo; exact topology pending"]
related_sections: ["15", "39", "43", "47", "48", "56", "58", "59", "61", "63", "65"]
---

<a id="s57-design"></a>
# Compatibility manifest, migration, and rejection design

## Identity hierarchy

**[RECOMMENDATION]** Keep identity compositional and inspectable:

```text
compat_root = SHA-256("halofpx.compat.v1\0" || DCBOR(manifest_without_root))
manifest = envelope + model + tokenizer + prompt + math + numeric + runtime + topology
object_id = SHA-256(object_frame_v1)
```

`object_frame_v1` is the following collision-unambiguous octet sequence; all integers are unsigned big-endian:

```text
magic[8]       = 48 41 4c 4f 4f 42 4a 01  # "HALOOBJ" + version 1
domain_len:u16 || domain:UTF-8
type_len:u16   || object_type:UTF-8
payload_len:u64
payload[payload_len]
```

`domain` is exactly `halofpx.object.v1`; `object_type` is a registered ASCII identifier normalized by schema, not free-form Unicode. Decoders reject invalid UTF-8, NULs, unknown object types, trailing bytes, lengths exceeding configured limits, and non-minimal alternative representations. The compatibility root continues to use the Section 57 deterministic-CBOR profile; it does not reuse this object frame.

The manifest contains full component digests and typed summary fields. An index may key by `compat_root`, but acceptance reads and verifies the stored manifest; it never trusts a directory name. The content/object digest is independent so corruption is distinguishable from incompatibility.

**[RECOMMENDATION]** Publish golden vectors containing the exact preimage bytes and expected SHA-256 for empty, one-byte, maximum admitted, Unicode-rejection, unknown-type, and length-boundary cases. Two independent implementations must reproduce them before this framing is promoted.

<a id="s57-topology"></a>
### Topology identity

```yaml
topology:
  mode: tensor_parallel
  plan_schema: halofpx.plan.v1
  world_size: 2
  ranks:
    - logical_rank: 0
      owners: [{tensor: blk.0.attn_q.weight, axis: 0, begin: 0, end: 2048}]
    - logical_rank: 1
      owners: [{tensor: blk.0.attn_q.weight, axis: 0, begin: 2048, end: 4096}]
  collective_order_digest: sha256:...
  transport_protocol: halofpx.fabric.v1
```

**[RECOMMENDATION]** Ranges are half-open with explicit tensor name, axis, units, shape, dtype, ordering, and logical owner. Rank-local state embeds `logical_rank`, its exact ownership digest, global plan digest, world size, and epoch. Physical host/device mapping is a separate field unless it changes state layout or backend ABI. A two-rank object is never silently relabeled as single-rank state; fallback recomputes or uses an explicitly proven layout adapter.

## Versioning and compatibility rules

| Change | Reader action | Migration policy |
|---|---|---|
| Unknown schema major/canonicalization/hash algorithm | reject before allocation | require a separately pinned decoder/tool |
| Known major, newer minor with unknown noncritical field | preserve field; root still covers it | accept only if schema declares it ignorable and tests prove semantics unchanged |
| Unknown critical field, duplicate key, noncanonical encoding | reject/quarantine malformed manifest | no optimistic repair |
| Model/tokenizer/template/RoPE/quant/KV/topology mismatch | diagnostic miss and recompute | never byte-migrate semantic state |
| Index-only or derived metadata change | rebuild from verified immutable objects | no state reinterpretation |
| Object encoding/hash upgrade | verify old manifest/object with old reader; re-emit new object; verify again | atomic side-by-side publication |
| State schema/ABI change | exact adapter plus continuation-equivalence gate, otherwise recompute | old decoder remains isolated and pinned |

**[RECOMMENDATION]** Versions describe parsing; fingerprints describe exact compatibility. A version match never overrides a component mismatch, and a commit match never overrides a byte/ABI mismatch.

## Migration protocol

1. Freeze an immutable source generation; never migrate an object being written.
2. Select the decoder by stored schema ID/major and validate canonical bytes, length, digest, bounds, and required fields before deserialization.
3. Reconstruct the old compatibility manifest and require its stored root; capture producer binary/commit and migration tool hash.
4. Classify the change using the table above. Semantic identity changes become misses/recomputation, not migration.
5. For an approved adapter, decode to a typed intermediate representation, validate shapes/ranges/ranks, serialize the new schema, and run fixed-prefix continuation equivalence against cold recomputation.
6. Publish a new generation atomically with predecessor, tool, old/new roots, and validation receipt. Preserve the old generation through rollback policy; never overwrite it silently.

## Rejection and diagnostics contract

**[RECOMMENDATION]** Return a machine-readable code plus safe structured detail:

```json
{
  "code": "HALOKV_COMPAT_MISMATCH",
  "component": "topology.ranks[1].owners",
  "expected_digest": "sha256:1a2b...c9d0",
  "observed_digest": "sha256:8e7f...1021",
  "artifact_id": "ckpt:...",
  "action": "MISS_RECOMPUTE",
  "schema": "halofpx.compat.v1"
}
```

Required codes: `MANIFEST_MALFORMED`, `SCHEMA_UNSUPPORTED`, `HASH_UNSUPPORTED`, `OBJECT_CORRUPT`, `COMPAT_MISMATCH`, `TOPOLOGY_MISMATCH`, `RANK_SHARD_MISSING`, `MIGRATION_REQUIRED`, `MIGRATION_FAILED`, and `CANONICAL_COLLISION`. Diagnostics identify the first mismatch and optionally all bounded mismatches; they redact tokens, paths, tenant data, and full templates. Metrics use component/code labels, never high-cardinality full hashes.

**[RECOMMENDATION]** Corruption or canonical collision quarantines. Ordinary valid-but-incompatible state produces a miss/recompute. Distributed startup fails closed if required ranks disagree; it may offer a documented single-node cold path, not partially restore a mismatched rank set.
