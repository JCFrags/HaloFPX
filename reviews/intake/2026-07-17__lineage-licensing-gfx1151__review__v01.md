---
type: intake-review
status: proposed
created: 2026-07-17
reviewer: research_24_28
targets:
  - rocmfpx_lineage_licensing_wiki
  - strix-halo-gfx1151-llm-wiki-2026.07.17
overall_decision: revise
risk: high
approval_required: human
canonical_wiki_modified: false
---

# ROCmFPX lineage, licensing, and gfx1151 intake review

## Outcome

**REVISE before promotion.** Both packages are well-structured, internally intact research inputs and contain several promotable primary-source facts. Neither package is an approved software baseline, a release-license determination, or machine validation of its proposed build profiles.

The lineage package is strongest where it records exact Git objects and literal license notices. The gfx1151 package is strongest where it keeps Core SDK, application, prebuilt-binary, community, and experimental scopes separate. The principal corrections are:

1. ROCm Core SDK 7.14.0 is an official, versioned AMD release with a gfx1151 package lane, but AMD's transition guide also calls 7.14.0 a **preview release**. “Official-supported” must not erase that maturity label.
2. AMD's Core SDK lane, Ryzen application matrix, RDNA3.5 prebuilt lane, and this project's installed/live lane are different authorities. They must remain separate rows.
3. No live evidence in this repository validates the proposed ROCmFPX `a5605a7`, llama.cpp `86d86ed`, ROCm 7.14.0, Mesa 26.1.5, or container profiles. The two nodes were measured on ROCm 7.2.4, Mesa 26.1.4, firmware 20260622, and a different deployed ROCmFP4 source revision.
4. License observations support an engineering gate, not a legal conclusion. Distribution remains blocked until the actual release artifact, corresponding source, notices, dependencies, and imported-file provenance are inventoried and reviewed.

## Scope and method

Reviewed against:

- repository and Wiki authority in `AGENTS.md`, `README.md`, `wiki/HaloFPX_Wiki/README.md`, and the relevant category/section manifests;
- Agent Harness routing in `references/agent-harness.md` and the canonical Agent Harness architecture/review guidance;
- both preserved intake trees, their manifests, source registries, data tables, recipes, scripts, containers, and license records;
- primary GitHub commit/compare APIs and literal file headers/licenses;
- current AMD ROCm 7.14.0, transition, 7.2.4, Ryzen, and GPU-architecture documentation;
- retained live evidence under `sources/measurements/2026-07-17-strix-halo-live-inventory/`.

The imported scripts were inspected but not executed. I independently checked the archive manifests: `30/30` lineage entries and `128/128` gfx1151 entries matched SHA-256.

## Package decisions

| Package/material | Decision | Basis and required revision |
|---|---|---|
| Preserved ZIPs, extraction receipts, manifests | **ACCEPT** as immutable intake | Archive hashes and extracted counts are recorded; independent manifest verification passed. This is provenance, not claim approval. |
| Exact Git object identities and parent edges in the lineage package | **ACCEPT** selectively | Primary GitHub APIs confirm the cited ROCmFPX, CachyLLama, and upstream llama.cpp objects and merge parents. Preserve the cutoff timestamp and full hashes. |
| Claim that ROCmFPX `a5605a7` is not graph-descended from llama.cpp snapshot `5fd2dc...` | **ACCEPT** | GitHub compare reports no common ancestor. Describe the relationship as content/snapshot provenance, not Git ancestry. |
| Root and sampled file-level license observations | **ACCEPT** as observations | Literal root licenses and sampled headers support the ledger. They do not establish whole-tree coverage or compatibility of a future combined binary. |
| `license_compatibility_matrix` conclusions | **REVISE** | Replace categorical “compatible/copy” language with scoped engineering treatment: preserve original license and notices, record modification provenance, and require release-specific legal review. |
| Direct import of `llama-ai` code or prose into an MIT-oriented core | **DEFER** | The repository declares GPL-3.0-or-later source and CC-BY-NC-SA documentation, with unresolved boundary and derivative-work questions. Independently restated facts and clean-room requirements are safer than copying. |
| Distribution of a HaloFPX/ROCmFPX binary, container, SDK tarball, or bundled UI | **DEFER** | No candidate-artifact SBOM, corresponding-source map, complete notices, dependency license closure, or counsel decision exists. This gate does not block private source research. |
| ROCm 7.14.0 as an official AMD Core SDK release for gfx1151 | **ACCEPT with revision** | AMD now publishes versioned 7.14.0 documentation and gfx1151 packages. Add AMD's explicit **preview release** label and do not infer application support. |
| ROCm 7.14.0 as the current project deployment baseline | **REJECT** | The nodes are measured on the 7.2.4 legacy lane, and no 7.14.0 project build/runtime evidence exists. It is a candidate experiment lane only. |
| ROCm 7.2 Ryzen/application and RDNA3.5 prebuilt rows | **ACCEPT as separate snapshots** | These are narrower application/prebuilt scopes. Pin versioned URLs and capture dates; do not merge them into the Core SDK row. |
| gfx1151 hardware identity and minimum-kernel facts | **ACCEPT selectively** | AMD identifies Radeon 8060S as gfx1151/40 CUs, and both live nodes enumerate gfx1151. Minimum-kernel statements remain vendor-profile constraints, not proof that every newer kernel is supported. |
| ROCmFPX `a5605a7`, llama.cpp `86d86ed`/b10064, Mesa 26.1.5 build profiles | **DEFER** | Exact pins are useful candidates, but static package validation and source availability are not successful builds or runs on either node. |
| Standard USB4 IP networking | **ACCEPT** in its upstream scope | Keep it distinct from native GPU-visible or verbs transport. |
| `thunderbolt-ibverbs@76ba39...` | **REJECT** for production; **DEFER** for isolated research | Its own project describes it as buggy, insecure, and non-production. Any experiment needs an isolated threat model and an explicit stop/rollback plan. |
| Imported build/install/diagnostic scripts and container recipes | **DEFER** | Review findings below prevent direct execution or promotion. Reimplement or copy only selected pieces into project-owned paths after safety, reproducibility, and license review. |
| Generated `site/`, search index, `llms-full.txt`, and other duplicate renderings | **REJECT** for canonical promotion | Retain only in immutable intake. Promote reviewed Markdown/data/source records, not generated duplicates. |

