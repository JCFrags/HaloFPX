# L50 ROCm device admission and stories requalification

Status: **NOT PROMOTED**
Base: `e606f62cb19063ceb7bfdbe9dff979ea0544abf0`

## Scope and source finding

L50 remained no-primary and no-production. The recovered L49 journal is retained
verbatim and binds unit `halofpx-l48-worker-capture.service`, invocation
`50c47e1d94d34eafbc4997d96c05a9b6`, PID `2399711`, and the
2026-07-27 09:23:44 PDT exit. It records `unknown device: ROCm0`,
`available devices: No devices found`, and CPU-only inventory.

Exact reconstruction found that the nimo-1 worker had been configured with
`-DGGML_HIP=OFF`; the nimo-2 build used HIP. L50 changed only the rejected
candidate's build and admission authority to require `-DGGML_HIP=ON`,
`-DAMDGPU_TARGETS=gfx1151`, the frozen CMake/toolchain/dependency identities,
and a real no-model ROCm0 application gate.

## Pre-runtime qualification

- Focused tests: 54/54 PASS.
- Independent pre-runtime review: GO for one stories15M session.
- Worker binary SHA-256:
  `7a8fb0496486cc12746ec31f7ed3eb32ba6d9b450948bb2fc39dd7014e194b10`.
- CMakeCache SHA-256:
  `adca9151785db6dcc0c15d42ac7d82e6b9a8d3c050ef4009a31074480194884d`.
- Dynamic-dependency authority SHA-256:
  `74413384b81b793117f7c9ab3b598d5923502a0366f6ff4f6d526e64e00f606f`.
- Device inventory SHA-256:
  `27c70d2186271e068e42ea720b1f78d76d1ba805605676abd3e0df51a1243951`.
- The real isolated gate admitted HELLO/HFXCAP2 and authenticated
  `ROCm0` / `gfx1151`; its qualification invocation was
  `c364aa338bb747d7917a9ba6496aaa6e`.
- The frozen 120-second inner / 150-second outer readiness authority was
  unchanged.

## Sole stories15M session

The runtime gate again admitted the exact ROCm0/gfx1151 binary:

- gate invocation `c1ae540d167540cdb7a93a7a8c0434e0`;
- gate PID `2407529`;
- authenticated device receipt backend `ROCm`, device `ROCm0`, gfx `gfx1151`;
- capture worker invocation `a35a9642694244dca62c3886977131be`,
  PID `2407701`, with HELLO/HFXCAP2 and placement admission.

The capture canary then exited status 4 during warmup. The bounded model-session
record contains:

- `ggml_backend_sched_graph_compute_async failed with error -1`;
- `process_ubatch: failed to compute graph, compute status: -1`;
- `llama_decode: failed to decode, ret = -3`.

No prompt capture, restored token, composed L40/L42/L44 result, or acceptance
claim exists. L50 made no repeat and does not infer a cause beyond those
retained observations.

The child also failed its immediate evidence gate with
`canary halofpx-l48-canary-capture InvocationID is unavailable`. Closeout
recovered exact systemd authority before reset:
invocation `a1b165d2401b4483946c0e4857da545b`, ExecMainPID `1673926`,
ExecMainCode `1`, ExecMainStatus `4`, Result `exit-code`. The `--wait --pipe`
output was retained in the bounded SSH operation record, but the
invocation-filtered user journal had no entries. This is independently
non-promotable under L50's mandatory early-exit evidence requirement.
The first closeout journal command also exposed a fish cursor-quoting failure;
that transport evidence is retained and is not classified as the runtime
failure.

## Safety and closeout

- Production preflight and final snapshots are byte-identical, SHA-256
  `511f05d2b638277e19ae7af44573eb9e244d829458f24cdefc46e3f88ffd6ded`.
- nimo-1 production coordinator remained PID `2356329`, port `8081`,
  HTTP 200, NRestarts 0.
- nimo-2 production worker remained PID `1535639`, port `50052`,
  NRestarts 0.
- All five disposable units are not-found/inactive/dead/MainPID 0.
- Ports 50248 and 50249, protected keys, source/build roots, state/evidence
  roots, and disposable processes are absent.
- The primary artifact was not accessed and production was not mutated.
- The rejected runtime candidate is removed from the terminal tree.

Raw evidence and its integrity manifest are under
`docs/halofpx/evidence/l50-raw/`. The SHA-256 of `SHA256SUMS` is
`e38059b6db8b33cac0009adac693f8a9b59f9b90751be05204012162264f5d2c`.
