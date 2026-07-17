---
section_id: "85"
title: "Knowledge Freshness Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories:
    - "HaloFPX research sources and pinned donor/upstream repositories"
  software_versions:
    - "live observations recorded in S85-02 through S85-16"
  hardware_revisions:
    - "exact two-node hardware identity pending"
related_sections: ["02", "04", "11", "13", "15", "18", "23", "24", "29", "50", "62"]
---

# Facts and constraints

## Observed upstream snapshot

All moving-ref observations below were checked 2026-07-17. A commit date is not a freshness guarantee.

| Domain | Exact observation | Evidence state and limit |
|---|---|---|
| llama.cpp | `master` `788e07dc91d266ad3162a1ce9037665656269689`; ordered release feed showed `b10056` at `b85833e934123f373a8dc087316e385b28c98cc0`, while `/releases/latest` returned `b10054` at `ac2557cb24def295888ef47f1a35b401d978c510` | **[VERIFIED]** Remote/feed snapshot. “Latest” endpoint and ordered feed disagreed; preserve both and never use the endpoint label as durable identity [S85-02]. |
| ROCmFPX | `main` `a5605a72768c6562241b248e268e33dc92787394`; no release returned | **[VERIFIED]** Snapshot; absence of a GitHub release is not abandonment [S85-03]. |
| CachyLLama / llama-ai | `master` `6be745998f568e379ea197fcf827baec73ff9940`; `main` `1017f3dfdce3ca2b06aa9007b23295db3bb35722`; no releases returned | **[VERIFIED]** Snapshot; dated branches and moving refs remain outside a release contract [S85-04, S85-05]. |
| Linux | `v7.2-rc3`, peeled commit `a13c140cc289c0b7b3770bce5b3ad42ab35074aa`, 2026-07-12; stable `7.1.3`, 2026-07-04; observed `master` `fce2dfa773ced15f27dd27cd0b482a7473cdcf2a` | **[VERIFIED]** Mainline development and stable-release state are distinct; neither proves target qualification [S85-06, S85-07]. |
| ROCm | `7.2.1` Ryzen/gfx1151 support control; `7.2.3`, tag `14f8138863403a26e0caef6671cfab9b09aa636e`, released 2026-05-04 research baseline; separate Core SDK/TheRock `7.14.0`, tag `830cc1b5e90d7da1b07e39113d7a5c95f3e687a1`, released 2026-07-16 unqualified candidate; observed `develop` `e6331e174c746d38b25a6d14fece05a2505637b6` | **[VERIFIED]** Control, research baseline, candidate, and development head are distinct; none proves the installed tuple or automatically replaces another lane [S85-08, S85-09, S85-16]. |
| RCCL | ROCm 7.2.3 component `2.27.7`, old tag peeled to `96a25b5fd6f73fba58c7d83eb57cf19a50230aa4`; active authority is now `ROCm/rocm-systems/projects/rccl`, latest scoped commit observed `7d981a46d3170c2dc50dfaf6002666119534d548` | **[VERIFIED]** `ROCm/rccl` declares its move/deprecation; default-branch polling there is misleading [S85-10, S85-11]. |
| Mesa | stable `26.1.5`, tag peeled to `6a02618ccf6c5651ecb9cccbde571eb61fd73592`, released 2026-07-15; `26.2.0-rc1` candidate `57017725151dc7e96ba82eeedc35baf1fc13911f`; observed `main` `20f4f9f45057559475600b60364b60643011990f` | **[VERIFIED]** Stable, RC, and development lanes are separate; RADV/gfx1151 behavior is not proven by any version label [S85-12]. |
| liburing | `2.15`, annotated tag object `84bb497ca2f9d24ca0b9e5646fb6a05e72c0f04e`, peeled commit `d41bf9220ec39277ff235379e9089d9e0fd6c2a5`, released 2026-06-29; observed `master` `e50e32a6b9030faba2e30fa0ba999571a0cffe28` | **[VERIFIED]** Library release/current-ref snapshot; kernel feature probes remain required [S85-13]. |
| linux-firmware | observed `main` `924d73c9a2501a256d18a26cbe640548c70b3a9a` | **[VERIFIED]** Moving repository snapshot; not proof of installed firmware files or hashes [S85-14]. |
| model catalog | The five publisher repository heads still match Section 29 pins: `381fc969`, `ad44e777`, `e815299b`, `68faf511`, `cbd3fa9f` | **[VERIFIED]** Model API snapshot; weights, tokenizer, license, conversion, and runtime qualification remain separate [S85-15]. |

### Lane-aware ROCm baseline vocabulary

| Term used in Section 85 | Exact lane/revision | What it means | What it does not mean |
|---|---|---|---|
| Ryzen-supported control | ROCm `7.2.1` documentation lane | **[VERIFIED]** AMD's versioned matrix lists `gfx1151`, the relevant Ryzen AI Max family, Ubuntu, and a PyTorch combination; use it as the documented control boundary [S85-09]. | Not the installed tuple, a HaloFPX qualification, or evidence for USB4STREAM, ROCmFPX, Vulkan, RCCL, or arbitrary workloads. |
| Research baseline | ROCm `7.2.3` at `14f8138863403a26e0caef6671cfab9b09aa636e` | **[VERIFIED]** Immutable umbrella source selected for research comparison and component inventory [S85-08]. | Not described here as a qualified production or installed baseline. |
| Unqualified candidate | ROCm Core SDK/TheRock `7.14.0` at `830cc1b5e90d7da1b07e39113d7a5c95f3e687a1` | **[VERIFIED]** A distinct candidate lane observed for evaluation [S85-16]. | Not a numeric successor that can silently replace 7.2.x and not qualified on either target node. |
| Installed tuple | **[OPEN]** | Must be established from both nodes' package versions, source/package provenance, libraries, kernel/driver/firmware, and machine evidence. | Must not be inferred from documentation, repository pins, or intended configuration. |