## Verified lineage facts

The following cutoff facts are suitable for Section 11 after a local citation record is created:

| Repository | Intake pin | Review result |
|---|---:|---|
| `charlie12345/ROCmFPX` | `a5605a72768c6562241b248e268e33dc92787394` | Commit exists. Its parents are `25c71...` and `a8b5...`. Current remote HEAD observed during this review is `61f2f2d7...`, so the intake pin is a snapshot, not “current.” |
| `fewtarius/llama-ai` | `1017f3dfdce3ca2b06aa9007b23295db3bb35722` | Commit exists and still matched remote HEAD during review. |
| `fewtarius/CachyLLama` | `6be745998f568e379ea197fcf827baec73ff9940` | Merge parents `c8ead...` and upstream llama.cpp `92366...` are confirmed; remote HEAD still matched during review. |
| `ggml-org/llama.cpp` | `86d86ed4396b4130922f7b9af26e3d9fc11a591b` | Commit exists; current remote HEAD observed during review is `6bdd77f13...`. |

ROCmFPX merge `2335e6...` has parents `221402...` and `5b3956...`, but a local remote-tracking name such as `upstream/main` is not evidence of the remote URL. CachyLLama's merge is direct graph evidence because the upstream llama.cpp parent object is independently resolvable. Upstream llama.cpp commit `2969d6...` names ROCmFPX/Hy3, which supports reverse-flow provenance but does not retroactively create common Git ancestry.

## Licensing and code-import constraints

### Findings that can be promoted

- ROCmFPX, CachyLLama, and llama.cpp have MIT roots with file-level exceptions.
- Sampled ROCmFPX files include dual/combined SPDX expressions, Apache-2.0 with LLVM exception material, OpenVINO Apache-2.0 material, xxHash BSD-2-Clause material, and files inheriting the repository root license.
- `llama-ai` identifies source as GPL-3.0-or-later and documentation as CC-BY-NC-SA-4.0; its CachyLLama submodule remains a separately pinned MIT work.
- A root license is not an aggregate license report for bundled ROCm/TheRock components, npm assets, model assets, or downloaded binaries.

### High-risk blockers

1. **Release scope is undefined.** Internal experimentation, source publication, binary distribution, container distribution, and commercial distribution trigger different obligations. A single “compatible” flag is insufficient.
2. **Whole-tree coverage is incomplete.** The package is a sampled snapshot, not a complete clone/history/submodule/generated-asset scan. File moves and copied snippets can lose visible headers.
3. **GPL/CC boundary is unresolved.** Do not copy `llama-ai` code, build scripts, or documentation into the MIT-oriented core until the intended use and license boundary are explicitly approved.
4. **Corresponding source and notices are absent for a candidate artifact.** A distributed binary/container must map each shipped component to exact source, build inputs, modifications, license text, notice, and source-delivery method.
5. **ROCm/TheRock redistribution is not closed.** The exact 7.14.0 tarball, its digest, included component inventory, license files, and redistribution terms must be captured before use in a release artifact.
6. **Authorship/provenance must follow copied files.** The intake's CC BY 4.0 documentation and MIT scripts/templates can only be copied with their license/notice/provenance. The package copyright statement is not a substitute for a file-level provenance record.

