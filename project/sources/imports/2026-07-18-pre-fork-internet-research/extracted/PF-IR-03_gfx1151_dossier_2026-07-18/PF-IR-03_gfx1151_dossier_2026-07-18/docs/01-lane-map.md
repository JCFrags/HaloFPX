# Lane map and non-mixing rules

## Version lanes

| Property | Legacy ROCm 7.2.x | Core SDK/TheRock 7.14.x |
|---|---|---|
| Release/build model | Legacy ROCm release stream | TheRock-built Core SDK |
| Primary prefix | `/opt/rocm` | `/opt/rocm/core` for package installs; selected tarball root for raw SDK |
| Package naming | `rocm-*`, `roc*`, `hip*` | `amdrocm-*`, architecture-suffixed meta packages |
| gfx1151 package example | legacy packages resolved from 7.2.4 repository | `amdrocm-core-sdk7.14-gfx1151` or `amdrocm7.14-gfx1151` |
| Source root | ROCm umbrella/superrepositories at `rocm-7.2.4` | TheRock `therock-7.14` plus exact gitlinks |
| Maturity | 7.2.4 production release broadly; Ryzen platform wording remains preliminary/preview-scoped | Explicit preview release; gfx1151 build/sanity pass but no Release Ready mark |
| Package signing | Distro package-manager metadata must be retained and verified | Stable multi-arch package repository documents GPG `signed-by`; nightly native packages are a different, explicitly unsigned lane |

## [DO_NOT_MIX] Runtime namespace rules

A tuple fails provenance review when any of these conditions is true:

- `ldd`, RPATH, `LD_LIBRARY_PATH`, `PATH`, `HIP_PATH`, `ROCM_PATH`, `HIP_DEVICE_LIB_PATH` or CMake package discovery crosses lane roots.
- A 7.14 `amdclang` uses 7.2 device bitcode, or a 7.2 compiler resolves 7.14 `.kpack`/device libraries.
- `libamdhip64`, `libhsa-runtime64`, COMGR, rocBLAS/hipBLASLt or RCCL come from different lanes.
- Package-manager files and an unpacked SDK shadow the same SONAMEs.
- `/opt/rocm` compatibility symlinks are treated as proof of source identity instead of resolving them to their target package.

The ABI/API compatibility statement in AMD's transition guide is not permission to create an untracked mixed installation. It describes application compatibility; it does not replace artifact provenance.

## Isolation patterns

Preferred patterns are separate immutable images, separate boot entries for kernel overlays, and explicit environment activation. At minimum, record:

```text
readlink -f /opt/rocm
readlink -f /opt/rocm/core
command -v amdclang hipcc rocminfo
ldd <workload-binary>
readelf -d <workload-binary>
```

The supplied scripts capture resource directories and shared-object resolution so a mixed lane can be rejected mechanically.
