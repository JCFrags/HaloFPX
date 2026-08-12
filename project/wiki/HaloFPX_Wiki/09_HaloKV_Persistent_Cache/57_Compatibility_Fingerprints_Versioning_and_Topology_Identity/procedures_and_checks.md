---
section_id: "57"
title: "Fingerprint validation and migration checks"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["future HaloFPX integration tree"]
  software_versions: ["fingerprint schema candidate v1"]
  hardware_revisions: ["two gfx1151 Strix Halo hosts; exact inventories pending"]
related_sections: ["15", "26", "39", "48", "56", "58", "61", "63", "65", "78"]
---

<a id="s57-procedures"></a>
# Fingerprint validation and migration checks

**[RECOMMENDATION]** Treat all commands as design targets until a HaloFPX manifest tool exists. Ordinary inventory/tests require no root. Filesystem/device fault injection may require a disposable test mount and explicit operator authorization; never use a production cache.

## P57-01 — Deterministic manifest generation

1. On both hosts, record exact model shard paths, byte lengths and `sha256sum`; dump typed GGUF metadata/tensor inventories without converting values to display strings.
2. Capture the effective tokenizer, special-token policy, external sidecars, effective chat template after CLI/API override, resolved context/RoPE values, K/V types, and state flags.
3. Record full commits, submodules, dirty patch digest, build options, binary/shared-library hashes, backend ABI/protocol, GPU target, and kernel/shader bundle digests.
4. Export the committed topology plan with rank count, logical rank, half-open owner ranges, collective order, and transport protocol.
5. Encode twice with independent process invocations; require byte-identical deterministic CBOR and equal component/root SHA-256. Decode/re-encode and require byte equality.

Acceptance: both matched ranks produce equal global components and plan root; only explicitly rank-local ownership fields differ. A second conforming encoder should reproduce the bytes before schema approval.

## P57-02 — Single-field mutation matrix

Mutate one item at a time: model byte; GGUF type/value/array order; tensor type/shape; tokenizer byte/merge/special ID; template byte; architecture/dimension; each resolved RoPE parameter; K/V type; quant ID/layout; backend ABI/binary; state schema/flags; commit/dirty patch; world size; mode; plan; logical rank; shard axis/begin/end/order.

Acceptance: every semantic mutation changes its component and root digest, produces the expected mismatch path/code, and restores no state. Restore falls back to cold recomputation or an explicit pre-execution refusal. No mutation may become a warning-only hit.

## P57-03 — Corruption and canonicalization

- Flip/truncate/append/swap object bytes; alter lengths and digests; reorder manifest keys; inject duplicate keys, indefinite lengths, overlong integers, unknown critical fields, NaN/Infinity, and excessive nesting/counts.
- Present an equal root string with a different canonical manifest to exercise the collision/canonicalizer-fault path without claiming a real SHA-256 collision.
- Fuzz the decoder before allocation limits, then run ASan/UBSan CPU lanes where supported.

Acceptance: corrupt/malformed objects are bounded, rejected and quarantined; the simulated equal-root/different-manifest case is fatal and never accepted. Valid incompatible objects remain misses rather than corruption.

## P57-04 — Upgrade, rollback, and migration

Build fixtures for every supported old schema. Verify old digest with the pinned old reader, migrate side-by-side, validate the new digest, and compare resumed logits/tokens/state position against cold recomputation under the exact-continuation criterion owned by section 78. Crash before/after every publication boundary. Roll back the runtime and prove the preserved old generation still works or is diagnostically rejected.

Acceptance: no in-place overwrite; migration provenance is complete; a failed adapter leaves the source intact; model/tokenizer/template/RoPE/topology changes recompute rather than migrate.

## P57-05 — Two-host topology and degraded mode

Test rank swap, missing rank, duplicate logical rank, changed world size, owner gap/overlap, boundary off-by-one, plan reorder, link/protocol change, backend mismatch, and stale epoch. Confirm each rank validates the global plan plus its local owner digest before allocation/restore. Then request documented single-node fallback.

Acceptance: no partial rank set is consumed; all ranks agree on refusal; allocations are released; fallback uses a distinct single-node topology root and cold or explicitly compatible state.

## P57-06 — Cost and observability

On both hosts measure manifest generation latency, streaming hash throughput, startup overhead, manifest/object size, lookup latency, and diagnostic cardinality for representative model sizes. Preserve raw commands, binaries, environment, repetitions, and results under `experiments/`; do not select an optional faster hash until correctness and overhead are measured.

## Internet follow-up

- Re-audit selected integration commits immediately before baseline freeze; the observed heads are evidence pins, not approval.
- Pin the final canonical-CBOR implementation and review its RFC 8949 conformance/limits.
- Review cryptographic policy when FIPS 180-4 is superseded, and record the algorithm transition plan rather than following “latest” silently.
- Trace every backend/state serialization field in the final fork and map it to a manifest field or documented non-invalidator.
