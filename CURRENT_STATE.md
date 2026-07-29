# HaloFPX Current Project State

Verified from Project Lead records: 2026-07-29 09:34 Pacific Daylight Time

This page is a routing summary.
The Project Lead records remain authoritative for active work and production state.

## Current authority

- Documentation repository baseline: `d30814ed08fe395f1bb1d292281ce82edb6bdab4`.
- HaloFPX implementation repository HEAD: `6c88472bf5f567a1064f27f4d8a90fc8e2b47a02`.
- The HaloFPX worktree contains preserved, uncommitted L111 implementation work.
- The documentation task does not own that implementation work.
- The [Project Lead status](project-management/lead/CURRENT_STATUS.md) records the complete current state.
- The [Project Lead decisions](project-management/lead/DECISIONS.md) record accepted boundaries.

## Active project boundary

[VERIFIED] The Project Lead paused internal hidden workers on 2026-07-29.
The user requires user-visible Codex tasks.
The documentation task is active under the retained documentation specification.
[Source: Project Lead decision](project-management/lead/DECISIONS.md#2026-07-29--require-user-visible-worker-tasks)

[VERIFIED] L110 closed `NOT PROMOTED` at
`6c88472bf5f567a1064f27f4d8a90fc8e2b47a02`.
L111 owns only the loader transaction gate described by the Project Lead.
Uncommitted L111 files are not accepted source.
[Source: Project Lead status](project-management/lead/CURRENT_STATUS.md)

## Production state

[MEASURED] On nimo-1 at 2026-07-29 09:00 Pacific Daylight Time, the coordinator
used process identifier (PID) `3027112`.
Its InvocationID was `e6da1fe637144cb394119959c0e88736`.
The service had `NRestarts=0` and one port 8081 listener.
The Hypertext Transfer Protocol (HTTP) result was 200.
[Source: Project Lead status](project-management/lead/CURRENT_STATUS.md)

[MEASURED] On nimo-2 at 2026-07-29 09:00 Pacific Daylight Time, the rank worker used
PID `2148915` and InvocationID `3480c89086e04d5d80060366c5c7ab7f`.
The service had `NRestarts=0` and one port 50052 listener.
[Source: Project Lead status](project-management/lead/CURRENT_STATUS.md)

The documentation task did not recheck or change production.
Read the current Project Lead authority before any authorized transition.

## Product and performance state

[VERIFIED] L101 established exact primary-model cache correctness for the retained
rank-local mechanism.
L101 remained `NOT PROMOTED` because its terminal evidence envelope failed.
L102 corrected that envelope validator and passed at
`b1e21c49606f2ffd2768d0f28766b0007498a6a8`.
[Source: Project Lead status](project-management/lead/CURRENT_STATUS.md)

[VERIFIED] The user-facing distributed server-cache composition epic is paused.
L103 through L108 identified architecture blockers.
The Project Lead has accepted no end-user two-node persistent-cache product.
[Source: Project Lead status](project-management/lead/CURRENT_STATUS.md)

[MEASURED] The accepted Project Lead record contains a matched feature-off primary workload.
The workload measured approximately 203.8 prompt tokens/s and 16.65 generation tokens/s.
No accepted full-model speed improvement exists.
[Source: Project Lead status](project-management/lead/CURRENT_STATUS.md#performance-truth)

## Next safe actions

- Documentation workers must follow [`WORKER_START_HERE.md`](WORKER_START_HERE.md).
- Implementation workers must use the
  [L111 visible-worker specification](project-management/lead/worker-specs/L111_VISIBLE_IMPLEMENTATION_TASK.md).
- No worker may treat uncommitted HaloFPX source as accepted.
- No worker may change production without a current Project Lead decision.
