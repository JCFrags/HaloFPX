---
type: implementation-milestone-review
status: accept
date: 2026-07-17
lane: L04a
parent_commit: 221edec31c208574bc10f2cf3efe8e0702c3aad2
base_commit: 61f2f2d7bc4955e9bca821095ef69125837133b5
base_tree: 0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd
---

# L04a offline-manifest-parser review

## Verdict

**Accept as the first L04 implementation milestone.** The target-native parser
accepts only the bounded structural subset of the hash-locked deterministic-CBOR
manifest contract. Its sole success status is `structural_only`, which is
explicitly untrusted and cannot create a cache candidate or hit. The parser is
memory-only, has no provider dependency, and is excluded from the default build.

This verdict does not admit HMAC keys, compatibility hashes, object payloads,
profiles, codecs, runtime reads, or persistent writes. Those remain separately
gated work.

## Correctness and security boundary

The parser enforces exact closed-map structure, deterministic shortest CBOR,
fixed digest lengths, bounded rank/object arrays, strict UTF-8 registered IDs,
generation/predecessor consistency, rank ownership, descriptor compatibility,
and duplicate-object rejection. It performs no input-sized allocation, I/O,
resynchronization, quarantine, or logging. A failure offset is only a bounded
detection cursor and is not represented as the first invalid byte.

Synthetic target-owned fixtures cover every-byte truncation, trailing and
oversized input, noncanonical integers, wrong/indefinite types, versions,
algorithms and lengths, malformed UTF-8, semantic contradictions, minimum and
maximum arrays, immutability, and concurrent deterministic parsing. No imported
executable or deferred PF-IR-10 fixture was used.

## Independent adversarial review

The initial review returned `REVISE` on three contract issues: the registered-ID
validator was narrower than the locked UTF-8 CDDL; the parser header inherited
the L03 provider seam for a digest alias; and `error_offset` was described too
strongly. The implementation now has bounded strict UTF-8 validation and edge
tests, a format-local digest type plus local-include allowlist enforcement, and
an accurate detection-cursor contract. The parser target was also made
`EXCLUDE_FROM_ALL`. Re-review returned `ACCEPT` with no remaining correctness,
schema, allocation, exception, concurrency, runtime-linkage, provenance, or
scope blocker.

## Verification

| Check | Result |
|---|---|
| Clean Windows CPU Release build (`build/halofpx-l04a-clean`) | Pass, all configured default targets |
| Explicit excluded parser-library build | Pass |
| Clean HaloFPX CTests | Pass, 6/6 |
| Clean focused inherited CTests | Pass, 7/7 including fixture dependency |
| Baseline/candidate `llama-server --help` | Byte-identical direct capture, 55,691 characters |
| Static offline/no-hook contract | Pass; format header has no local include and production sources have no parser call |
| Immutable reference clones | Clean with refs byte-equivalent to locked records, 4/4 |
| `git diff --check` | Pass; autocrlf notices only |

The local build does not claim HIP, Vulkan, ROCm, target-node, authentication,
compatibility-hash, payload-corruption, state-continuation, crash, or performance
qualification. Feature-off remains the compatibility control.

## Provenance, rollback, and next gate

The implementation and fixtures are target-native. No CachyLLama or GPL
llama-ai implementation or separately licensed documentation entered the MIT
engine; the direct-cherry-pick roster remains empty and no P3 donor unit was
created. Removing this milestone leaves the preceding inert seam unchanged,
and no persistent state exists to migrate or invalidate.

L04b must add independently testable authentication and compatibility/hash
rejection without converting structural success into provider eligibility.
Unknown or revoked keys, bit corruption, replay, wrong domains, missing or
unexpected components, and hostile inputs must remain misses before any runtime
reader can be proposed.
