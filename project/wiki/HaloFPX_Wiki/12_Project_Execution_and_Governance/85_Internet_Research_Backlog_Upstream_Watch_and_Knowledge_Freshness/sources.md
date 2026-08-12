---
section_id: "85"
title: "Internet Research and Freshness Sources"
status: "verified"
last_verified: "2026-07-18"
applies_to:
  repositories: ["HaloFPX wiki and watched upstream/donor repositories"]
  software_versions: ["source revisions recorded below"]
  hardware_revisions: ["product-specific hardware sources pending exact BOM"]
related_sections: ["02", "04", "11", "13", "15", "18", "23", "24", "29", "50", "62"]
---

# Sources

Internet sources S85-01 through S85-17 were accessed on 2026-07-17. S85-18
records the separately preserved 2026-07-18 research return intake. Moving refs
are observations, not immutable authorities.

### S85-18 - PF-IR 2026-07-18 preserved return intake and review

- **Origin/revision:** eleven research-agent ZIPs PF-IR-01 through PF-IR-11,
  preserved with SHA-256 hashes under
  `sources/imports/2026-07-18-pre-fork-internet-research/`; review at
  `reviews/intake/2026-07-18__pf-ir-returns__review__v01.md`.
- **Supports:** complete return status, provenance of the imported packages,
  package-to-local-gate routing, and PF-IR-04 licensing evidence.
- **Limit:** aggregate imported research and local review, not a substitute for
  each package's primary sources, exact-source revalidation, machine evidence,
  legal approval, or a product decision.

## Project and repository feeds

### S85-01 - Existing HaloFPX applicability and gap inventory

- **Origin/revision:** local `wiki/HaloFPX_Wiki/` completed section manifests and claims as present at task time; especially Sections 11, 13-15, 18, 23-24, 29, 50, and 62.
- **Supports:** frozen pins, existing applicability, open questions, and cross-section impact routes.
- **Limit:** synthesized local wiki; each material fact remains governed by its original cited source. Not counted as new Internet proof.

### S85-02 - llama.cpp repository and release feeds

- **Publisher/URLs:** ggml-org; <https://api.github.com/repos/ggml-org/llama.cpp>, <https://github.com/ggml-org/llama.cpp/releases.atom>, <https://github.com/ggml-org/llama.cpp/releases/tag/b10054>.
- **Revision/date:** observed `master` `788e07dc91d266ad3162a1ce9037665656269689`; ordered release feed showed `b10056` at `b85833e934123f373a8dc087316e385b28c98cc0`, while `/releases/latest` returned `b10054` at `ac2557cb24def295888ef47f1a35b401d978c510`, published 2026-07-17 00:29Z.
- **Supports:** live head/release snapshot and convenience-endpoint discrepancy.
- **Limit:** Atom entry time, GitHub `published_at`, tag commit time, and checked-at time are different fields; GitHub metadata does not qualify HaloFPX behavior.

### S85-03 - ROCmFPX repository feed

- **Publisher/URL:** charlie12345; <https://api.github.com/repos/charlie12345/ROCmFPX>.
- **Revision/date:** observed `main` `a5605a72768c6562241b248e268e33dc92787394`, commit timestamp 2026-07-17 02:34:40Z; no latest release returned.
- **Supports:** donor watch anchor.
- **Limit:** moving ref; experimental claims require code/test review.

### S85-04 - CachyLLama repository feed

- **Publisher/URL:** fewtarius; <https://api.github.com/repos/fewtarius/CachyLLama>.
- **Revision/date:** observed `master` `6be745998f568e379ea197fcf827baec73ff9940`, commit timestamp 2026-07-09 00:17:28Z; no latest release returned.
- **Supports:** persistent-cache donor watch anchor.
- **Limit:** fork claims and benchmarks are not HaloFPX measurements.

### S85-05 - llama-ai repository feed

- **Publisher/URL:** fewtarius; <https://api.github.com/repos/fewtarius/llama-ai>.
- **Revision/date:** observed `main` `1017f3dfdce3ca2b06aa9007b23295db3bb35722`, commit timestamp 2026-07-09 00:21:33Z; no latest release returned.
- **Supports:** wrapper/submodule and agent-serving watch anchor.
- **Limit:** GPL-3.0-or-later project; inclusion remains a licensing/design decision.

## Kernel, graphics, compute, and I/O feeds

### S85-06 - Linux kernel release feed

- **Publisher/URL:** kernel.org; <https://www.kernel.org/> and <https://cdn.kernel.org/>.
- **Revision/date:** mainline `7.2-rc3`, released 2026-07-12; tag peeled to `a13c140cc289c0b7b3770bce5b3ad42ab35074aa` by official Git remote.
- **Supports:** exact mainline release observation and RC status.
- **Limit:** not distro packaging, installed source, or HaloFPX qualification.

