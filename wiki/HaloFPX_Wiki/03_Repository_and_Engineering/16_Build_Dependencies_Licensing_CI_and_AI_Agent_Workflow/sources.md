---
section_id: "16"
title: "Primary sources for engineering controls"
status: "draft"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"
    - "llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"
  software_versions: ["SPDX 3.0.1", "SLSA 1.2"]
  hardware_revisions: []
related_sections: ["03.11", "03.14", "03.15", "11"]
---

# Sources

Access date for all sources: **2026-07-16** (America/Los_Angeles). Repository claims are pinned to commits even when a branch was used for discovery.

## Repository sources

### S16-01 — ROCmFPX pinned source tree

- Publisher: `charlie12345/ROCmFPX`
- Revision: `a5605a72768c6562241b248e268e33dc92787394`
- URL: https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394
- Supports: source identity, project status, build entry points, repository layout.
- Limitation: repository claims and historical benchmarks are not HaloFPX measurements.

### S16-02 — ROCmFPX CMake and build documentation

- Files: [`CMakeLists.txt`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/CMakeLists.txt), [`ggml/CMakeLists.txt`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/CMakeLists.txt), [`docs/build.md`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/docs/build.md), [`Makefile`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/Makefile), [`pyproject.toml`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/pyproject.toml)
- Supports: CMake requirement, C/C++ standards, backend options, generated build info, dependency bounds, `SOURCE_DATE_EPOCH` behavior.
- Limitation: optional backend requirements are broader than HaloFPX's intended support set.

### S16-03 — ROCmFPX target build scripts

- Files: [`scripts/build-strix-rocmfp4-mtp.sh`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/build-strix-rocmfp4-mtp.sh), [`scripts/build-rocmfp4-rocm714-local.sh`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/build-rocmfp4-rocm714-local.sh)
- Supports: `gfx1151`, HIP+Vulkan flags, selected targets, local/nightly ROCm pattern, packaging behavior.
- Limitation: one script contains a maintainer-local rocWMMA path; the nightly download example lacks a recorded archive checksum in the script.

### S16-04 — ROCmFPX CI definitions

- Files: [custom ROCmFPX reference workflow](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/.github/workflows/check-rocmfpx.yml), [workflow directory](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394/.github/workflows), [`ci/README.md`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ci/README.md)
- Supports: custom CPU reference gate and inherited workflow inventory.
- Limitation: static workflow files do not prove run status or branch protection; the custom gate does not run ROCm/Vulkan hardware.

### S16-05 — ROCmFPX contribution, AI, license, and notices controls

- Files: [`AGENTS.md`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/AGENTS.md), [`CONTRIBUTING.md`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/CONTRIBUTING.md), [`LICENSE`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/LICENSE), [`THIRD_PARTY_NOTICES.md`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/THIRD_PARTY_NOTICES.md), [`CODEOWNERS`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/CODEOWNERS)
- Supports: AI restrictions, human responsibility, review/test/commit conventions, MIT license, bundled notices.
- Limitation: notices are not a full SBOM or legal opinion.

### S16-06 — llama.cpp pinned build/dependency source

- Publisher/revision: `ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689`
- Files: [`CMakeLists.txt`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/CMakeLists.txt), [`ggml/CMakeLists.txt`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/ggml/CMakeLists.txt), [`docs/build.md`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/docs/build.md), [`pyproject.toml`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/pyproject.toml)
- Supports: current upstream drift comparison, build/dependency mechanisms.
- Limitation: newer upstream state is not automatically compatible with the ROCmFPX fork.

### S16-07 — llama.cpp contributor and license controls

- Files: [`AGENTS.md`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/AGENTS.md), [`CONTRIBUTING.md`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/CONTRIBUTING.md), [`LICENSE`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/LICENSE)
- Supports: upstream AI policy, review/format/commit conventions, MIT license.
- Limitation: HaloFPX private-fork rules may add controls but cannot relax rules for an upstream submission.

### S16-08 — CachyLLama pinned tree and CI

- Publisher/revision: `fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940`
- URLs: [tree](https://github.com/fewtarius/CachyLLama/tree/6be745998f568e379ea197fcf827baec73ff9940), [workflows](https://github.com/fewtarius/CachyLLama/tree/6be745998f568e379ea197fcf827baec73ff9940/.github/workflows), [license](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/LICENSE)
- Supports: fork identity, MIT root license, inherited CI/build controls.
- Limitation: workflow presence is not evidence for cache-specific correctness; no dedicated append-only AI log was found.

### S16-09 — llama-ai pin, submodule, build scripts, and license

- Publisher/revision: `fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722`
- Files: [`.gitmodules`](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/.gitmodules), [commit tree gitlink](https://github.com/fewtarius/llama-ai/tree/1017f3dfdce3ca2b06aa9007b23295db3bb35722/CachyLLama), [`scripts/rebuild.sh`](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/scripts/rebuild.sh), [`scripts/install-deps.sh`](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/scripts/install-deps.sh), [`README.md`](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/README.md), [`LICENSE`](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/LICENSE)
- Supports: submodule pin, dependency installer, ROCm download/build behavior, GPL-3.0-or-later source boundary, CC-BY-NC-SA-4.0 documentation declaration.
- Limitation: distro/hardware assumptions and downloaded nightlies require current verification.

### S16-10 — llama-ai agent instructions

- File: [`AGENTS.md`](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/AGENTS.md)
- Supports: Bash style, ignored build/data directories, submodule handling, deprecated patch workflow, scoped hardware assumptions.
- Limitation: repository guidance is not validated HaloFPX policy.

## Governance and standards

### S16-11 — Agent Harness architecture

- Publisher: local canonical Agent Harness
- Revision/access date: local document inspected 2026-07-16
- Paths: `C:\Users\britt\Documents\Agent_Harness\AGENTS.md`; `C:\Users\britt\Documents\Agent_Harness\guide\architecture.md`; routed by `../../../../references/agent-harness.md`
- Supports: evidence promotion, provenance, reversibility, memory scoping, closeout review.
- Limitation: conceptual harness guidance; HaloFPX repository rules take precedence.

### S16-12 — SPDX Specification 3.0.1

- Publisher: SPDX / Linux Foundation
- Revision: 3.0.1, copyright 2010-2024
- URL: https://spdx.github.io/spdx-spec/
- Supports: machine-readable BOM, software build/provenance/license/integrity metadata.
- Limitation: selecting fields/tooling and validating a generated SBOM remain implementation tasks.

### S16-13 — SLSA provenance

- Publisher: SLSA / Linux Foundation
- Revision: specification 1.2, approved page accessed 2026-07-16
- URL: https://slsa.dev/spec/v1.2/provenance
- Supports: provenance as verifiable where/when/how information linking artifacts to source/build process.
- Limitation: this section recommends a compatible shape; it does not claim HaloFPX meets a SLSA level.

### S16-14 — GNU GPL FAQ

- Publisher: Free Software Foundation
- Revision: live FAQ accessed 2026-07-16
- URL: https://www.gnu.org/licenses/gpl-faq.en.html
- Supports: distinction between separate co-installed programs and combined works; need for compatible terms when combining.
- Limitation: general publisher guidance, not legal advice for the project's facts or jurisdiction.

### S16-15 — SOURCE_DATE_EPOCH reference

- Publisher: Reproducible Builds project
- Revision: live specification guidance accessed 2026-07-16
- URL: https://reproducible-builds.org/docs/source-date-epoch/
- Supports: standardized timestamp input and Git-derived setting pattern.
- Limitation: setting the variable alone does not make all outputs reproducible.
