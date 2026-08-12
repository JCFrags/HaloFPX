---
section_id: "16"
title: "Open engineering-control questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX planned fork"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo / gfx1151 (planned)"]
related_sections: ["03.11", "03.14", "03.15", "04", "11"]
---

# Open questions

| ID | Question | Resolution evidence | Owner/dependency |
|---|---|---|---|
| H16-O01 | What exact ROCm/LLVM build is supported on both target hosts? | clean HIP builds, operator tests, package/SDK digests on both hosts | Section 04 + machine experiment |
| H16-O02 | What Vulkan loader, driver, shader compiler, and SDK revisions are qualified? | host manifests and clean Vulkan tests | Section 04 + machine experiment |
| H16-O03 | Ship one HIP+Vulkan binary or separate backend artifacts? | size/startup/reliability comparisons and packaging review | architecture decision |
| H16-O04 | Which dependency-lock authority is used: container digest, Nix, distro snapshot, or generated lock/manifest? | two clean offline rebuilds and maintenance analysis | build ADR |
| H16-O05 | Is bit-for-bit reproduction achievable across two paths/hosts? | isolated double-build hashes plus diffoscope-style analysis | release engineering |
| H16-O06 | Which GPU tests block every PR, and which run nightly because of cost? | runner capacity and flake-rate evidence | CI ADR |
| H16-O07 | How are self-hosted Strix runners isolated from untrusted PR code and secrets? | threat model and approved runner policy | security review |
| H16-O08 | What matched quality/performance thresholds block a merge? | baseline experiments with model/workload hashes and variance | Section 11 |
| H16-O09 | Which CachyLLama commits are imported and what tests prove semantic parity? | Section 14 inventory, patch records, cache tests | Sections 14/15 |
| H16-O10 | Will any GPL-3.0-or-later `llama-ai` source be copied, linked, or distributed with HaloFPX? | distribution design plus competent license review | project owner/legal |
| H16-O11 | What license covers original HaloFPX orchestration, docs, and generated artifacts? | approved licensing ADR and notices | project owner/legal |
| H16-O12 | Which SBOM/provenance tools and schemas are release authority? | validated SPDX 3.0.1 and SLSA-shaped artifacts | release engineering |
| H16-O13 | How is the AI log made tamper-evident beyond ordinary Git history? | protected-branch policy, digest chain, signed provenance test | governance/security |
| H16-O14 | What counts as material AI assistance requiring a log entry? | approved examples and review trial | governance ADR |
| H16-O15 | Are upstream GitHub Actions pinned to immutable SHAs in the HaloFPX fork? | action inventory, risk review, automated enforcement | supply-chain review |
| H16-O16 | Which model/tokenizer/test-data licenses permit CI and release redistribution? | per-artifact license manifest | model/data owners |

## Concrete source follow-up

1. **[OPEN]** Diff ROCmFPX against its exact llama.cpp merge base to enumerate build/CI drift; shallow source inspection cannot establish the merge base.
2. **[OPEN]** Inventory every GitHub Action reference and optional CMake download, replacing tags/branches with reviewed immutable digests where feasible.
3. **[OPEN]** Review current official AMD Strix Halo/ROCm support, release notes, and redistribution terms immediately before choosing the toolchain.
4. **[OPEN]** Produce a complete per-file/per-package license scan, including embedded single-header libraries, ROCm runtime bundling, Web UI, models, and test corpora.
5. **[OPEN]** Verify whether upstream AI contribution policy or CI conventions changed after the pinned commits before any upstream PR.

## Machine-validation queue

1. capture identical host manifests and note every intentional difference;
2. build the pinned baseline twice per host, once HIP+Vulkan and once each backend separately;
3. run CPU/HIP/Vulkan backend-ops and quantization reference checks;
4. test SSD-cache integrity, schema/version mismatch, and recomputation behavior;
5. test two-rank failure and single-node fallback behavior;
6. run matched quality/performance baselines with raw evidence;
7. build offline from the dependency mirror and compare artifacts;
8. generate and validate SBOM, provenance, notices, and AI log linkage.

## Contingent decisions

No supported toolchain, release format, CI threshold, runner topology, reproduction threshold, or combined license conclusion should be finalized until its corresponding evidence above exists.

