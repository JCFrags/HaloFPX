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

Selected build-time SHA-256 values reported by the controller:

| Artifact | OFF | ON |
|---|---|---|
| `compile_commands.json` | `e06d2c646945557ee59e294d42fd456c7e228ccb12e20edf7fb513c60c4fada2` | `91a332ced7b250730ee0e2bc1d5671ca85063cb10ac92183aace40f8ca57f821` |
| `libggml-hip.so.0.11.1` | `280b1132ee1327ee5d4b5ecf6063b95da77f7f855661653f2cc814c130d01299` | `d9c8a27bf704a266b243e594d1dbbdcbb82e759865f5c982be8fb7577831f3cb` |
| `test-backend-ops` | `751adecc0ed57d2754d214f04a97981ac4f0a93cfe5a0206b9a9577e9b6bcadd` | `d1e60215d66f8a13381dbcc65e11e34c4b73ad4cb4b883bbf55a6895c2ad40b7` |

The two `compile_commands.json` files are retained in the portable archive and
can be hashed again. The `libggml-hip.so.0.11.1` and `test-backend-ops` values
are controller-recorded build observations only: the target binaries and raw
checksum-command stdout were not retained. Those four binary digest values
identify the outputs observed during qualification but are not independently
recoverable or re-hashable from the published assets.

The portable raw evidence archive is **[VERIFIED] published and byte-checked**
as an immutable private prerelease asset:

- tag:
  `evidence-ffn-q8-reuse-3402aa7-2026-08-12`;
- release target and tag commit:
  `7e68d8a2eaa36a5a115ca2736f6bfca66ee4770f`;
- asset:
  [`halofpx-ffn-q8-reuse-3402aa7-evidence-portable.tar.gz`](https://github.com/JCFrags/HaloFPX/releases/download/evidence-ffn-q8-reuse-3402aa7-2026-08-12/halofpx-ffn-q8-reuse-3402aa7-evidence-portable.tar.gz);
- size: `40,697` bytes;
- SHA-256:
  `7a154b62d665c0a1324a84eda8adadde32006a1467f259bfb7e583f9797a82b0`.

GitHub reported release ID `369641490`, asset ID `512401535`,
`prerelease=true`, and `immutable=true`. An authenticated fresh download after
publication matched both the exact size and SHA-256 above. This supplemental
release is outside the original `evidence-2026-08-12` publication manifest.
The immutable tag points to the pre-publication PR head, so its tree still
contains the earlier pending-upload text and cannot be rewritten to include
this later result. This post-publication branch receipt and
[`../../../../ARTIFACTS.md`](../../../../ARTIFACTS.md), then their `main`
versions after merge, are the tracked recovery authority. Verify both fields
before using the asset.

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

## Exact target-built source recovery

The exact thin Git bundle used to create the target source tree is
**[VERIFIED] published and byte-checked** as a second immutable private
prerelease asset:

- tag:
  `evidence-ffn-q8-reuse-source-bundle-3402aa7-2026-08-12`;
- release target and tag commit:
  `0db715c6e436be88a4d5444763421020f53dc728` (PR #45 merged on `main`);
- asset:
  [`halofpx-ffn-q8-reuse-3402aa7.bundle`](https://github.com/JCFrags/HaloFPX/releases/download/evidence-ffn-q8-reuse-source-bundle-3402aa7-2026-08-12/halofpx-ffn-q8-reuse-3402aa7.bundle);
- size: `13,082` bytes;
- SHA-256:
  `4526b227b3bd45fe63eecc0c9803788a157d31060aa054c83cde50d91ddadcf6`.

GitHub reported release ID `369647794`, asset ID `512424633`,
`prerelease=true`, and `immutable=true`. An authenticated fresh download was
byte-identical to the local source bundle and matched the size and SHA-256
above. `git bundle verify` passed, listed
`3402aa7fbe820496726bfb45504549830634d7bd` at
`refs/heads/codex/rocmfpx-ffn-q8-reuse`, and reported prerequisite
`bf420e9f1db4ea4ba1d7c87771b6a4d662b5be67`.

Recover the exact target-built source from an authenticated current HaloFPX
clone without changing the checked-out branch:

```powershell
gh release download evidence-ffn-q8-reuse-source-bundle-3402aa7-2026-08-12 `
  --repo JCFrags/HaloFPX `
  --pattern halofpx-ffn-q8-reuse-3402aa7.bundle
Get-Item .\halofpx-ffn-q8-reuse-3402aa7.bundle | Select-Object Length
Get-FileHash -Algorithm SHA256 .\halofpx-ffn-q8-reuse-3402aa7.bundle
git bundle verify .\halofpx-ffn-q8-reuse-3402aa7.bundle
git fetch .\halofpx-ffn-q8-reuse-3402aa7.bundle `
  'refs/heads/codex/rocmfpx-ffn-q8-reuse:refs/remotes/evidence/ffn-q8-reuse-3402aa7'
git rev-parse refs/remotes/evidence/ffn-q8-reuse-3402aa7
```

Accept the recovered ref only when its identity is
`3402aa7fbe820496726bfb45504549830634d7bd`. The bundle requires base commit
`bf420e9f1db4ea4ba1d7c87771b6a4d662b5be67`, which is retained in current
HaloFPX history. The bundle contains source Git objects only. It contains no
target binaries, build directories, raw checksum-command stdout, production
state, credentials, or model data. The immutable tag points to PR #45's merge
commit before this later recovery section; this tracked receipt and
[`../../../../ARTIFACTS.md`](../../../../ARTIFACTS.md), then their `main`
versions after merge, are the completed URL and recovery authority.

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

The retained pre-build observation recorded these production identities:

- nimo-1 coordinator PID `3027112`, InvocationID
  `e6da1fe637144cb394119959c0e88736`, `NRestarts=0`, HTTP health `ok`;
- nimo-2 worker PID `2148915`, InvocationID
  `3480c89086e04d5d80060366c5c7ab7f`, `NRestarts=0`.

The portable archive does not contain a post-build identity/health probe, so it
does not establish an unchanged before/during/after production tuple. It proves
that the guarded code compiles and links for the target toolchain.
It does not prove that a real graph selects the pair path, that results match,
that one rather than two conversion kernels launches, or that prompt speed or
time to first token improves. Those require isolated target execution and
matched evidence under the qualification plan in
[`../../rocmfpx-ffn-q8-reuse.md`](../../rocmfpx-ffn-q8-reuse.md).
