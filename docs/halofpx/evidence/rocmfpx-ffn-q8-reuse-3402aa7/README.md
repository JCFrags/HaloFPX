# ROCmFPX dense FFN Q8_1 reuse compile qualification

Status: **[VERIFIED] source and target compile/link gate passed** for the
default-off implementation candidate in GitHub issue #29. Runtime dispatch,
numerical parity, launch-count reduction, and performance remain **[OPEN]**.

## Authority and environment

- target-built source commit: `3402aa7fbe820496726bfb45504549830634d7bd`;
- target-built parent implementation commit:
  `26a639beab45b1f00e405f906507f67ce1aa0458`;
- target-built source base: `bf420e9f1db4ea4ba1d7c87771b6a4d662b5be67`;
- rebased equivalent source commit:
  `8369bfa296b7ef4d9bd297fc9f4728cd3142af2a`;
- current integration base: `b77f2bce6e7875ab065e09894f45915585c9f156`;
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

The portable raw evidence archive is pending upload as the GitHub release
asset `halofpx-ffn-q8-reuse-3402aa7-evidence-portable.tar.gz`. Its SHA-256 is
`7a154b62d665c0a1324a84eda8adadde32006a1467f259bfb7e583f9797a82b0`
and its size is 40,697 bytes. The release URL remains **[OPEN]** until the
asset is uploaded; verify the checksum before using it.

The archive has eight regular-file members with normalized root ownership,
mode `0644`, timestamp zero, and distinct OFF/ON paths. Rebuilding it changed
only archive names and metadata; every retained payload kept its original
SHA-256:

| Member | SHA-256 |
|---|---|
| `off/halofpx-ffn-q8-reuse-3402aa7-off-qualified.log` | `0b09e7af9dcf08d59bf061effa1183c86b6c3fa4350b568743e9d3fcdaf62492` |
| `on/halofpx-ffn-q8-reuse-3402aa7-on-qualified.log` | `972452c0c21c691b6103fe3e02713efcbebfbca6a1dc8348061cfd0b249ee420` |
| `off/halofpx-ffn-q8-reuse-3402aa7-off-host-contracts.log` | `fa64ae405b96e244c6032bcbdd97c049c6c41906f21d4a8c00bd70d65d8cf0ba` |
| `on/halofpx-ffn-q8-reuse-3402aa7-on-host-contracts.log` | `1768102140c17426abdf483fcfc38919314e08d187123e0549e4a167a40d350e` |
| `excluded/halofpx-ffn-q8-reuse-26a639b-off.log` | `742851a6b6aa8af8b3c8a348c7cf85097ebd3587a86ee9472e2b96e0a682abb3` |
| `excluded/halofpx-ffn-q8-reuse-26a639b-off-rpc-on.log` | `2721b49c52303a1d517e2c21195de18a4d9cdd7931f67feeea2fd857e55446d6` |
| `off/compile_commands.json` | `e06d2c646945557ee59e294d42fd456c7e228ccb12e20edf7fb513c60c4fada2` |
| `on/compile_commands.json` | `91a332ced7b250730ee0e2bc1d5671ca85063cb10ac92183aace40f8ca57f821` |

Before publication, member names, metadata, and content were scanned for
private-key markers, common service-token forms, authorization/cookie
headers, credential assignments and URLs, SSH material, email addresses,
private IPv4 addresses, and Windows/POSIX home paths; no matches were found.

## Rebase equivalence

PRs #30, #31, #34, #35, #36, and #38 reached `main` after the target compile.
The implementation was then rebased onto `b77f2bce`. The exact target-built HIP
execution source and its standalone host/source contracts have identical Git
blobs at `3402aa7` and `8369bfa2`:

| Path | Shared Git blob |
|---|---|
| `ggml/src/ggml-cuda/ggml-cuda.cu` | `4c2aecec6a25ed668f9b87373fa0678e96840003` |
| `ggml/src/ggml-cuda/mmq.cu` | `a669158344dac94ef6f5699e6c878adb901dfac2` |
| `ggml/src/ggml-cuda/mmq.cuh` | `2a579f1a1bc93af83bde7d9fff3cb9882cb930a4` |
| `ggml/src/ggml-cuda/rocmfpx-ffn-q8-reuse.h` | `9e6dcfa328b963e67d812738cadfce5017600673` |
| `tests/test-backend-ops.cpp` | `b3b3f1bd63a7061af931294cbc234024fbefd387` |
| `tests/test-halofpx-rocmfpx-ffn-q8-reuse-source-contract.cmake` | `240e35d218fd5bfb26939738f4d8b2a34fbd667d` |
| `tests/test-halofpx-rocmfpx-ffn-q8-reuse.cpp` | `7b11eeeeb4dac1b4075067e60af03e86c0f3d1c6` |

This was verified with a path-limited `git diff --exit-code`. The full CMake,
workflow, and documentation blobs differ only because the rebase retained
additive changes already present on `main`, including the independent ROCmFPX
MMVQ sum-free option and two-rank cache contracts. The FFN reuse option remains
default OFF, and its private HIP definition is unchanged. The target compile
therefore applies to the rebased HIP execution source, but it still does not
establish runtime correctness or performance.

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