### S85-07 - Linux USB4/Thunderbolt source and documentation

- **Publisher/URLs:** Linux kernel; <https://docs.kernel.org/admin-guide/thunderbolt.html>, <https://github.com/torvalds/linux/tree/fce2dfa773ced15f27dd27cd0b482a7473cdcf2a/drivers/thunderbolt>.
- **Revision/date:** observed `master` `fce2dfa773ced15f27dd27cd0b482a7473cdcf2a`; latest docs include USB4STREAM/configfs and `/dev/tbstreamX`.
- **Supports:** authoritative Linux interface/watch paths and OEM/fwupd warning route.
- **Limit:** source presence does not prove target module/config/controller behavior or application security.

### S85-08 - ROCm 7.2.3 release notes and history

- **Publisher/URLs:** AMD; <https://rocm.docs.amd.com/en/docs-7.2.3/about/release-notes.html>, <https://rocm.docs.amd.com/en/develop/release/versions.html>.
- **Revision/date:** ROCm 7.2.3, released 2026-05-04; Git tag `14f8138863403a26e0caef6671cfab9b09aa636e`; observed `develop` `e6331e174c746d38b25a6d14fece05a2505637b6`, commit time 2026-07-17 01:39:09Z.
- **Supports:** production release, known-issue/component feed, release date.
- **Limit:** umbrella release contains independently versioned components; data-center firmware table is not a client APU BOM.

### S85-09 - ROCm 7.2.1 Radeon/Ryzen gfx1151 support matrix

- **Publisher/URL:** AMD; <https://rocm.docs.amd.com/projects/radeon-ryzen/en/docs-7.2.1/docs/compatibility/compatibilityryz/native_linux/native_linux_compatibility.html>.
- **Revision/date:** ROCm 7.2.1 documentation snapshot; the GPU matrix lists `gfx1151` and Ryzen AI Max 300-series products, and the framework matrix lists PyTorch 2.9.1 with ROCm 7.2.1 on Ubuntu 24.04.4.
- **Supports:** official 7.2.1 Ryzen/gfx1151 control lane and its documented OS/framework boundary.
- **Limit:** framework validation does not prove ROCmFPX, Vulkan, USB4STREAM, or the intended combined tuple.

### S85-10 - RCCL documentation and active-source authority

- **Publisher/URLs:** AMD/ROCm; <https://rocm.docs.amd.com/projects/rccl/en/docs-7.2.3/>, <https://github.com/ROCm/rccl/tree/develop>, <https://github.com/ROCm/rocm-systems/tree/develop/projects/rccl>.
- **Revision/date:** documentation release 2.27.7 for ROCm 7.2.3; active `rocm-systems` head observed `27b4e4dd4438e205c3c9163efe4084b890bbb08e`, 2026-07-17 02:03:47Z; latest RCCL-scoped commit `7d981a46d3170c2dc50dfaf6002666119534d548`, 2026-07-16 16:42:01Z.
- **Supports:** official API documentation and the declared migration from deprecated `ROCm/rccl` to active `ROCm/rocm-systems/projects/rccl`.
- **Limit:** no proof of two-host USB4 behavior or custom network-plugin suitability; monorepo head may change outside RCCL.

### S85-11 - RCCL exact source tag

- **Publisher/URL:** ROCm; <https://github.com/ROCm/rccl/tree/96a25b5fd6f73fba58c7d83eb57cf19a50230aa4>.
- **Revision/date:** annotated `rocm-7.2.3` tag object `34eae42322725875d978cef7531fedaeb49b0b9b`, peeled commit `96a25b5fd6f73fba58c7d83eb57cf19a50230aa4`.
- **Supports:** immutable RCCL code anchor.
- **Limit:** this is the frozen old-repository baseline; default branch `develop_deprecated` is not the active authority.

### S85-12 - Mesa releases and calendar

- **Publisher/URLs:** Mesa; <https://docs.mesa3d.org/relnotes/26.1.5.html>, <https://docs.mesa3d.org/release-calendar.html>, <https://gitlab.freedesktop.org/mesa/mesa/-/tree/6a02618ccf6c5651ecb9cccbde571eb61fd73592>.
- **Revision/date:** Mesa stable 26.1.5, released 2026-07-15; tag object `3c008c397d06d707cd6cdda3fdad65ce4efe6e2b`, peeled commit `6a02618ccf6c5651ecb9cccbde571eb61fd73592`; tarball SHA-256 `79e421c7ce18cd9e790b8375920325779f10798630bf30e0b22f1a21c8617122`; candidate 26.2.0-rc1 target `57017725151dc7e96ba82eeedc35baf1fc13911f`; observed `main` `20f4f9f45057559475600b60364b60643011990f`, commit time 2026-07-17 04:06:03Z.
- **Supports:** current stable release, schedule, exact source/checksum anchors.
- **Limit:** Vulkan API headline and generic RADV fixes are not target capability or correctness proof.

