# Source and build provenance

## Core SDK/TheRock 7.14.0

| Item | Exact pin |
|---|---|
| TheRock tag | `therock-7.14` |
| TheRock commit | `418cd5f63abb7a604bad5874cd7b2e29334e640f` |
| LLVM (`compiler/amd-llvm`) gitlink | `aa451e1fe6a793394d6733051b1778633063ae96` |
| HIPIFY gitlink | `6acec7751d2b2bfe162dba9efdcf7c16efb27bd8` |
| SPIR-V LLVM Translator gitlink | `fb08e83ae872775acfeaee53fda3ccf99a04ba53` |
| rocm-libraries gitlink | `cd9574023093742434e8c992d13b89ab9a6c1cf8` |
| rocm-systems gitlink | `2b22ab0195cc1461cd9abf3b969e9dd7c10af350` |
| Tag/commit signature | unverified in this capture |

The authoritative recursive component pins are the gitlinks reachable from the frozen TheRock commit and the acquired artifact's `share/therock/therock_manifest.json`. Branch names in `.gitmodules` are discovery metadata, not lock values.

Use `scripts/fetch-7.14-sources.sh`. It checks out the exact commit, initializes submodules, records `git submodule status --recursive`, emits `git ls-tree` gitlinks, hashes patch files, and optionally compares the checkout to an extracted artifact manifest.

## Legacy ROCm 7.2.4

| Repository | Ref | Commit |
|---|---|---|
| ROCm/ROCm | `rocm-7.2.4` | `e0b62c25d8ea39473a7208c1c8995f1b5c2e277c` |
| ROCm/rocm-systems | `rocm-7.2.4` | `97f5574fe2fdc7bef44fb01545347912ee9f1779` |
| ROCm/rocm-libraries | `rocm-7.2.4` | `dabb6df2b988f8eabed1e2fecefaaf4e818bc7ef` |
| ROCm/llvm-project | `rocm-7.2.4` | `f58b06dce1f9c15707c5f808fd002e18c2accf7e` |

[PROVENANCE_GAP] The umbrella `default.xml` at the 7.2.4 tag defaults projects to `rocm-7.2.0`; it cannot serve as a patch-level lock by itself. Use the exact superrepository commits and record every recursive gitlink.

## Build-environment manifest

A reproducible build must additionally pin:

- base image digest and package snapshot;
- CMake/Ninja/Python/compiler versions;
- environment variables and CMake cache;
- source patches and their hashes;
- GPU target selection (`gfx1151` or the documented family selector);
- generated kernel packs and tuning databases;
- install file list and hashes;
- tests executed and their source revisions.

The external portion of this contract is in [open-base-01-external.yaml](../manifests/open-base-01-external.yaml).

## Compiler and device-library identity

Capture both path and content identity. The minimum record is:

```text
amdclang --version
amdclang --print-resource-dir
hipcc --version
hipconfig --full
find <rocm-root> -path '*/amdgcn/bitcode' -type d
find <rocm-root> -path '*/lib/clang/*' -type d
sha256sum of every bitcode/resource file
```

A path such as `/opt/rocm/lib/llvm/amdgcn/bitcode` is not assumed to be valid across lanes. The selected compiler is authoritative for its resource directory; the artifact inventory is authoritative for the device-library directory.
