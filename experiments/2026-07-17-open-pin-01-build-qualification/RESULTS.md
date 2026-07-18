# OPEN-PIN-01 build/reference qualification results

Status: **PARTIAL PASS — BUILD/REFERENCE ONLY**

## Exact comparison

| Item | Control | Candidate |
|---|---|---|
| Commit | `a5605a72768c6562241b248e268e33dc92787394` | `61f2f2d7bc4955e9bca821095ef69125837133b5` |
| Tree | `6528a116ad015c316948d288b4ffd9c3586c00ad` | `0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd` |
| nimo-1 build | **[MEASURED]** pass, 167.46 s | **[MEASURED]** pass, 168.60 s |
| nimo-2 build | **[MEASURED]** pass, 168.95 s | **[MEASURED]** pass, 168.47 s |
| ROCm0 `FLASH_ATTN_EXT` | **[MEASURED]** 2881/2881 on each node | **[MEASURED]** 2899/2899 on each node |
| TurboQuant unit suite | **[MEASURED]** 7/7 on each node | **[MEASURED]** 7/7 on each node |

The candidate's 18 additional attention cases all passed on both nodes. They cover D=128/256, KV=1280, nontrivial block counts, padded mask, mixed Q8/Turbo cache types, sinks, softcap, ALiBi, GQA, and permuted layouts. The totals above come from each test binary's own terminal summary, not line counting.

## Other passed checks

**[MEASURED]** Both revisions passed the pinned ROCmFP2 and ROCmFPX CPU reference programs and the corrected ROCmFP4 quant regression on both machines. Every run returned zero. No source tree became dirty; no stop-rule swap growth, reserve breach, kernel warning, or failed unit was observed.

**[MEASURED]** For a given commit, the recorded hashes of the principal executables and shared libraries are identical across nimo-1 and nimo-2. For example, `llama-server` is `6a0d77a775f74c0fc6c83e0a7eb52e568e35db0d2c65040c6a2e8d821335046c` for the control and `7be9b07041b63159e6c0fb6bd489b751f3eebdf00d4ae5660af16eb7b6a799de` for the candidate.

## Harness deviations retained as evidence

- The first nimo-2 control launcher stopped before compilation because `/usr/bin/time` is absent. The unchanged build was rerun with Bash's `time`; it passed. This was a harness dependency, not a source failure.
- The first nimo-2 reference launcher used an empty build path for the quant regression and `--list` instead of `--list-ops`. The original log is retained. The corrected commands passed and were reproduced on nimo-1.

## Decision boundary

**[OPEN]** This result does not select either pin and does not qualify model conversion, long-context behavior, MTP/speculative decoding, state compatibility, RPC/distributed execution, Vulkan numerical parity, sustained memory behavior, quality, performance, or release use. `OPEN-PIN-01` remains open until the broader candidate matrix and human decision are complete.