**[RECOMMENDATION]** Never compare ROCm lane suitability with a naive numeric “newer than” rule. Store the lane name, exact revision, component tuple, role (`control`, `research-baseline`, `candidate`, or `installed`), and qualification state independently.

## Trusted feed registry and cadence

Cadence is a project recommendation; feed authority is source-backed.

| Feed ID | Scope and primary feed | Poll cadence | High-value query/diff |
|---|---|---:|---|
| F85-LINUX | kernel.org releases; torvalds/linux tags and `drivers/thunderbolt`, `drivers/gpu/drm/amd`, `io_uring`; docs; lore lists | daily during RC/backport work, weekly otherwise | `USB4STREAM`, `thunderbolt-stream`, `XDomain`, `amdgpu gfx1151`, `KFD`, `reset`, `IOMMU`, `io_uring cancel` |
| F85-ROCM | AMD release history, release/known-issue pages, Radeon/Ryzen support matrices, exact ROCm/HIP/HSA tags | weekly; immediately on release/security notice | `gfx1151`, Strix Halo, kernel, Ubuntu, coherence, graph capture, memory allocation, known issue |
| F85-MESA | Mesa release notes/calendar, signed tarball hashes, exact GitLab tags, RADV/ACO source and issues | weekly; daily while qualifying a candidate | `RADV gfx1151`, `GFX11.5`, device coherent memory, hang, reset, compute, subgroup, regression |
| F85-LLAMA | llama.cpp commits/releases/PRs/issues and relevant paths | daily digest, weekly semantic triage | `ROCm OR HIP OR Vulkan OR RPC OR MTP OR speculative OR state OR cache OR GGUF OR <model>` |
| F85-DONORS | ROCmFPX, CachyLLama, llama-ai heads/branches/PRs/issues | daily head check, weekly feature/provenance diff | custom type IDs, quant block layout, HIP/Vulkan parity, prompt/state cache, user/session isolation |
| F85-RCCL | `ROCm/rocm-systems/projects/rccl` scoped commits, RCCL docs, old exact baseline tags | weekly; before tuple qualification | socket transport, `NCCL_SOCKET_IFNAME`, net plugin ABI, DMA-BUF, communicator failure, two-rank |
| F85-URING | kernel MAINTAINERS, io_uring maintainer `for-next`, liburing releases/source/manpages, lore archive | monthly; weekly during HaloKV I/O work | registered files/buffers, direct I/O, cancellation, late CQE, ring resize, resource teardown |
| F85-MODELS | publisher repositories/APIs plus immutable config, tokenizer, template, license, weight manifests | weekly for pinned candidates; before every conversion | SHA change, config/architecture, tokenizer/chat template, license/gating, safetensors index |
| F85-HW | exact OEM support/BIOS feed after BOM; LVFS/fwupd; linux-firmware; AMD Product Security and client revision guidance | weekly security scan; monthly OEM scan; before firmware change | exact product/SKU/GUID, BIOS/EC/PD/USB4/retimer, AGESA/PI, CVE, erratum, rollback |

Issue and PR feeds are leads, not verified compatibility. Secondary benchmarks may generate a reproducer but may not promote a HaloFPX fact.

## Freshness classes and stale detection

| Class | Examples | Review due | Stale or revalidation trigger |
|---|---|---:|---|
| V0 emergency | security, corruption, data loss, unsafe firmware | same day | bulletin/revision, confirmed corruption, withdrawal, exploitability change |
| V1 high churn | moving code refs, kernel RCs, llama.cpp/donor heads, model APIs | 7 days | ref changes, force-move, relevant path/symbol/config change |
| V2 release tuple | ROCm/RCCL/Mesa/liburing/distro packages | 30 days | new release, known-issue revision, support-matrix or package/backport change |
| V3 hardware | BIOS/EC/PD/retimer/cable/OEM advisories | 30 days | exact BOM change, firmware offer/withdrawal, new erratum or board revision |
| V4 stable specification | pinned source blobs, published API/spec revisions | 180 days | superseding revision, erratum, contradiction, implementation divergence |

**[RECOMMENDATION]** Every volatile claim record needs `source_id`, immutable locator, observed moving ref, `retrieved_at` UTC, content hash/ETag when available, applicability tuple, claim IDs, freshness class, `review_due`, reviewer, and result. A missed due date marks the claim `stale-pending-review`; it does not prove the claim false. An unreachable feed marks monitoring degraded and must not be interpreted as “no change.”

**[RECOMMENDATION]** Immutable source text does not expire merely with age. Its applicability expires when the product baseline, dependency tuple, model artifact, hardware revision, or decision premise changes.

## Revalidation triggers

- New kernel/ROCm/Mesa/RCCL release, distro backport, config change, or firmware payload.
- Change under relevant upstream paths, custom type IDs/layouts, cache/state serialization, protocol, converter, tokenizer/template, or model config/license.
- Security bulletin, known-issue revision, regression/withdrawal, failed signature/hash, or moved/deleted tag.
- Machine inventory drift, new hardware/BIOS/cable, unexplained performance/correctness change, GPU reset, link retrain, I/O error, or cache rejection.
- ADR review, baseline promotion, deployment/rollback, or a claim consumed outside its recorded applicability.

No release observation by itself is an upgrade decision or **[MEASURED]** result.
