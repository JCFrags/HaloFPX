# Official Ryzen/Linux support matrices

## Core SDK 7.14.0 — current matrix

[DOCUMENTED_SUPPORT] The current 7.14 matrix lists Strix Halo/RDNA 3.5 `gfx1151` APUs, including 300-series and 400-series Ryzen AI MAX/MAX PRO parts. The support boundary for Ryzen APUs is:

| OS | Kernel | Driver |
|---|---|---|
| Ubuntu 26.04 | GA 7.0 | inbox `amdgpu`/KFD from the supported Ubuntu release |
| Ubuntu 24.04.4 | HWE 6.17 | inbox `amdgpu`/KFD from the supported Ubuntu release |

The matrix does not assign Ryzen an Instinct-style amdgpu package or PLDM firmware version. Kernel and firmware provenance must therefore be captured from the chosen Ubuntu image.

## 7.14 maturity is not a single Boolean

- [DOCUMENTED_SUPPORT] `gfx1151` is in the official compatibility selector.
- [PREVIEW_AVAILABILITY] AMD's transition guide explicitly calls Core SDK 7.14.0 a preview.
- [NOT_RELEASE_READY] TheRock's `SUPPORTED_GPUS.md` records build and sanity success but leaves Release Ready blank for Linux gfx1151.
- [MATURITY_CONFLICT] These statements coexist. The dossier does not erase one to simplify the other.

## Legacy ROCm 7.2 Ryzen matrix

| Field | Captured official value |
|---|---|
| OS | Ubuntu 24.04.3 |
| OS caveat | preliminary support using the Ubuntu 24.04.2 installer |
| GPU targets | gfx1150, gfx1151 |
| gfx1151 products listed | Ryzen AI Max+ 395, Ryzen AI Max 390, Ryzen AI Max 385 |
| Framework tuple | PyTorch 2.9, ROCm 7.2, Python 3.12 |
| Framework comment | official production support |
| Officially validated data type | FP16 only |

[LANE_BOUNDARY] ROCm 7.2.4 release notes are an Instinct-focused quality update. For Ryzen platform qualification, the Ryzen matrix—not the generic 7.2.4 patch note—is the controlling source.

## Documentation drift record

[DOCUMENTATION_DRIFT] A 7.14 release-note table references an Ubuntu 24.04 OEM kernel minimum of `6.14.0-1018` or newer, while the current compatibility matrix specifies Ubuntu 24.04.4 with HWE 6.17. Qualification should use the current matrix. The older/minimum statement is retained for audit rather than silently reconciled.

## Unverified combinations

The following are not documented support tuples in the captured sources:

- Core SDK 7.14.0 on generic upstream Linux 7.1.3.
- Core SDK 7.14.0 on Linux 7.2-rc with USB4STREAM.
- Legacy 7.2.4 packages combined with the 7.14 compiler or libraries.
- A 7.14 raw tarball accepted only because its locally computed hash is stable across two nodes.
