# Known issues and qualification implications

| Label | Scope | Issue | Required qualification action |
|---|---|---|---|
| [KNOWN_ISSUE] | gfx1151 KFD | Incorrect CWSR/VGPR sizing can hang ROCm on kernels missing the two fixes | prove both fixes are present; queue/CWSR stress and suspend/resume |
| [KNOWN_ISSUE] | rocprofiler-compute | `TCP_REQ_sum` and related GL0 metrics report zero on gfx1151 | exclude affected metrics from pass/fail claims |
| [KNOWN_ISSUE] | rocprofiler-compute | `$max_mclk` not auto-populated; bandwidth metrics may be wrong | capture max memory clock via AMD SMI and use specs correction |
| [KNOWN_ISSUE] | RDNA3/Ryzen AI MAX inference | lower LLM performance in affected PyTorch versions | test both default and `TORCH_BLAS_PREFER_HIPBLASLT=1`; record environment |
| [KNOWN_ISSUE] | RCCL | 64–512 MB multi-node collective performance can be degraded | include this size band and a build with fault injection disabled |
| [KNOWN_ISSUE] | SGLang | default AITER/fused decode settings can fail on validated Radeon targets; gfx1151 is not in the listed SGLang target set | classify gfx1151 SGLang as unverified; do not infer from generic Radeon wording |
| [KNOWN_ISSUE] | HIP SPIR-V | first-launch segmentation fault is listed | include cold-start SPIR-V test if used |
| [KNOWN_ISSUE] | profiling | stale SPM sessions may require reboot; normalization issues are documented | clean boot and record profiler state before qualification |
| [KNOWN_ISSUE] | 7.2.4 MIGraphX | minor int8 performance regression | retain in legacy control comparisons; MIGraphX moves outside Core SDK in 7.14 |
| [UNVERIFIED_COMBINATION] | USB4STREAM | no AMD gfx1151 ROCm qualification for 7.2-rc/next kernel | separate boot image; full compute regression and two-node transport test |

## Performance-claim discipline

A workaround-enabled result must not replace the default result. Record both, including environment variables, library resolution, kernel version, firmware identity, power profile, memory clocks and workload commit. Profiler counters known to be invalid cannot support a performance acceptance decision.

## RCCL two-node scope

RCCL is documented for Ryzen, but multi-node transport, topology and message-size performance remain system-specific. At minimum test all-reduce, all-gather, reduce-scatter and broadcast across small, 64–512 MB and large buffers, with correctness checks and failure recovery. Capture RCCL build options and `NCCL_*`/`RCCL_*` environment variables.
