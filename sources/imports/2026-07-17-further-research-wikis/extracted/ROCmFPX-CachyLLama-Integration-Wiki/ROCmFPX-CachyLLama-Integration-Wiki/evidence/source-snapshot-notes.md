# Source Snapshot Notes

Evidence was captured on **2026-07-17** against the immutable heads recorded in `commit-lock.json`.

## Confirmed

- ROCmFPX and CachyLLama engine licenses are MIT at the reviewed heads.
- `fewtarius/llama-ai` is GPL-3.0 at the reviewed head.
- ROCmFPX already has a per-run SSD prompt cache with MTP target/draft/spec handling and failure tests.
- CachyLLama exposes source for persistent cache records, system-prefix cache, page manager, user isolation design, attention-only memory API, and expert telemetry.
- The small-AMD-GPU Vulkan submission fix is already present in canonical history.

## Not confirmed

- That `fewtarius/CachyLLama` is definitively the donor intended by the user; maintainer confirmation is required.
- Exact introduction SHAs and parentage for every selected donor-original capability.
- Runtime correctness or benchmark reproduction on local hardware.
- Production maturity of donor page-level SSD paging.
- Cross-platform portability/integrity of donor cache formats beyond reviewed source declarations.

## Interpretation rule

README performance figures are donor claims. Source declarations establish implementation intent, not acceptance. This design intentionally proposes independent tests and a new canonical format.
