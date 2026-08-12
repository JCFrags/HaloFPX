# HaloFPX Current Project State

Reconciled from retained source and Project Lead records: 2026-08-12

This page is a routing summary.
The Project Lead records remain authoritative for active work and production state.

## Current authority

- Imported documentation source: `b1c2d8aef707fb03920fc189ccd26395fa61879d`.
- Retained L111 implementation source: `620ef60aa446990335ef46c7d76738f797e62f8f`.
- L111 source and evidence are committed. The implementation commit is the
  direct child of accepted L110 base
  `6c88472bf5f567a1064f27f4d8a90fc8e2b47a02`.
- The [Project Lead status](project-management/lead/CURRENT_STATUS.md) records the complete current state.
- The [Project Lead decisions](project-management/lead/DECISIONS.md) record accepted boundaries.

## Active project boundary

[VERIFIED] L110 closed `NOT PROMOTED` at
`6c88472bf5f567a1064f27f4d8a90fc8e2b47a02`.
L111 then closed `PASS / RETAIN` at
`620ef60aa446990335ef46c7d76738f797e62f8f`.
The accepted result is only the bounded loader foundation. Release and Debug
focused CTest passed `1/1`; the feature-off static `llama` target compiled; and
independent review returned `PASS / RETAIN` with no P0 or P1 finding.
[Source: Project Lead decision](project-management/lead/DECISIONS.md#2026-08-12--accept-the-bounded-l111-loader-foundation)

[VERIFIED] L111 did not wire a graph, add asynchronous RPC, change scheduling,
run a model, touch production, restart cache integration, or make a performance
claim. Its feature-off evidence is compile-level rather than a real MiniMax
runtime-parity result.
[Source: L111 evidence](../docs/halofpx/evidence/l111/README.md)

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

[OPEN] Production was not rechecked during the 2026-08-12 source-only
reconciliation. The two measurements above are historical 2026-07-29
observations, not a current health claim. Read current machine authority before
any authorized transition.

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
- Treat the
  [L111 visible-worker specification](project-management/lead/worker-specs/L111_VISIBLE_IMPLEMENTATION_TASK.md)
  as the completed historical contract, not an active implementation handoff.
- Any graph, asynchronous RPC, scheduler, model/runtime, cache-product, or
  performance milestone requires a separate Project Lead decision and focused
  qualification.
- No worker may change production without a current Project Lead decision.