These are release blockers, not a claim that private clean-room research is prohibited. Legal compatibility and patent questions require qualified review when the release design is concrete.

## ROCm/gfx1151 version-lane audit

Do not collapse these rows:

| Lane | Current evidence | Allowed statement |
|---|---|---|
| AMD ROCm Core SDK 7.14.0 | AMD versioned docs and transition guide list gfx1151 packages and 7.14.0; transition guide calls it a preview release | **[VERIFIED]** official Core SDK candidate with preview maturity; application validation separate |
| AMD legacy ROCm 7.2.4 | AMD versioned compatibility matrix exists; current nodes have 7.2.4 packages | **[VERIFIED]** vendor release; **[MEASURED]** installed project lane, not AMD-certified CachyOS tuple |
| AMD Ryzen application matrix | Versioned ROCm 7.2 Ryzen page includes gfx1151 and PyTorch scope | **[VERIFIED]** only for the exact documented OS/framework/data-type scope |
| AMD RDNA3.5 prebuilt/application table | Package captured stable/unsupported release rows from a mutable page | **[VERIFIED]** only as a dated capture after preserving the exact page; refresh before operational use |
| HaloFPX live tuple | Both nodes: kernel `7.1.3-1-cachyos`, ROCm 7.2.4, Mesa 26.1.4, firmware 20260622; gfx1151 enumerates | **[MEASURED]** environment-specific, with node package-set differences |
| Proposed ROCmFPX/llama/Mesa profiles | Static intake only | **[OPEN]** until exact binaries are built, hashed, and tested on both nodes |

The package's use of `/latest/`, `/develop/`, repository `main`, and version tags as evidence is too mutable for canonical “current” claims. Replace each important citation with a versioned URL or exact commit, plus retrieval time and a preserved content hash. This matters because the repository already contains an earlier review whose AMD release-history capture did not yet list 7.14.0; the live official site now does. Both can be honest time-scoped observations, but they cannot silently overwrite each other.

Primary current records checked during review:

- <https://rocm.docs.amd.com/en/docs-7.14.0/>
- <https://rocm.docs.amd.com/en/latest/about/transition-guide-TheRock.html>
- <https://rocm.docs.amd.com/en/docs-7.2.4/compatibility/compatibility-matrix.html>
- <https://rocm.docs.amd.com/projects/radeon-ryzen/en/docs-7.2/docs/compatibility/compatibilityryz/native_linux/native_linux_compatibility.html>
- <https://rocm.docs.amd.com/en/latest/reference/gpu-arch-specs.html>

## Runtime comparison

The retained inventory supports only the following project-specific claims:

- both targets are Radeon 8060S/gfx1151 with 40 compute units;
- `/dev/kfd`, render nodes, and `rocminfo` enumerate gfx1151;
- both captures report kernel `7.1.3-1-cachyos`, ROCm packages 7.2.4, HIP 7.2.53211, Mesa 26.1.4, and linux-firmware 20260622;
- package subsets differ between nodes, so “ROCm 7.2.4” is not a sufficient environment fingerprint;
- the deployed service is associated with `charlie12345/rocmfp4-llama@4860505e...`, not ROCmFPX `a5605a7` or llama.cpp `86d86ed`;
- nimo-2 owns the model/LAN API and nimo-1 is the RPC worker; deployed binary-to-source hashes remain open.

Therefore the intake's exact candidate pins, flags, containers, and performance expectations must not receive `[MEASURED]` or deployed-baseline status.

## Script, recipe, and container audit

Direct promotion/execution is blocked by the following:

- base images use mutable tags (`fedora:43`, `ubuntu:24.04`, and a version tag without captured OCI digest);
- apt/dnf package resolution is not version-locked and no package snapshot is recorded;
- Git commits are pinned, but repository/signature/tree/blob verification is not part of the recipes;
- build and GGML tests are disabled in container builds, and no runtime test evidence accompanies the images;
- the 7.14.0 tarball path requires a caller-provided checksum, but the package does not record the accepted artifact digest, SBOM, or component notices;
- `seccomp=unconfined`, host IPC, device passthrough, and a read-write model mount broaden the container threat and mutation surface;
- the 7.14 installer can remove an existing prefix and writes under `/opt`; exact-target validation, rollback, and change control are required;
- the diagnostics collector creates files and an archive and may capture sensitive host/runtime data; privacy, redaction, and retention rules are missing;
- `verify-host.sh` checks minimum/profile hints, not exact node parity, loaded DSOs, binary provenance, or a complete supported tuple.

