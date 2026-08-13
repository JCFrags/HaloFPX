# ROCmFPX dense FFN Q8_1 reuse compile qualification

Status: **[VERIFIED] source and target compile/link gate passed** for the
default-off implementation candidate in GitHub issue #29. Runtime dispatch,
numerical parity, launch-count reduction, and performance remain **[OPEN]**.

## Authority and environment

- source commit: `3402aa7fbe820496726bfb45504549830634d7bd`;
- parent implementation commit: `26a639beab45b1f00e405f906507f67ce1aa0458`;
- required source base: `bf420e9f1db4ea4ba1d7c87771b6a4d662b5be67`;
- exact Git bundle SHA-256:
  `4526b227b3bd45fe63eecc0c9803788a157d31060aa054c83cde50d91ddadcf6`;
- host: `nimo-1`, CachyOS, Linux `7.1.3-1-cachyos`, `gfx1151`;
- HIP `7.2.53211-3d9ef42`, AMD clang 22 commit `f58b06d`, CMake 4.3.4;
- observation/build window ended at `2026-08-13T00:04:00Z`.

The exact bundle was imported into a clean detached source tree. The OFF and
ON configurations were Release shared-library builds with:

```text
BUILD_SHARED_LIBS=ON
GGML_HIP=ON
GGML_HIP_FORCE_MMQ=ON
GGML_RPC=ON
GGML_RPC_HALOFPX_LOCAL_STATE=ON
GGML_VULKAN=OFF
CMAKE_HIP_ARCHITECTURES=gfx1151
GPU_TARGETS=gfx1151
```

The pair differed only in
`GGML_HIP_ROCMFPX_FFN_Q8_REUSE=OFF|ON`. Builds ran at `nice -n 15` and
`ionice -c3`, serially, with two compile jobs. No GPU-facing executable or
model was run.

## Results

Both configurations compiled and linked these targets successfully:

- `ggml-hip`;
- `test-backend-ops`;
- `test-halofpx-rocmfpx-ffn-q8-reuse-off`;
- `test-halofpx-rocmfpx-ffn-q8-reuse-on`.

The host-only OFF selector, ON selector, and source-contract tests passed 3/3
in both build directories. `test-backend-ops` was deliberately not executed
because this receipt is compile-only and production GPU workloads remained
live.

The retained `compile_commands.json` evidence shows:

| Translation unit | OFF feature macro count | ON feature macro count | `gfx1151` |
|---|---:|---:|---:|
| `ggml-cuda.cu` | 0 | 1 | present in both |
| `mmq.cu` | 0 | 1 | present in both |

Selected artifact SHA-256 values:

| Artifact | OFF | ON |
|---|---|---|
| `compile_commands.json` | `e06d2c646945557ee59e294d42fd456c7e228ccb12e20edf7fb513c60c4fada2` | `91a332ced7b250730ee0e2bc1d5671ca85063cb10ac92183aace40f8ca57f821` |
| `libggml-hip.so.0.11.1` | `280b1132ee1327ee5d4b5ecf6063b95da77f7f855661653f2cc814c130d01299` | `d9c8a27bf704a266b243e594d1dbbdcbb82e759865f5c982be8fb7577831f3cb` |
| `test-backend-ops` | `751adecc0ed57d2754d214f04a97981ac4f0a93cfe5a0206b9a9577e9b6bcadd` | `d1e60215d66f8a13381dbcc65e11e34c4b73ad4cb4b883bbf55a6895c2ad40b7` |

The compressed raw controller bundle is retained outside Git at
`C:\Users\britt\AppData\Local\Temp\halofpx-ffn-q8-reuse-3402aa7-evidence.tar.gz`
with SHA-256
`d5a5cff6d8b39238663aba20674eee56d322334ad3b97279af1a6105759a6a31`.
It contains both final build logs, both compile-command databases, both
host-contract logs, and the two excluded configuration logs below.

## Excluded configuration observations

Two preliminary link attempts are retained as repository build-matrix gaps,
not as failures of this candidate:

1. `GGML_RPC=OFF` linked `ggml-hip`, then failed the final executable link on
   existing unconditional HaloFPX llama RPC references.
2. `GGML_RPC=ON` with `GGML_RPC_HALOFPX_LOCAL_STATE=OFF` linked `ggml-hip`,
   then reached the already documented missing OFF stub for
   `ggml_backend_rpc_halofpx_mutable_negotiate_preflight`.

The excluded logs have SHA-256 values
`742851a6b6aa8af8b3c8a348c7cf85097ebd3587a86ee9472e2b96e0a682abb3`
and
`2721b49c52303a1d517e2c21195de18a4d9cdd7931f67feeea2fd857e55446d6`.
The same missing preflight symbol was previously recorded by L71 and L106.

## Production and claim boundary

Before/during and after the compile, production identities remained unchanged:

- nimo-1 coordinator PID `3027112`, InvocationID
  `e6da1fe637144cb394119959c0e88736`, `NRestarts=0`, HTTP health `ok`;
- nimo-2 worker PID `2148915`, InvocationID
  `3480c89086e04d5d80060366c5c7ab7f`, `NRestarts=0`.

This proves that the guarded code compiles and links for the target toolchain.
It does not prove that a real graph selects the pair path, that results match,
that one rather than two conversion kernels launches, or that prompt speed or
time to first token improves. Those require isolated target execution and
matched evidence under the qualification plan in
[`../../rocmfpx-ffn-q8-reuse.md`](../../rocmfpx-ffn-q8-reuse.md).
