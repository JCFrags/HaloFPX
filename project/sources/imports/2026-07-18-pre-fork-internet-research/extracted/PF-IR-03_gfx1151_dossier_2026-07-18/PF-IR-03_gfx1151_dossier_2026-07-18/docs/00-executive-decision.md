# Executive decision

**Capture date:** 2026-07-18  
**Scope:** official external provenance and compatibility inputs only. Both-node builds and runtime qualification remain local.

## Decision summary

| Lane | Classification | Official boundary | Promotion posture |
|---|---|---|---|
| Legacy ROCm 7.2.4 | Control/comparison lane | Ryzen 7.2 matrix: Ubuntu 24.04.3, preliminary use of the 24.04.2 installer; gfx1151; PyTorch 2.9/ROCm 7.2/Python 3.12; FP16 validated | Usable as a reproducible control after exact package, kernel and firmware provenance is captured. It is not blanket gfx1151 production qualification. |
| Core SDK/TheRock 7.14.0 | Separately testable candidate | Ubuntu 26.04/GA 7.0 or Ubuntu 24.04.4/HWE 6.17, inbox driver; gfx1151 listed | Candidate only. AMD calls 7.14.0 preview; TheRock gfx1151 lacks a Release Ready mark. Prefer GPG-verified native packages. |
| Stable gfx1151 7.14 tarball | Candidate delivery variant | Exact URL documented | **Blocked:** no vendor checksum/detached signature or SBOM was located. Acquisition hash alone does not authenticate expected bytes. |
| Installed 2026-07-17 tuple | Local comparison | ROCm 7.2.4 + Mesa 26.1.4 + kernel 7.1.3 | Comparison only. Do not infer official qualification. |
| Linux 7.2 USB4STREAM | Separate kernel overlay candidate | At capture: 7.2-rc3; USB4STREAM documented in linux-next | Plausible for feature testing, but unverified with gfx1151 ROCm. Freeze userspace and test as an overlay. |

## Control tuple

`PF-IR-03-CONTROL-7.2.4` is the external control input. Its purpose is reproducibility and comparative testing, not a new support claim. The exact control must retain signed APT metadata, every package version and hash, the selected Ubuntu kernel package/config, the gfx1151 KFD CWSR fixes, and exact firmware package/blob hashes. See [control-tuple.yaml](../manifests/control-tuple.yaml).

## Candidate tuple

`PF-IR-03-CANDIDATE-7.14.0` is the current official-support candidate. Select one documented OS/kernel pair and the stable `amdrocm-core-sdk7.14-gfx1151` package lane. Preserve the package signing key fingerprint, InRelease/Packages metadata, package hashes, TheRock source pins, compiler resource directories, and artifact license/SBOM evidence. See [candidate-tuple.yaml](../manifests/candidate-tuple.yaml).

## Go/no-go gates

1. **No mixed lanes.** A process may resolve only one ROCm root, compiler resource tree, HSA runtime and device-library set.
2. **Fail closed on unknown artifact identity.** The raw 7.14 tarball remains non-promotable while its expected digest/signature is unauthenticated.
3. **Kernel/firmware are tuple members.** Confirm the gfx1151 KFD CWSR fixes and hash the firmware files actually loaded.
4. **Maturity labels remain literal.** Documented support, preview availability, known issues and unverified combinations are independent dimensions.
5. **USB4STREAM cannot redefine the compute baseline.** It is a separate kernel overlay with regression testing against a frozen userspace lane.

## Decision unblocked

- The external inputs for a legacy control and a separate 7.14 candidate are identified and machine-readable.
- OPEN-BASE-01 can import [open-base-01-external.yaml](../manifests/open-base-01-external.yaml) without silently coalescing lanes.
- Linux 7.2 USB4STREAM is plausible as a candidate overlay, but not as an official replacement for the gfx1151 compute baseline.
