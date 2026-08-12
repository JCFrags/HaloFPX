# OPEN-PIN-01 matched build qualification — 2026-07-17

Status: `PARTIAL PASS — BUILD/REFERENCE LANE COMPLETE; PIN STILL OPEN`

## Question

Can ROCmFPX research control `a5605a72768c6562241b248e268e33dc92787394` and nominated implementation candidate `61f2f2d7bc4955e9bca821095ef69125837133b5` build from the same locked source and toolchain on both target gfx1151 nodes?

This run cannot close `OPEN-PIN-01`. It covers source restoration, build, artifact identity, and immediately available reference/backend tests only. Model, MTP, long-context memory, graph replay, state compatibility, RPC, performance, and Vulkan D=256 gates remain separate.

## Authorization

The user authorized unloading the current model and doing local experiments on both nodes. The deployed runtime, unit files, model, and RPC cache were hashed and preserved before this run. Model services are inactive but enabled.

## Inputs

- Offline all-ref bundle SHA-256: `bcbe6cf910f4dd183d8ad96ea0d936ac85bd636a1cfe570179599d3fc5e307fa`.
- Bundle authority: `sources/repositories/source-locks/2026-07-17-pre-fork/`.
- Control commit: `a5605a72768c6562241b248e268e33dc92787394`.
- Candidate commit: `61f2f2d7bc4955e9bca821095ef69125837133b5`.
- Disposable node root: `/home/connorb/halofpx-lab/open-pin-01/`.
- Build script: pinned `scripts/build-strix-rocmfp4-mtp.sh`, `JOBS=16`, default stable decode tuning, `gfx1151`, HIP+Vulkan, tests enabled.

No network fetch, package installation, deployed-path overwrite, or source modification is permitted in this run.

## Order

1. Restore two independent clean worktrees from the bundle on nimo-2.
2. Record commit/tree/status/toolchain and CMake inputs.
3. Build control, then candidate; stop on the first unexplained failure.
4. If both pass, run pinned CPU reference checks and enumerate candidate test targets/options.
5. Repeat the same source/toolchain procedure on nimo-1 only after nimo-2 passes and its free-space estimate remains above the reserve.

## Stop rules

- source or bundle hash mismatch; dirty restored worktree; unexpected network access;
- free root space below 250 GB on nimo-2 or 30 GB on nimo-1;
- `MemAvailable` below 32 GB, swap growth above 2 GiB during build, OOM, filesystem/NVMe/GPU/kernel error;
- build root escapes `/home/connorb/halofpx-lab/open-pin-01/`;
- deployed runtime/model/cache/config changes or any model service starts unexpectedly;
- SSH management becomes unreliable or an unexplained compiler/linker error occurs.

## Cleanup and rollback

Build roots are isolated and may be retained for review. Do not delete them until local evidence is copied and hashed. Restoring service uses the dependency order in `sources/measurements/2026-07-17-local-preparation/MODEL-UNLOAD-RECEIPT.md` and is deferred until local preparation finishes or the user requests it.

## Outcome

Both exact revisions built successfully on both target nodes from the locked offline bundle. The immediately available CPU reference checks, TurboQuant unit suite, ROCmFP4 quant regression, and the complete `FLASH_ATTN_EXT` ROCm0 matrix passed. See [RESULTS.md](RESULTS.md). This is not a pin decision and does not close `OPEN-PIN-01`.
