# AMD Product Security authority map — Strix Halo / Ryzen AI MAX+ 395

## Current public PI floor

The highest exact public Strix Halo PI floor in the captured bulletins is `StrixHaloPI-FP11_1.0.0.2b`, released by AMD to OEMs on `2025-12-29` for CVE-2024-36345 and CVE-2024-36343. PI `1.0.0.2a`, released `2025-11-25`, is separately required for CVE-2026-0438, CVE-2025-54502, and CVE-2025-62626.

AMD's release-to-OEM date is not an end-user BIOS availability date. The OEM must integrate and publish a board-specific image.

## Exact target rows

| Authority | Exact product language | Target mitigation | State |
|---|---|---|---|
| AMD-SB-7033 | Ryzen AI Max+ / Strix Halo | PI 1.0.0.1 | `[FIX_AVAILABLE] [OPEN on target]` |
| AMD-SB-4013 | Ryzen AI Max 300 | PI 1.0.0.1 and 1.0.0.1c; KDS control | `[FIX_AVAILABLE] [OPEN on target]` |
| AMD-SB-4017 | Ryzen AI MAX / Ryzen AI Max 300 | PI 1.0.0.2a and 1.0.0.2b | `[FIX_AVAILABLE] [HOLD]` |
| AMD-SB-4017 | Ryzen AI Max 300 | CVE-2025-48516: no fix planned | Persistent residual risk |
| AMD-SB-7048 | Ryzen AI Max 300 | Updated PI enables Mixed Refresh Mode | `[OPEN]` exact Strix Halo PI row |
| AMD-SB-7054 | Ryzen AI Max 300 | PI 1.0.0.2a | `[FIX_AVAILABLE] [HOLD]` |
| AMD-SB-7055 | Ryzen AI Max 300 | PI 1.0.0.2a; software workarounds | `[FIX_AVAILABLE] [HOLD]` |
| AMD-SB-4015 | Strix Halo | Windows chipset-driver floors | Inventory required |
| AMD-SB-6024/6027 | Ryzen AI Max family | GPU-driver floors | OS-stack inventory required |
| AMD-SB-6010/6013 | RX 7000 / PRO W7000 mapped family | Driver plus non-default isolation mode | Security/performance decision required |
| AMD-SB-4016 | Ryzen AI Max 300 | RAID 9.3.3.245 | Conditional on RAID presence |

## Disclosure boundary

AMD omits products it believes are not affected from several tables, and bulletin information is subject to change and omission. `[NOT_LISTED_FOR_TARGET]` therefore remains a scoped observation, not an unaffected attestation. OEM integration defects, non-public advisories, and revision-specific errata remain `[OPEN]`.
