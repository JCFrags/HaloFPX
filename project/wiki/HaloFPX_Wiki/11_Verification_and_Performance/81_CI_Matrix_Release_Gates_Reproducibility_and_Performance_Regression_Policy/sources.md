---
section_id: "81"
title: "CI, Release Gate, Reproducibility, and Regression Sources"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940", "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"]
  software_versions: ["SLSA 1.2", "Reproducible Builds definition accessed 2026-07-17"]
  hardware_revisions: []
related_sections: ["73", "78", "79", "80"]
---

# Sources

Accessed 2026-07-17. Repository workflows were inspected at exact commits; no HaloFPX pipeline was executed.

| ID | Source and revision | Claims supported | Limitations/conflicts |
|---|---|---|---|
| S81-01 | [llama.cpp workflows at `788e07d`](https://github.com/ggml-org/llama.cpp/tree/788e07dc91d266ad3162a1ce9037665656269689/.github/workflows) | Backend/platform builds, sanitizers, server CI, release and integrity checks | Upstream coverage is not the future HaloFPX matrix |
| S81-02 | [ROCmFPX aggregate checks at `a5605a7`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/check-rocmfpx-all.sh) | Fork aggregate check and optional-test behavior | Optional/model-dependent skips can coexist with aggregate success |
| S81-03 | [CachyLLama at `6be7459`](https://github.com/fewtarius/CachyLLama/tree/6be745998f568e379ea197fcf827baec73ff9940) | Exact donor identity for release-input tracking | Identity only; no HaloFPX gate evidence |
| S81-04 | [llama-ai at `1017f3d`](https://github.com/fewtarius/llama-ai/tree/1017f3dfdce3ca2b06aa9007b23295db3bb35722) | Exact donor identity for release-input tracking | Identity only; no HaloFPX gate evidence |
| S81-05 | [Reproducible Builds definition, accessed 2026-07-17](https://reproducible-builds.org/docs/definition/) | Bit-for-bit reproducibility definition and required inputs | Does not establish runtime correctness |
| S81-06 | [SLSA specification 1.2](https://slsa.dev/spec/v1.2/) | Provenance model and build-lineage evidence | Provenance complements, not replaces, tests and deployment verification |
| S81-07 | [GitHub securing builds, accessed 2026-07-17](https://docs.github.com/en/code-security/tutorials/implement-supply-chain-best-practices/securing-builds) | Artifact and SBOM attestations | GitHub-specific mechanism; project signing authority remains open |
| S81-08 | [GitHub supply-chain security, accessed 2026-07-17](https://docs.github.com/code-security/supply-chain-security/understanding-your-software-supply-chain/about-supply-chain-security) | Dependency review, CodeQL, and supply-chain controls | Moving service documentation; not a release policy by itself |
| S81-09 | [GitHub immutable releases, accessed 2026-07-17](https://docs.github.com/en/code-security/concepts/supply-chain-security/immutable-releases) | Release-tag/asset immutability and attestations | Hosting control; does not prove artifact reproducibility or deployment |
