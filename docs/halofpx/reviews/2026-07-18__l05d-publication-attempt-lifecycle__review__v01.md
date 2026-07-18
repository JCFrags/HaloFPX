---
type: implementation-milestone-review
status: accept
date: 2026-07-18
lane: L05d-publication-attempt-lifecycle
parent_commit: 3ae385d30441eef6ebf65322fd7bab87fd03901c
base_commit: 61f2f2d7bc4955e9bca821095ef69125837133b5
base_tree: 0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd
---

# L05d publication attempt lifecycle review

## Verdict

**Accept the disabled synchronous attempt-lifecycle slice only.** Every backend
operation now carries one exact attempt identity; definite pre-anchor failures
are abandoned; ambiguity fences the root; replayed terminal IDs are rejected;
and acknowledgement follows durable close rather than anchor sync alone.

## Independent adversarial review

The first review returned `REVISE`: a fault before begin registration was
reported uncertain but the simulator did not actually fence its root. The
coordinator also ignored the fencing result, and ambiguous abandonment lacked a
simulator restart test.

Uncertainty fencing is now a required backend state transition that can create
a root-uncertain state without a preexisting active attempt. The coordinator
records `attempt_fence_confirmed`; false explicitly requires external root
quarantine. Focused before/after-begin and ambiguous-abandonment tests clear the
fault and prove all fresh IDs remain blocked. Final re-review returned `ACCEPT`.

## Verification

| Check | Result |
|---|---|
| Clean Windows CPU Release build | Pass, `build/halofpx-l05d-clean`, HIP/Vulkan/WebUI OFF |
| Focused publication/static/simulator suite | Pass, 3/3 |
| HaloFPX CTests | Pass, 13/13 |
| Focused inherited CTests | Pass, 7/7 |
| Core matrix per simulator process | Pass, 1,472/1,472 |
| Coordinator repeated processes | Pass, 100/100 |
| Simulator repeated processes | Pass, 100/100 |
| Repeated core matrix | Pass, 147,200/147,200 |
| Independent review | Accept after revise/fix/re-review |
| Product/runtime linkage | None; static contract passed |
| Donor code or documentation | None; direct-cherry-pick roster remains empty |

Repeat hashes and counts are retained in
[`evidence/l05d-attempt-lifecycle-repeat-receipt.json`](../evidence/l05d-attempt-lifecycle-repeat-receipt.json).
Elapsed time is not a benchmark.

## Limits and rollback

The registry and faults are in-memory synchronous abstractions. No persistent
journal or replay history, authenticated authority, cross-process ownership,
real asynchronous cancellation, reconciliation, authority transfer, filesystem
behavior, capacity policy, server wiring, node, canary, or durability claim is
opened.

Rollback is source-only: revert the lifecycle API, simulator state, tests, ADR,
evidence, and this review. No external state was written.
