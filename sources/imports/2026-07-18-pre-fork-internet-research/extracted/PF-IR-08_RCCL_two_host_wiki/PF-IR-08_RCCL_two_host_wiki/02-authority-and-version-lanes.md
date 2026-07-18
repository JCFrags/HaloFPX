# Authority and version lanes

## Pinned map

| ROCm lane | RCCL | Source tag/ref | Exact commit | Relevant boundary |
|---|---:|---|---|---|
| 6.4.3 | 2.22.3 | `rocm-6.4.3` | `2f7ac66cd64c68d4af8bb4562ce193778a7e470e` | Older API/ABI; no projection from later lanes. |
| 7.0.2 | 2.26.6 | `rocm-7.0.2` | `01dfdacf4278d4369dc8c34c8877f4210f1f486b` | Separate release contract. |
| 7.1.1 | 2.27.7 | `rocm-7.1.1` | `bf3ebf549fec376521b7d35f09a67f67071c96d3` | Component version matches 7.2.x, tag differs. |
| 7.2.1 | 2.27.7 | `rocm-7.2.1` | `96a25b5fd6f73fba58c7d83eb57cf19a50230aa4` | Stable public header: Net v10, no `ncclTimeout`, revoke, or grow. |
| 7.2.2 | 2.27.7 | `rocm-7.2.2` | `96a25b5fd6f73fba58c7d83eb57cf19a50230aa4` | Same captured source commit. |
| 7.2.3 | 2.27.7 | `rocm-7.2.3` | `96a25b5fd6f73fba58c7d83eb57cf19a50230aa4` | Same captured source commit. |
| 7.2.4 | 2.27.7 | `rocm-7.2.4` | `96a25b5fd6f73fba58c7d83eb57cf19a50230aa4` | Stable audit baseline. |
| active `develop` | 2.30.4 | monorepo head | `801a9ca2ad8940ac7cd7d571163e003f3a3d6cab` | Net v12; timeout/revoke/grow docs and tests. |

## Installed-library rule

Before any experiment, capture:

```bash
readlink -f /opt/rocm/lib/librccl.so*
sha256sum /opt/rocm/lib/librccl.so*
strings /opt/rocm/lib/librccl.so | grep -E 'RCCL|NCCL' | head
export NCCL_DEBUG=VERSION
```

Package labels can diverge from a custom build or an overridden `LD_LIBRARY_PATH`. The runtime log and library hash decide which contract is being tested.

## gfx1151 timing

Merged PR #3415 enabled gfx1151 in the monorepo on February 26, 2026. Its test statement was single-GPU only and explicitly left multi-GPU/multi-node validation pending. Therefore a 7.2.x package containing RCCL 2.27.7 cannot be assumed to contain that later active-branch enablement merely because the broader ROCm 7.2 Ryzen matrix lists gfx1151.
