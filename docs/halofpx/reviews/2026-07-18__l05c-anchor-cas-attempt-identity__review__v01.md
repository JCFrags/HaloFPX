---
type: implementation-milestone-review
status: accept
date: 2026-07-18
lane: L05c-anchor-cas-attempt-identity
parent_commit: 4366e4935433a28566fcb5bf2208670b5c1909de
base_commit: 61f2f2d7bc4955e9bca821095ef69125837133b5
base_tree: 0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd
---

# L05c anchor CAS and attempt identity review

## Verdict

**Accept the excluded exact-anchor compare-and-swap slice only.** It supplies a
nonzero attempt identity and the full expected predecessor to the final anchor
linearization primitive. It does not open persistent writing or claim complete
asynchronous attempt fencing.

## Independent adversarial review

The first review returned `REVISE` for two concrete issues. An injected stale
result after the simulator had already changed its live anchor could be reported
as definitely not applied, and the interleaving test changed only generation.

The simulator now translates post-linearization injected stale to interrupted,
which the coordinator reports as visibility-uncertain. A regression proves that
the live anchor may be new while acknowledgement remains forbidden. The unit
test now mutates each of the nine full predecessor identity fields independently
after the initial read and before final CAS. Every case is conclusively stale,
does not replace the mutated anchor, and cannot acknowledge durability. Final
independent re-review returned `ACCEPT`.

## Verification

| Check | Result |
|---|---|
| Clean Windows CPU Release build | Pass, `build/halofpx-l05c-clean`, HIP/Vulkan/WebUI OFF |
| Focused publication/static/simulator suite | Pass, 3/3 |
| HaloFPX CTests | Pass, 13/13 |
| Focused inherited CTests | Pass, 7/7 |
| Coordinator repeated processes | Pass, 100/100 |
| Simulator repeated processes | Pass, 100/100 |
| Repeated core failpoint scenarios | Pass, 134,400/134,400 |
| Independent review | Accept after revise/fix/re-review |
| Product/runtime linkage | None; static contract passed |
| Donor code or documentation | None; direct-cherry-pick roster remains empty |

The first clean configuration attempt used the obsolete
`LLAMA_SERVER_WEBUI=OFF` name. L05c targets built, but the default build later
failed while provisioning inherited WebUI assets. The same fresh directory was
reconfigured with this base's correct `LLAMA_BUILD_WEBUI=OFF` option and the
complete Release build passed. This is retained as a harness/configuration
mistake and is not attributed to product behavior.

Repeat hashes and counts are retained in
[`evidence/l05c-anchor-cas-repeat-receipt.json`](../evidence/l05c-anchor-cas-repeat-receipt.json).
Elapsed time is explicitly not a benchmark.

## Limits and rollback

Authenticated attempt registration, persistent journals, per-operation token
checks, cancellation, late asynchronous completion fencing, process ownership,
authority transfer, real filesystem CAS and durability, protected anchor bytes,
capacity policy, server wiring, nodes, and canaries remain closed.

Rollback is source-only: revert the request/backend contract extension,
simulator behavior, tests, ADR, evidence, and this review. No external state was
written.
