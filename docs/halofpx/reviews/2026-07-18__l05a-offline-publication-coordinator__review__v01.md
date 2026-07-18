---
type: implementation-milestone-review
status: accept
date: 2026-07-18
lane: L05a-publication-coordinator-slice
parent_commit: ddc13f2dde7ffa16808c76b9d1ce98b5e6201c55
base_commit: 61f2f2d7bc4955e9bca821095ef69125837133b5
base_tree: 0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd
---

# L05a offline publication coordinator review

## Verdict

**Accept only the disabled offline coordinator slice.** It is not M63-01 or
L05 completion and opens no filesystem backend, persistent write, provider,
server, recovery, deployment, or canary gate.

The excluded target proves bounded request validation and synchronous protocol
ordering over an injected backend. It requires exact predecessor identity,
generation old plus one, unchanged authority epoch, verified manifest digest
equality with the next anchor, object-before-manifest-before-anchor order, a
shared in-process root fence, and no durability acknowledgement until anchor
synchronization succeeds.

## Independent adversarial review

The first review returned `REVISE` for five concrete gaps:

1. a failed or throwing anchor replacement could complete late but was reported
   as definitely not applied;
2. authority epoch could be increased without the formal transfer protocol;
3. the coordinator did not compare the verified manifest identity with the
   manifest named by the next anchor;
4. serialization covered one coordinator instance instead of one publication
   root; and
5. the tests omitted those states and mutated only one predecessor field.

The implementation now reports every failed/thrown anchor-replacement attempt
as `anchor_visibility_uncertain`; rejects every authority-epoch change; compares
the backend-returned canonical authenticated-manifest digest before manifest
sync/publication; shares a noncopyable root fence across coordinators; and tests
all predecessor fields, escalation, manifest mismatch, boundary-20 failure and
exception, and two-coordinator exclusion. Independent Release re-review returned
`ACCEPT` with no remaining blocker inside the declared slice.

## Verification

| Check | Result |
|---|---|
| Clean Windows CPU Release build | Pass, `build/halofpx-l05a-clean`, WebUI OFF |
| Publication unit/static contract | Pass, 2/2 |
| HaloFPX CTests | Pass, 12/12 |
| Focused inherited CTests | Pass, 7/7 |
| Exact two-object protocol trace | Pass, 21 ordered calls |
| Every single call-boundary failure | Pass, 21/21 unacknowledged |
| Distinct coordinator/root-fence test | Pass |
| `git diff --check` | Pass; autocrlf notices only |
| Immutable reference clones | Clean with expected HEADs, 4/4 |
| Selected base commit/tree | Exact `61f2f2d7...` / `0a35143f...` |
| Product/runtime linkage | None; static contract pass |
| Donor code or documentation | None; direct-cherry-pick roster remains empty |

An initial parallel full-build retry encountered transient Windows object-file
`Permission denied` errors after an earlier command was forcibly timed out.
No matching build process remained; the same build completed immediately with
four-way parallelism, and all selected tests then passed. This is retained as a
build-orchestration observation, not attributed to HaloFPX source behavior.

## Limits and next gate

The abstract backend is not a concrete writer or durable-filesystem simulator.
This milestone does not prove exact bytes, safe paths, handles, short I/O,
no-follow/no-replace operations, cross-process ownership, stale-attempt or late
completion fencing, crash recovery, ENOSPC/EDQUOT/EIO/read-only handling,
quota/reserve/eviction, observability, directory synchronization, or power-loss
durability.

The next safe M63-01 slice is an offline deterministic durable-filesystem
simulator with separate live/durable namespaces, exact traces, named
before/after failpoints, crash-old/crash-new recovery, unreachable-garbage
retention, and stale-attempt tests. A real writer remains blocked on the frozen
anchor/auth/update, encoder, streaming, resource-policy, and machine-filesystem
gates documented in the milestone record.

Rollback is source-only: revert the excluded target, unit/static tests, and
documentation. No node, model, cache root, runtime option, or deployment was
changed.
