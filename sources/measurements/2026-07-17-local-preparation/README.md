# Local preparation evidence — 2026-07-17

Status: `CAPTURED`

This directory preserves read-only paired-machine manifests, model inventories, and the controlled model-unload receipt used for pre-fork preparation. It is environment-specific evidence, not a claim about all Strix Halo systems.

## Raw evidence

- `nimo-1/paired-machine-manifest.txt`: 1,201 lines; SHA-256 `e2c6b0180940808053065fc9110e3099f76795e209454e199b50375a159bf9d4`.
- `nimo-2/paired-machine-manifest.txt`: 1,234 lines; SHA-256 `e450fb9649199e990d5671954f508a118120bd1ce1c1e66b593873e459a0ead6`.
- Each node also has a size/mtime/inode model inventory. No model was deleted.

## Matched platform summary

Both nodes reported `[MEASURED]`:

- AMD Ryzen AI Max+ 395, 16 cores / 32 threads, one NUMA node;
- Radeon 8060S, `gfx1151`, VBIOS `113-STRXLGEN-001`;
- firmware `3.05` and microcode `0xb700037`;
- CachyOS kernel `7.1.3-1-cachyos`, ROCm core/runtime `7.2.4`, HIP `7.2.53211-3d9ef42`;
- GCC `16.1.1`, Clang `22.1.6`, CMake `4.3.4`;
- two `mtu 9000` Thunderbolt-net interfaces and MPTCP enabled.

Material difference: nimo-1's kernel command line did not include `zswap.enabled=0`; nimo-2's did. Root storage also differed materially: about 46.1 GB free on nimo-1 versus 341.0 GB on nimo-2 at preflight.

## Current kernel and USB4STREAM

`[MEASURED]` Neither node's installed kernel/module/header/config search exposed a `USB4STREAM` symbol or module. Both kernels have `CONFIG_USB4=m` and `CONFIG_USB4_NET=m`; `CONFIG_USB4_DMA_TEST` is disabled. This does not establish whether a separate USB4STREAM patch series can be obtained or built.

## Handling

Raw manifests may contain machine serial identifiers and should stay inside the project evidence boundary. They should not be published without redaction review.