### S85-13 - liburing repository, release, and io_uring discussion feed

- **Publisher/URLs:** Jens Axboe/liburing; <https://github.com/axboe/liburing/releases/tag/liburing-2.15>, <https://github.com/axboe/liburing/tree/d41bf9220ec39277ff235379e9089d9e0fd6c2a5>, <https://lore.kernel.org/io-uring/>.
- **Revision/date:** liburing 2.15, released 2026-06-29; annotated tag object `84bb497ca2f9d24ca0b9e5646fb6a05e72c0f04e`, peeled commit `d41bf9220ec39277ff235379e9089d9e0fd6c2a5`; observed `master` `e50e32a6b9030faba2e30fa0ba999571a0cffe28`, commit time 2026-07-01 12:24:23Z; io_uring maintainer `for-next` observed `6dbcc40ec7aa17ed3dd1f798e4201e75ab7d7447` with commit timestamp **[OPEN]**.
- **Supports:** userspace helper release/source and primary development discussion feed.
- **Limit:** liburing explicitly spans different kernel versions; probe actual op/features.

### S85-14 - linux-firmware feed

- **Publisher/URL:** kernel.org; <https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git/>.
- **Revision/date:** observed `main` `924d73c9a2501a256d18a26cbe640548c70b3a9a`.
- **Supports:** upstream firmware change signal.
- **Limit:** installed package/file hashes and OEM BIOS/EC/PD/retimer firmware are separate authorities.

## Model, hardware, and governance feeds

### S85-15 - Publisher model repository APIs

- **Publisher/URLs/revisions:** Hugging Face publisher repositories: [Qwen2.5-Coder-32B-Instruct](https://huggingface.co/api/models/Qwen/Qwen2.5-Coder-32B-Instruct) `381fc969f78efac66bc87ff7ddeadb7e73c218a7`; [Qwen3-30B-A3B](https://huggingface.co/api/models/Qwen/Qwen3-30B-A3B) `ad44e777bcd18fa416d9da3bd8f70d33ebb85d39`; [DeepSeek-V3](https://huggingface.co/api/models/deepseek-ai/DeepSeek-V3) `e815299b0bcbac849fa540c768ef21845365c9eb`; [Mistral Small 3.1](https://huggingface.co/api/models/mistralai/Mistral-Small-3.1-24B-Instruct-2503) `68faf511d618ef198fef186659617cfd2eb8e33a`; [Nemotron-3-Nano](https://huggingface.co/api/models/nvidia/NVIDIA-Nemotron-3-Nano-30B-A3B-BF16) `cbd3fa9f933d55ef16a84236559f4ee2a0526848`.
- **Supports:** current publisher head/gating/license metadata for Section 29 candidates.
- **Limit:** repository APIs do not prove downloaded weight hashes, conversion, quality, runtime support, or license interpretation.

### S85-16 - ROCm Core SDK 7.14.0 release notes

- **Publisher/URL:** AMD/ROCm; <https://github.com/ROCm/ROCm/releases/tag/rocm-7.14.0>.
- **Revision/date:** ROCm Core SDK 7.14.0, tag `830cc1b5e90d7da1b07e39113d7a5c95f3e687a1`, released 2026-07-16 03:28:58Z.
- **Supports:** separate TheRock/Core SDK lane, explicit versioning discontinuity, and a current candidate relevant to gfx1151.
- **Limit:** it is not an automatic replacement or qualification result for the 7.2.3 HaloFPX research baseline.

### S85-17 - AMD Product Security and Agent Harness promotion authority

- **Publishers/URLs/paths:** AMD Product Security, <https://www.amd.com/en/resources/product-security.html>; local `C:/Users/britt/Documents/Agent_Harness/AGENTS.md` and `guide/architecture.md`.
- **Revision/date:** AMD bulletin index observed 2026-07-17; Agent Harness local authority read 2026-07-17.
- **Supports:** security feed and evidence -> candidate/review -> publication/stale governance.
- **Limit:** AMD bulletins require exact SKU mapping; local governance is a project process, not technical compatibility evidence.

## Conflicts and limitations

- **[VERIFIED]** Current remote heads match Section 11 pins, but moving refs may change after access.
- **[VERIFIED]** Ordered llama.cpp release feed and `/releases/latest` disagreed during the observation; “latest” is not a stable locator.
- **[VERIFIED]** RCCL active-source authority migrated; the deprecated repository remains useful only for frozen baselines.
- **[OPEN]** The exact OEM support and client revision-guide authorities cannot be selected until the node BOM is recorded.
- **[OPEN]** Public source feeds may not expose proprietary USB-IF/PCI-SIG errata or OEM firmware internals.
- **[RECOMMENDATION]** Preserve future feed observations as append-only evidence; never rewrite this snapshot to imply it was always current.
