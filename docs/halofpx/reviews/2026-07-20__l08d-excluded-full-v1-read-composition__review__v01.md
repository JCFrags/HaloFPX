# L08d excluded full-v1 read composition independent review v01

Verdict: **ACCEPT** for the default-excluded, synthetic L08d milestone. No
P1/P2 finding or code blocker remains.

## Findings

- P3: caller-supplied `context_store_v1_read_only_admission` is a fixture
  allowlist, not trusted product admission authority. The isolated `hit` is
  only an internally verified fixture candidate; it admits no state profile,
  codec, persistent reader, or live restore.
- P3: the provider owns a heap copy of the manifest master key and best-effort
  wipes it on normal destruction. Compiler, allocator, crash, and process-
  remanence limits remain behind existing protected-key gates.
- P3: filesystem safe-open/streaming, protected key and anchor sourcing,
  semantic codec validation, payload zeroization, and product integration must
  remain explicitly deferred.

The ADR and milestone page include all three restrictions.

## Review conclusions

- Authentication metadata remains private and is exposed only after the final
  `authenticated_unadmitted` result. Key IDs, key bytes, and tags are absent.
- The carrier retains all variable manifest and descriptor facts required by
  this seam; fixed v1 version fields are appropriately omitted.
- Lookup exact-matches identity, metadata, descriptor roster and cardinality,
  applies aggregate bounds, and verifies every frame before constructing one
  candidate. A corrupt second object cannot expose a partial candidate.
- Deep-copied input ownership and immutable lookup support concurrent calls.
- The library remains `STATIC EXCLUDE_FROM_ALL`, reports closed capabilities,
  disables publish, and has no filesystem, writer, server, live-state, or donor
  dependency.
- Feature-off behavior and rollback remain unchanged. The implementation is
  target-native and introduces no CachyLLama or GPL llama-ai code, separately
  licensed documentation, dependency, or provenance obligation.
- Windows Release 8/8 and nimo-1 GCC 16 Release 8/8 plus feature-off/L02 2/2
  are proportionate for this isolated seam.

The reviewer ran no extra tests and made no source changes.
