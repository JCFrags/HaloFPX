---
type: implementation-milestone-review
status: accept
date: 2026-07-18
lane: L05b-deterministic-durability-simulator
parent_commit: b8123fe5518fb769f6c4f6a75c2933bbd658a649
base_commit: 61f2f2d7bc4955e9bca821095ef69125837133b5
base_tree: 0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd
---

# L05b deterministic durability simulator review

## Verdict

**Accept the excluded deterministic simulator slice only.** It remains high-
level, in-memory evidence toward M63-01. It is not M63-01/L05 completion and
opens no concrete writer, persistent setting, server path, deployment, or
durability-mode claim.

The simulator separates live/durable object, manifest, namespace, and anchor
state; applies named faults before or after each coordinator operation; projects
four deterministic namespace/anchor crash outcomes; recovers only the exact
selected old or next anchor; retains unreachable attempt material; and bounds
its trace and object count.

## Independent adversarial review

The first review returned `REVISE` because the 1,344-run matrix did not directly
assert namespace survival, and because old-generation recovery treated the
predecessor as valid without representing or declaring that abstraction. It
also requested post-commit unequal-state recovery tests.

The final implementation adds direct live/durable and garbage assertions around
object/manifest publication and directory sync under both namespace policies;
an explicit prevalidated predecessor-chain control whose invalidation blocks
anchor reads and makes both old/new recovery miss; documentation that this is a
one-bit control rather than historical-byte validation; and post-commit unequal
manifest/object miss tests. Fresh independent rebuild and focused tests passed
3/3. Final re-review returned `ACCEPT` with no remaining scope blocker.

## Verification

| Check | Result |
|---|---|
| Clean Windows CPU Release build | Pass, `build/halofpx-l05b-clean`, WebUI OFF |
| Publication/coordinator/static/simulator focus | Pass, 3/3 |
| HaloFPX CTests | Pass, 13/13 |
| Focused inherited CTests | Pass, 7/7 |
| Core failpoint matrix per process | Pass, 1,344/1,344 |
| Repeated executable processes | Pass, 100/100 |
| Repeated core scenarios | Pass, 134,400/134,400 |
| Repeat executable SHA-256 | `ca3bc2bc6666373e02586b1754027b2e8bec1780657630c0594a23a5217eb091` |
| Maximum fixture | Pass, 128 objects / 777 calls / 1,554 trace entries |
| Independent review | Accept after revise/fix/re-review |
| `git diff --check` | Pass; autocrlf notices only |
| Product/runtime linkage | None; static contract pass |
| Donor code or documentation | None; direct-cherry-pick roster remains empty |

The separate repeat receipt is
[`evidence/l05b-simulator-repeat-receipt.json`](../evidence/l05b-simulator-repeat-receipt.json).
Its elapsed time is not a benchmark and makes no performance claim.

## Limits and next gate

The simulator has no canonical bytes, encoder, authentication, capacity
accounting, short I/O, byte corruption, quarantine record, cleanup, or resource
policy. It does not prove asynchronous late-completion fencing, cancellation,
concurrent stale attempts, authority transfer, coordinator failover,
cross-process ownership, or locks. The backend still lacks attempt identity and
expected-predecessor compare-and-swap authority.

No path, handle, syscall, no-follow/no-replace primitive, filesystem process
restart, reboot, directory durability, SSD power-loss behavior, server/provider
hook, persistent option, or node change exists. Those remain required before
M63-01..03, L05 exit, or any persistence/canary promotion.

Rollback is source-only: revert the simulator, test, enum, CMake, static
contract, evidence receipt, and documentation. No external state was written.
