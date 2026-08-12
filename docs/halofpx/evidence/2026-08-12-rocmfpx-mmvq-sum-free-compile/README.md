# ROCmFPX MMVQ sum-free target compile receipt — 2026-08-12

Purpose: retain the first exact-target qualification evidence for the
default-off `GGML_HIP_ROCMFPX_MMVQ_SUM_FREE` candidate from
`codex/rocmfpx-mmvq-sum-free` at commit
`3c9ba0b7c842a02b47c591ad47448cfb94b6dc72`.

The later `feature-off/` records bind the hardened branch head
`a3fd09e0d522e20d0153bb2a07ddd09916249c8d` and prove that the same HIP source
also compiles for `gfx1151` with the candidate definition absent.

This is a **compile and host-contract receipt**, not a GPU correctness or
performance result. The production inference processes remained active, so the
fail-closed target qualifier correctly was not used to execute candidate HIP
kernels or model benchmarks.

## Retained result

- **[MEASURED]** Both CachyOS Strix Halo machines compiled the exact source
  commit through HIP for `gfx1151`, including `quantize.cu` and
  `libggml-hip.so.0.11.1`.
- **[MEASURED]** Each retained `quantize.cu` compile command contains both the
  `gfx1151` target and the
  `GGML_HIP_ROCMFPX_MMVQ_SUM_FREE` candidate definition.
- **[MEASURED]** The feature-off and feature-on host contracts passed 2/2 on
  both machines. These contracts check the exact admitted type set, unchanged
  36-byte `block_q8_1` ABI, positive-zero unused high-lane policy, and the
  expected compile mode. Source inspection, not these host contracts, establishes
  that the candidate kernel retains the scale and all 32 quantized bytes.
- **[MEASURED]** Both independently built HIP libraries had SHA-256
  `aab1bff95c831f5222701dac0e9124ef1c11a5201a2c1294fa6ae6dec97cb0cc`.
- **[MEASURED]** The later feature-OFF build completed on both targets with the
  candidate macro absent from `quantize.cu`, `gfx1151` present, and matching
  HIP-library SHA-256
  `eb870d948e68ee200a831c51ea615bbb06be5d8ad68f35f13c4bc1bc2a2c9868`.
- **[MEASURED]** Post-build snapshots recorded nimo-1's coordinator active and
  healthy at PID `3027112`, InvocationID
  `e6da1fe637144cb394119959c0e88736`, with zero restarts. The nimo-2 snapshot
  recorded its RPC worker active and listening at PID `2148915`, InvocationID
  `3480c89086e04d5d80060366c5c7ab7f`, with zero restarts. These snapshots show
  no recorded restart; they are not continuous service monitoring.
- **[OPEN]** No candidate HIP kernel was executed in this receipt. Numerical
  parity, model-output parity, and generation-speed benefit still require a
  matched OFF/ON target run while the production processes are deliberately
  idle.

The exact Git bundle delivered to each host had SHA-256
`df993cc4ae058edb46a00601952e4ebdd1011b032926906368afdcc59cd834af`.
The source was cloned from that bundle into `/tmp`, checked out by exact commit,
configured in a fresh build directory, and built with four low-priority jobs.

## Environment boundary

Both machine records identify:

- CachyOS rolling Linux with kernel `7.1.3-1-cachyos`;
- HIP `7.2.53211-3d9ef42` and AMD clang 22;
- CMake 4.3.4; and
- the `gfx1151` HIP target.

The `nimo-1/` and `nimo-2/` directories retain the candidate-ON
configure/build logs,
compile-command proof, source and bundle identities, host-contract output,
artifact hashes, OS/toolchain records, service-health observation, and the
remote capture checksums. `feature-off/nimo-1/` and `feature-off/nimo-2/`
retain the corresponding hardened-head feature-OFF records and exact build
driver. Absolute `/tmp` paths in raw records are the paths on the named target
at capture time.

## Claim boundary

This receipt supports merging a default-off implementation seam that compiles
on both exact target machines. It does not support enabling the option by
default, claiming a generation speedup, changing serialized GGUF data, or
extending the optimization to ROCmFP4 or stock quantization types. Promotion
requires the repository's full target qualifier, including GPU correctness,
legacy controls, model census/parity when a suitable model is available, and
matched ABBA performance evidence.
