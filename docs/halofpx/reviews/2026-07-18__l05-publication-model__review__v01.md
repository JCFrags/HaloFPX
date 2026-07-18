---
type: implementation-milestone-review
status: accept
date: 2026-07-18
lane: L05-P63-00
parent_commit: 214a3432f6df862c1fb81da5cf46aeea65eed092
base_commit: 61f2f2d7bc4955e9bca821095ef69125837133b5
base_tree: 0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd
---

# L05 P63-00 publication-model review

## Verdict

**Accept the formal-model gate.** This opens only implementation of a disabled,
offline, target-native writer and fault harness. Persistent writes, server
integration, and canaries remain closed.

The target-owned model binds exact lineage, generation, manifest and
predecessor digests, policy/key epochs, and authority epoch. It validates the
entire bounded predecessor chain, serializes anchor replacement for the
single-writer root, fences crashed writers and their attempts, rejects
corruption/removal/replay/collision, and models miss/recompute without any
runtime or filesystem linkage.

## Independent adversarial review

The review initially returned `REVISE` for substantive gaps: integer-only
anchors, unreachable EEXIST verification paths, incomplete evidence hashing,
dead-writer attempts, missing manifest removal/recompute, numeric-only
predecessors, cross-lineage identity ambiguity, concurrent pending anchors,
stale Wiki authority, and a replay negative that could take an unrelated
corruption trace.

All findings were corrected. The final replay trace explicitly reaches
`MutateManifestIdentity`; cross-lineage replay has a separate required
counterexample; exact commands and all checker outputs are hash-declared; and
the canonical Wiki and manifest are reconciled. Final independent re-review
returned `ACCEPT` with no remaining formal-semantics, evidence, provenance,
default-off, or gate-interpretation blocker.

## Verification

| Check | Result |
|---|---|
| Final source SHA-256 | `320d294949624469a5c636fc510300f6f558845094139402a6845d765b1c38fe` |
| TLC matrix | Pass, 17/17 expected outcomes |
| TLC state exploration | 44,539,476 generated / 5,968,128 distinct |
| TLC evidence | 159/159 declared artifacts verified; manifest `35069f6bfe387aa15218a8c3040e2dac678030cb7268346be3687bb08058ca7a` |
| Deliberate broken variants | Pass, five exit-12 counterexamples |
| Apalache `v0.57.0` | Typecheck pass and bounded `Safety` pass through length 5 |
| Apalache evidence | 9/9 declared artifacts verified; manifest `cf57171d9cb1df719c9f07a5093cdfab5aa68c43c877d673d3d030c239bc17d6` |
| Clean Windows CPU Release build | Pass, `build/halofpx-l05-clean`, WebUI explicitly OFF |
| HaloFPX CTests | Pass, 10/10 |
| Focused inherited CTests | Pass, 8/8 |
| Canonical Wiki validator | Pass, 86/86 schema-valid |
| Canonical Wiki manifest check | Pass, exact generated manifest |
| Immutable reference clones | Clean with expected HEADs, 4/4 |
| Selected base commit/tree | Exact `61f2f2d7...` / `0a35143f...` |
| `git diff --check` | Pass; autocrlf notices only |

The first clean build attempt inherited the upstream WebUI default and failed
when network provisioning could not assemble complete assets. The clean build
was reconfigured with `LLAMA_BUILD_WEBUI=OFF`, matching the locked HaloFPX
decision, and then completed. No WebUI provenance or enablement gate was opened.

## Limits and next gate

P63-00 is finite abstraction evidence, not proof of C++ conformance,
cryptography, path safety, filesystem atomicity/synchronization, device or
power-loss durability, or performance. The newly authorized offline writer
harness must still pass crash injection at every concrete publication boundary;
ENOSPC, EDQUOT, EIO, read-only and sync-failure tests; quota/reserve/eviction;
observability and administrative controls; rollback; and M63-01..03 before L05
exit or any persistence/server/canary promotion.

Rollback is source-only: reverting this milestone removes formal artifacts and
documentation. It cannot affect a cache root because no writer, filesystem I/O,
runtime hook, node deployment, or persistent configuration exists.
