# HaloFPX Current Project State

Reconciled from retained source and Project Lead records: 2026-08-12

This page is a routing summary.
The Project Lead records remain authoritative for active work and production state.

## Current authority

- Remote `main` observed during this correction:
  `e3e8b286b8316abd246fb155044effd11e60b0eb`. The exact source base named by
  the incident record is
  `b77f2bce6e7875ab065e09894f45915585c9f156`. These are dated repository
  boundaries, not live pointers; a fresh clone must record its own
  `git rev-parse HEAD` and `git rev-parse origin/main`.
- Historical documentation repository baseline:
  `d30814ed08fe395f1bb1d292281ce82edb6bdab4`.
- Imported documentation source: `b1c2d8aef707fb03920fc189ccd26395fa61879d`.
- Retained L111 implementation source: `620ef60aa446990335ef46c7d76738f797e62f8f`.
- L111 source and evidence are committed. The implementation commit is the
  direct child of accepted L110 base
  `6c88472bf5f567a1064f27f4d8a90fc8e2b47a02`.
- The [Project Lead status](project-management/lead/CURRENT_STATUS.md) records the complete current state.
- The [Project Lead decisions](project-management/lead/DECISIONS.md) record accepted boundaries.

## Current continuation state

**[VERIFIED]** The cache-saving behavior reference is pinned to
`fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940`.
It is a behavior/reference authority, not evidence that its implementation or
complete feature set has been imported.

**[VERIFIED]** PR #23 merged at
`aee627bd46de21327c9082f7915818430d38f453` and closed issue #14. The
focused hosted qualification starts separate server processes for cold
publication, exact restart hit, compatibility mismatch, and same-size
corruption. Continuation remains exact; incompatible or corrupt state misses
and cold-recomputes. The admitted profile is default-off, world size 1, rank
0, ordinary transformer memory, and greedy memoryless sampling. This does not
accept prefix matching, recurrent/hybrid state, two-rank coordination, or
target-machine performance.

**[VERIFIED]** PR #27 merged at
`bf420e9f1db4ea4ba1d7c87771b6a4d662b5be67`. It adds an optional,
default-off OpenSSL EVP SHA-256 implementation for the separate run-local SSD
prompt-cache files. Exact-length, full-file, digest, corruption, and
feature-off behavior remain qualified. Internal EVP failure reopens the file
and restarts scalar hashing at byte zero. No end-to-end speedup is accepted.

**[VERIFIED]** PR #30 merged at
`7a36e01a25bd5c27b684b489d9996b4de3afa299`. The default-off HIP
specialization removes an activation-block sum only for exact Q2/Q3/Q6/Q8
ROCmFPX MMVQ consumers. Both CachyOS Strix nodes compiled matched feature-on
and feature-off source while production retained its PIDs and zero restart
counts. No GPU correctness, model parity, or performance result is accepted;
issue #25 remains open.

**[VERIFIED]** PR #35 merged at
`167df62ffc8970bc408d72e97ab71a57de4b69d2` and fixes the server's
mixed sampled/raw logits row-count fallback. Issue #28 remains open for a
single coherent output snapshot and synchronization reduction.

**[VERIFIED]** PR #31 merged at
`0ba18151438cb0e7279c7c8ae08e152f6f70145b` and closed issue #16 with the
model-general frozen-plan, schedule, preflight, and raw-evidence core. It is
not execution-qualified; issue #37 owns the CachyOS process adapter.

**[OPEN]** Issues #15, #18, #26, #28, #29, #32, #33, and #37 remain open for
prefill instrumentation, cache metrics, two-rank cache composition, sampling
synchronization, FFN activation reuse, verified longest-prefix reuse,
live-derived cache compatibility, and the CachyOS A/B adapter.

## Retained publication-era project boundary

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

[VERIFIED] The Project Lead paused internal hidden workers on 2026-07-29.
That historical task-control state is preserved for provenance; later owner
direction authorizes the current multi-agent work. Uncommitted L111 files are
not accepted source. The accepted L111 boundary is the exact retained commit
and evidence above.

## Production state

**[MEASURED]** On 2026-08-12, a nimo-2 candidate build ran while the
production worker owned about `114041696 kB` of `gpu_active` HMM pages. Despite
roughly 14 GiB ordinary memory availability, the kernel invoked global OOM
four times and killed production worker PID `2148915`. systemd restarted it as
PID `2248760`, InvocationID `d15fe49610274e77bd9a3d84a0b791a5`,
`NRestarts=1`. A real request then exposed stale coordinator RPC state; the
coordinator restarted as PID `3113343`, InvocationID
`0656332b63a140eab7214627baa43253`, `NRestarts=1`. A 5-prompt-token plus
1-generated-token request subsequently completed and `/health` was OK. The
[incident evidence](../docs/halofpx/evidence/2026-08-12-target-hmm-oom-incident/README.md)
is a safety/recovery record, not a benchmark or performance result. It did not
rehash either recovered executable or its loaded libraries.

**[RECOMMENDATION]** [Issue #41](https://github.com/JCFrags/HaloFPX/issues/41)
is now a P0 prerequisite for target work: refuse builds, quantization,
disposable inference, and benchmarks during protected production or
unaccounted KFD/render/HMM ownership. `MemAvailable` alone cannot admit work.
After either rank changes identity, health alone cannot prove recovery; exact
two-rank authority plus a real minimal inference is required.

The older zero-restart observations below are historical before-state, not
current production authority.

[MEASURED] On nimo-1 at 2026-07-29 09:00 Pacific Daylight Time, the coordinator
used PID `3027112`, InvocationID `e6da1fe637144cb394119959c0e88736`,
`NRestarts=0`, and one port 8081 listener. The HTTP result was 200.

[MEASURED] On nimo-2 at 2026-07-29 09:00 Pacific Daylight Time, the rank worker used
PID `2148915`, InvocationID `3480c89086e04d5d80060366c5c7ab7f`,
`NRestarts=0`, and one port 50052 listener.

**[MEASURED]** At `2026-08-12T23:06:08Z`, a bounded read-only health recheck
found the established comparison services active and running. Nimo-1 retained
PID `3027112`, InvocationID `e6da1fe637144cb394119959c0e88736`,
`NRestarts=0`, its port 8081 listener, and `{"status":"ok"}` from the local
health route. Nimo-2 retained PID `2148915`, InvocationID
`3480c89086e04d5d80060366c5c7ab7f`, `NRestarts=0`, and its port 50052
listener.
[Source: health-only receipt](../docs/halofpx/evidence/2026-08-12-strix-halo-health-recheck/README.md)

This health receipt did not re-audit packages, firmware, unit or launcher
contents, binaries/libraries, model identity, cache contents, or performance.
Read the complete target-machine authority and run a fresh bounded preflight
before an authorized transition.

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
- Keep issue #25 open and the merged PR #30 feature default-off until target
  correctness, model parity, and matched evidence complete. Do not infer a
  performance gain from host contracts or compilation.
- Use issues #15, #18, #26, #28, #29, #32, #33, and #37 as the open
  continuation trackers; preserve cache, cold-prompt, and generation
  measurements as separate lanes.
- Treat the
  [L111 visible-worker specification](project-management/lead/worker-specs/L111_VISIBLE_IMPLEMENTATION_TASK.md)
  as the completed historical contract, not an active implementation handoff.
- Any graph, asynchronous RPC, scheduler, model/runtime, cache-product, or
  performance milestone requires a separate Project Lead decision and focused
  qualification.
- No worker may change production without a current Project Lead decision.
