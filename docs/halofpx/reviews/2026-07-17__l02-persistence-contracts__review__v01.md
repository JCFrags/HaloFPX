---
type: implementation-milestone-review
status: accept
date: 2026-07-17
lane: L02
parent_commit: 85dac1a878cbd655af9ce8f1dd3bf4bab4422e0a
base_commit: 61f2f2d7bc4955e9bca821095ef69125837133b5
base_tree: 0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd
---

# L02 persistence-contract review

## Verdict

**Accept L02 and permit L03 default-off interface/provider seams.** The state,
scope, format, publication, replay, threat, and distributed ownership contracts
are closed conservatively. No codec, persistent reader/writer, donor source, or
runtime feature is admitted or enabled.

Persistent provider implementation remains gated. In particular, the first
writer still requires the reviewed TLA+/TLC model, format golden vectors from
two encoders, authenticated key operations, filesystem-specific no-replace and
durability proof, quotas/reserve/eviction, fault tests, rollback, and
administrative controls.

## Scope and evidence

The review covers the five ADRs, closed CDDL registry, machine-readable
default-off contract, CMake contract test, L01 compatibility control, accepted
v03 plan, readiness decisions, canonical Wiki Sections 57-64 and 71, source
locks, capability-level provenance review, and Agent Harness review routing.

The reviewed CDDL SHA-256 is
`de3585ab8531127198214f76bda4ca6ff276c576c6d269a20174654a0e1cdaa8`.
The CMake test locks this exact value plus the critical JSON invariants.

## Independent adversarial review

An independently delegated read-only review initially returned `REVISE` with
eight findings:

1. authenticate manifests, not only hash them;
2. define a closed canonical manifest and compatibility-component schema;
3. prohibit replacement at object and manifest publication;
4. define per-lineage replay authority and anchor crash ordering;
5. canonically frame the namespace HMAC preimage;
6. authenticate and type distributed `READY`/`COMMIT`/`ABORT` control;
7. require universal token/position/output/compatibility/ownership state; and
8. assert security-critical contract values and exact schema drift.

Three revision passes resolved every finding. The final independent verdict was
`ACCEPT`, with no remaining correctness, security, provenance, default-off,
publication, replay, distributed-control, fallback, or test-contract blocker.
The reviewer made no file edits.

## Safety and provenance disposition

- Feature off remains the only enabled behavior; reads/writes and the writer
  gate are false/closed.
- The state profile and codec rosters are empty.
- Anonymous, cross-principal, shared, and implicit-fallback reuse are disabled.
- Authenticated manifests cover their complete body and authentication header;
  strong object hashes remain corruption evidence.
- Parse/authenticate/authorize/validate completes before referenced payload
  decode or live-context mutation.
- Publication is one-writer, synchronize-before-visibility, no-replace, and
  selected by a protected per-lineage anchor.
- Rank messages require an admitted mutually authenticated protected channel
  plus exact full-message authentication; the current RPC path cannot authorize
  publication.
- Donor formats remain offline read-only inventory inputs only. No CachyLLama
  or GPL llama-ai implementation/documentation entered the MIT engine, no P3
  unit was promoted, and the direct-cherry-pick roster remains empty.

## Verification

| Check | Result |
|---|---|
| Direct L02 CMake contract | Pass |
| Registered L01 + L02 HaloFPX CTests | Pass, 2/2 |
| Focused inherited CTests | Pass, 7/7 including fixture dependency |
| `git diff --check` | Pass; autocrlf notices only |
| Reference-clone worktrees | All four clean |
| Reference-clone status/refs | Exact match to source-lock records |
| Configured implementation remotes | None |

The local Windows CPU tests do not claim HIP, Vulkan, ROCm, target-node,
filesystem-durability, cryptographic implementation, or performance
qualification.

## Wiki reconciliation and reusable improvement

The implementation contract narrows the Wiki candidate design in fail-closed
directions: authenticated manifests supplement SHA-256, compatibility
components have exact canonical preimages, publication cannot clobber existing
objects, replay authority is independent per lineage, and distributed control
cannot trust unauthenticated RPC. It deliberately leaves page size, segment
layout, compaction, DAG policy, async I/O, codec admission, and durability-mode
enablement to later gated evidence.

The review itself was rechecked against its evidence and does not promote local
test results into target or release claims. The reusable improvement is the
hash-locked closed schema plus executable invariant test: later edits must
change the reviewed schema digest intentionally instead of silently weakening
the contract.
