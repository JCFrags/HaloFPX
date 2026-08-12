---
section_id: "85"
title: "Upstream Watch Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["HaloFPX research sources, donor repositories, and upstream dependencies"]
  software_versions: ["procedural; use immutable revisions from the active baseline"]
  hardware_revisions: ["two Strix Halo nodes after inventory"]
related_sections: ["02", "05", "11", "15", "23", "29", "72", "84", "86"]
---

# Procedures and checks

## Safe watch cycle

Prerequisites: Internet access for source review; Git and an HTTPS client. Root is **not** required. Store credentials outside the repository. Fetching is read-only; baseline promotion is a separate reviewed change.

1. **Load authority:** read this section, the affected section manifest/claims/sources, and linked decisions.
2. **Poll:** record feed ID, URL, UTC time, HTTP status/ETag where available, moving ref, and immutable revision. A failed poll produces `monitoring-degraded`, never “unchanged.”
3. **Preserve candidate:** retain exact source URL/SHA/tag, release date, license, and checksum/signature metadata. Do not copy copyrighted material beyond the evidence needed.
4. **Diff semantically:** restrict the first pass to relevant paths/symbols and inspect tests/docs. Record changed assumptions; do not infer runtime results from commit text.
5. **Map impact:** list claim IDs, section IDs, experiment IDs, baseline tuple IDs, and decisions that consume the changed fact.
6. **Triage:** apply P0-P3 severity from [design implications](design_implications.md#severity-and-service-levels). Mark affected published claims `stale-pending-review` or `contradicted` without deleting prior applicability.
7. **Revalidate:** perform source-level checks, then the smallest safe on-machine experiment needed. Preserve environment, commands, raw outputs, and failures.
8. **Review/promote:** reviewer chooses accept, revise, defer, reject, or no-applicability. Publish with supersession links; create/modify an ADR only through Section 86 governance.
9. **Close:** verify links/counts/front matter, update review due dates, and confirm rollback remains usable.

## Reproducible remote-ref checks

PowerShell, no root, read-only:

```powershell
$repos = @(
  'https://github.com/ggml-org/llama.cpp.git',
  'https://github.com/charlie12345/ROCmFPX.git',
  'https://github.com/fewtarius/CachyLLama.git',
  'https://github.com/fewtarius/llama-ai.git',
  'https://github.com/torvalds/linux.git',
  'https://github.com/ROCm/ROCm.git',
  'https://github.com/ROCm/rocm-systems.git',
  'https://github.com/axboe/liburing.git',
  'https://gitlab.freedesktop.org/mesa/mesa.git'
)
$repos | ForEach-Object { git ls-remote --symref $_ HEAD }
```

Record output with UTC time. `ls-remote` proves only advertised ref state; it does not verify a build, signature, license, or runtime.

Useful immutable tag checks:

```powershell
git ls-remote --tags https://github.com/torvalds/linux.git 'refs/tags/v7.2-rc3*'
git ls-remote --tags https://github.com/ROCm/rccl.git 'refs/tags/rocm-7.2.3*' # frozen old-repo baseline only
git ls-remote --tags https://gitlab.freedesktop.org/mesa/mesa.git 'refs/tags/mesa-26.1.5*'
git ls-remote --tags https://github.com/axboe/liburing.git 'refs/tags/liburing-2.15*'
```

Annotated tag objects and peeled commits are different IDs; preserve both. Verify release signatures/checksums under the project's eventual signing policy.

## Source-code/query recipes

| Domain | First-pass query or diff |
|---|---|
| Linux USB4/amdgpu | kernel log for `drivers/thunderbolt`, `drivers/gpu/drm/amd`, `io_uring`; lore queries `thunderbolt-stream OR USB4STREAM OR xdomain`, `gfx1151 amdgpu reset` |
| ROCm/HIP/HSA | release notes/support matrix plus exact tag diff for `gfx1151`, coherence/fences, allocation, graphs, compiler targets, known issues |
| Mesa/RADV | release notes and RADV/ACO source/issues for `gfx1151 OR GFX11.5`, compute, subgroup, coherent memory, hang/reset/regression |
| llama.cpp | `repo:ggml-org/llama.cpp (ROCm OR HIP OR Vulkan OR RPC OR MTP OR speculative OR state OR cache OR GGUF) is:pr` plus relevant path diff |
| ROCmFPX/CachyLLama | exact head versus pin; custom GGML/GGUF IDs/layouts, backend dispatch, converter, state/prompt cache, user/session paths and tests |
| RCCL | active `ROCm/rocm-systems/projects/rccl` scoped history plus exact old baseline tag for socket selection, plugin ABI, DMA-BUF/pointer support, timeout/error/failure semantics |
| io_uring | kernel/liburing source and lore for registered resources, direct I/O alignment, async cancel, CQE generation/late completion, teardown |
| Models | publisher API/repo SHA plus `config.json`, tokenizer files, chat template, license/gating, weight index, converter/runtime architecture diffs |
| Hardware | exact OEM SKU/GUID + BIOS/EC/PD/retimer/cable; AMD Product Security and client revision guidance; LVFS metadata |

## On-machine revalidation tasks

Run only on the intended nodes with an experiment card from Section 84. Root requirements depend on the probe; inventory commands should be read-only.

`EX85-NN` IDs below are Section 85 requirement aliases, not executable card IDs. Before a run, explicitly link each alias to an existing or newly approved Section 84 `HLX-EXP-YYYYMMDD-NNN` card. No such mapping is asserted by this section; similarity of titles or numbering is insufficient.

| ID | Trigger and minimum proof |
|---|---|
| EX85-01 | Kernel/firmware change: paired hardware IDs, kernel/config/modules, loaded firmware filenames/hashes, XDomain/USB4 topology, link counters, HIP and Vulkan smoke |
| EX85-02 | ROCm/Mesa/RCCL change: exact packages/libraries, `rocminfo`, compiler target, `vulkaninfo` JSON, backend-op tests, two-rank correctness/error handling |
| EX85-03 | llama.cpp/donor change: locked clean builds, unit/backend/server/state tests, custom format golden vectors, patch range-diff/provenance map |
| EX85-04 | Model change: artifact hashes, tokenizer/template round-trip, conversion/tensor audit, CPU then HIP/Vulkan smoke, deterministic state restore and quality gate |
| EX85-05 | io_uring/cache change: capability probe, aligned I/O/error/cancel/late-CQE tests, crash/corruption rejection, resource-leak and pressure checks |
| EX85-06 | BIOS/firmware/erratum: signed package provenance, pre/post inventory, rollback proof, cold/warm boot, USB4 and GPU reset/recovery, sustained soak |

No destructive firmware, cache-corruption, cable-pull, or fault-injection test runs on production data. Use disposable artifacts and prove recovery first.

## Automated acceptance checks

- Reject a feed record missing `retrieved_at`, immutable locator, applicability, or source authority.
- Flag duplicate source IDs, moving URLs without pinned revision, expired `review_due`, or a changed ref without impact disposition.
- Block promotion when a source conflict is unresolved, a required signature/hash fails, machine tuple differs, or rollback is unproven.
- Block cache acceptance on any fingerprint/integrity mismatch; recompute instead.
- Require source count/open-question count to match `section.yaml`, all relative links to resolve, YAML to parse, and `git diff --check` to pass.
