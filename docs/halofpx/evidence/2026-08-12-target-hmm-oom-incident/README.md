# 2026-08-12 nimo-2 HMM/global-OOM safety incident

Disposition: **SAFETY INCIDENT / INVALID BENCHMARK / NO PERFORMANCE RESULT**

This directory is the portable, immutable record of a production-safety
incident on the dual CachyOS Strix Halo target. It is linked to
[#41](https://github.com/JCFrags/HaloFPX/issues/41) and was assembled from
exact repository base
`b77f2bce6e7875ab065e09894f45915585c9f156`. The interrupted candidate was
`e1023132accf92db92898751844a37c59f5e5a69`; it was never benchmarked.

## What happened

While the established nimo-2 RPC worker owned approximately 114,041,696 KiB
of `gpu_active` HMM pages, an unrelated `-j2` candidate build was allowed to
run because ordinary host-memory indicators still showed roughly 14 GiB
available. Between `2026-08-12T19:10:05.500410-07:00` and
`2026-08-12T19:10:15.628468-07:00`, the kernel invoked global OOM four times.
It killed the user bus launcher, the user systemd manager, its PAM helper, and
finally the production `ggml-rpc-server` process.

The build's visible compiler residents were only tens of MiB. This evidence
does not establish a conventional build RSS leak or a benchmark result. It
establishes that `MemAvailable`, free RAM, and swap are not sufficient target
admission predicates while KFD/render/HMM ownership is high. Additional target
activity was unsafe in that state.

| Pacific time | Evidence-backed event |
|---|---|
| 19:10:05.500410 | First `global_oom`; kernel reports `gpu_active:114041696kB`. |
| 19:10:05.517983 | OOM kills the user D-Bus launcher. |
| 19:10:15.627744–19:10:15.628138 | Repeated OOM kills the user systemd manager and PAM helper. |
| 19:10:15.628468 | OOM kills production nimo-2 worker PID `2148915`. |
| 19:10:18.896908 | systemd schedules worker restart 1. |
| 19:10:18.939118 | Replacement worker PID `2248760` starts. |
| 19:12:02.385396 | A real coordinator request reaches stale RPC state and fails. |
| 19:12:04.778926 | Coordinator fails with `core-dump`. |
| 19:12:09.813623 | systemd schedules coordinator restart 1. |
| 19:16:23.124104 | Replacement coordinator completes a 5-prompt-token plus 1-generated-token recovery probe. |

The final probe is recovery evidence only. Its printed timing values are not a
performance measurement and must never be promoted as one.

## Production authority before and after

The before values are controller-observed. The after values are from the
read-only `systemctl`, `/proc`, and health capture in `raw/`.

| Role | Before work | Recovered state |
|---|---|---|
| nimo-1 coordinator | PID `3027112`; InvocationID `e6da1fe637144cb394119959c0e88736`; restarts 0; `/health` OK | PID `3113343`; InvocationID `0656332b63a140eab7214627baa43253`; restarts 1; active/running; `/health` OK |
| nimo-2 worker | PID `2148915`; InvocationID `3480c89086e04d5d80060366c5c7ab7f`; restarts 0; listener `:50052` | PID `2248760`; InvocationID `d15fe49610274e77bd9a3d84a0b791a5`; restarts 1; active/running |

The coordinator health route was not a distributed-readiness proof. It could
remain superficially healthy after the worker restarted; the first real
request exercised stale RPC state and aborted the coordinator. After either
rank changes identity, recovery therefore requires both exact service
identities and a real minimal two-rank inference request.

## Interrupted build

The OFF build was Release/Ninja for `gfx1151`, with HIP and
`GGML_HIP_FORCE_MMQ` enabled; CUDA, Vulkan, RPC, server, web UI,
`GGML_HIP_ROCMFPX_MMVQ_SUM_FREE`, and
`GGML_HIP_ROCMFPX_FFN_Q8_REUSE` were disabled. Tests were enabled. The command
was:

```text
cmake --build /var/tmp/halofpx-screen-20260813/build-ffn-off --parallel 2 --target llama-bench test-backend-ops
```

The observed tree was `fish 2244520 -> cmake 2244762 -> ninja 2244763 ->
c++/cc1plus`. The controller sent TERM to the known tree, waited two seconds,
and confirmed no matching build process. The OFF build stopped at step 380/402
with `ninja: build stopped: interrupted by user`. The planned ON build was
never configured or built. No completed binary, parity output, or performance
sample was retained.

The source archive and interrupted OFF build directory remained under
`/var/tmp/halofpx-screen-20260813` at capture time. They are inert retained
artifacts, not qualified binaries. This response did not delete them or mutate
either target.

## Safety rule established by the incident

Target builds, quantization, disposable inference, and A/B runs are rejected
while a protected production service or any unaccounted KFD/render/HMM owner
is active. Authorized maintenance, an exact before-state receipt, an empty
foreign GPU-owner census, and a clean kernel-OOM baseline are prerequisites.
`MemAvailable`, free RAM, conventional RSS, and swap must not override this
gate. A worker identity change invalidates coordinator RPC readiness until a
real minimal inference succeeds.

## Evidence map

- `raw/nimo-2-kernel-oom-window.stdout.log`: exact kernel OOM window and task
  census; primary incident authority.
- `raw/nimo-2-worker-restart-window.stdout.log`: worker OOM, failure, restart,
  and reload journal.
- `raw/nimo-1-coordinator-restart-window.stdout.log`: stale-RPC failure,
  coordinator restart, and real recovery request.
- `raw/nimo-{1,2}-current-authority.stdout.log`: post-recovery identities,
  memory accounting, and coordinator health.
- `raw/nimo-2-build-{archive,cmake-cache,ninja-log}.stdout.log`: exact retained
  source/archive and incomplete OFF-build configuration/progress.
- `raw/nimo-2-build-process-absence.stdout.log`: empty successful/accepted
  process census; no matching build process remained.
- `raw/controller-observed-facts.txt`: before-state and interruption facts
  retained by the launch controller.
- `raw/github-issue-41.stdout.log`: portable issue snapshot.
- Matching `.stderr.log` files are retained collection receipts; zero-byte
  files mean the command emitted no standard error.
- `manifest.json` records identities, classification, boundaries, and hashes.
- `SHA256SUMS` covers every portable artifact except itself.

## Fresh-PC verification and optional recollection

From a fresh authenticated clone, verify the committed bundle offline:

```powershell
pwsh ./docs/halofpx/evidence/2026-08-12-target-hmm-oom-incident/validate.ps1
```

Recollection is optional and produces a new journal/current-state capture
rather than changing the historical evidence. The one-time build-configuration
and controller receipts remain immutable committed inputs. Recollection
requires authorized SSH aliases `nimo-1` and `nimo-2` plus GitHub CLI access
to the private repository. Use a new output directory:

```powershell
pwsh ./docs/halofpx/evidence/2026-08-12-target-hmm-oom-incident/collect-read-only.ps1 `
  -OutputRoot C:/absolute/path/to/new-read-only-capture
```

Target-side collection permits only read-only `journalctl`, `systemctl`,
`/proc`, `free`, and health operations. It refuses to overwrite a non-empty
output directory unless `-Force` is explicitly given. It does not build,
quantize, run inference, or control services.

This private-repository bundle contains operational host names, paths, process
IDs, and invocation IDs needed for continuation. It contains no credentials,
tokens, model contents, or private keys.