The recipes are useful design inputs. They are not hermetic builds or safe production runbooks as imported.

## Promotion map

| Destination | Material | Decision and conditions |
|---|---|---|
| Section 11 — repository lineage and frozen baselines | Exact commit objects, merge parents, no-common-ancestor result, cutoff timestamp | **ACCEPT** after adding local source records; call them frozen snapshots, not current tips |
| Section 13 — ROCmFPX inventory | Fork-specific `a5605a7` facts and `HSA_OVERRIDE_GFX_VERSION=11.5.1` scope | **REVISE** into exact source claims; no runtime/support inference |
| Section 14 — llama-ai/CachyLLama inventory | Submodule pin, CachyLLama direct ancestry, license roots | **ACCEPT selectively**; **DEFER** copied GPL/CC material |
| Section 16 — build/dependencies/licensing/CI | License ledger observations, release gate, provenance/notice templates | **REVISE**; promote procedures only after project-local review and remove legal conclusions |
| Section 17 — Strix Halo/gfx1151 architecture | Radeon 8060S/gfx1151/40-CU primary fact plus live identification | **ACCEPT** with primary AMD citation and `[MEASURED]` node record |
| Section 23 — kernel/firmware/ROCm/Mesa matrix | Separate 7.14 Core SDK preview, 7.2 Ryzen/app, 7.2.4 installed, and candidate rows | **REVISE** before promotion; preserve versioned sources and maturity labels |
| Sections 24–26 — HIP/HSA, Vulkan/RADV, toolchain | Build flags, RADV recommendations, compiler lanes | **DEFER** until source claims are disentangled from candidate recipes and machine tests |
| Section 37 — gfx1151 kernel optimization | rocWMMA/FlashAttention observations and ROCmFPX candidate flags | **DEFER** pending exact-build correctness and performance experiments |
| Sections 50–55 — fabric/transport | Upstream USB4 IP versus experimental verbs distinction | **ACCEPT** distinction; **REJECT** verbs as production path |
| Sections 70–72 — packaging/deployment/rollback | Container and host recipes | **DEFER** until digests, package locks, least privilege, safety, rollback, and license closure |
| Sections 74, 78, 79, 81, 84 — verification/experiments | Candidate matrix and acceptance ideas | **ACCEPT as experiment inputs**, never as results |
| Section 85 — freshness backlog | Mutable-source drift and upstream-tip watch | **ACCEPT** as a recurring, timestamped research task |

## Required follow-up research

1. Capture complete bare clones/object graphs for all four lineage repositories, all submodules, and the exact cutoff; preserve bundle hashes and remote URLs.
2. Run a full-tree SPDX/header/license/generated-asset scan on the **actual proposed source tree**, not the intake sample; resolve every unknown and preserve notices.
3. Define one concrete release artifact and generate its SBOM, binary dependency closure, source-to-binary map, corresponding-source delivery plan, third-party notices, and human legal review record.
4. Capture the exact ROCm 7.14.0 gfx1151 tarball URL, SHA-256/signature, contents, SBOM/licenses, and AMD preview/maturity statement before any experiment.
5. Build ROCmFPX `a5605a7` and llama.cpp `86d86ed` independently on both nodes from locked inputs; hash binaries and loaded DSOs; run CPU-reference correctness before performance work.
6. Qualify 7.14.0 as a separate candidate experiment from the installed 7.2.4 lane. Do not in-place upgrade either live node as part of research.
7. Record exact kernel config, firmware file hashes/MES version, ROCm package closure, compiler resource directory, Mesa/RADV identity, and node-role parity for each run.
8. Refresh AMD Core SDK, Ryzen application, and RDNA3.5 prebuilt matrices through versioned/captured records; log changes rather than rewriting old observations.
9. Resolve deployed `rocmfp4-llama@4860505e...` binary provenance and compare its feature/patch delta against both ROCmFPX `a5605a7` and upstream llama.cpp `86d86ed`.
10. If USB4 verbs work remains desired, create a separate isolated threat model, kernel-module provenance record, fault/rollback plan, and explicit non-production experiment approval.

## Final gate

Promotion may proceed only as small, independently cited facts and experiment proposals. It must not promote either package wholesale, execute imported scripts, replace the measured 7.2.4 live baseline, or authorize redistribution. A human owner must approve any code import, deployment lane change, or release-license decision.
