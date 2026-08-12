---
section_id: "72"
title: "Upgrade, Migration, and Recovery Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project", "CachyLlama"]
  software_versions: ["CachyLlama 6be745998f568e379ea197fcf827baec73ff9940", "Semantic Versioning 2.0.0"]
  hardware_revisions: ["dual Strix Halo target; exact nodes open"]
related_sections: ["67", "68", "70", "71"]
---

# Facts and constraints

## Source-backed facts

- **[VERIFIED]** Semantic Versioning requires a declared public API, major changes for incompatible API changes, and immutable released contents [S72-01].
- **[VERIFIED]** systemd supports service stop/start/restart, readiness, managed state directories, and credential handling, but does not itself prove application state compatibility [S72-02].
- **[VERIFIED]** OCI manifests identify image configuration/layers by digest, enabling exact image selection [S72-03].
- **[VERIFIED]** NIST SP 800-34 Rev. 1 describes contingency planning, recovery strategies, plan testing, training, and maintenance [S72-04].
- **[VERIFIED]** At CachyLlama commit `6be7459`, the inspected cache has magic/version and a compatibility hash plus optional target/draft/speculative blobs; source inspection did not identify cryptographic blob checksums or atomic temporary-file replacement [S72-05].
- **[VERIFIED]** SLSA 1.2 provides provenance and verification concepts applicable to release artifacts [S72-06].

## Project constraints

- **[RECOMMENDATION]** Version product/API, peer wire protocol, capability set, configuration schema, hardware/model/plan manifests, cache-object format, and support-bundle schema separately.
- **[RECOMMENDATION]** Coupled ranks are one compatibility/failure domain. Mixed versions are refused unless that exact pair and plan were tested; coordinated drain and upgrade is the default.
- **[RECOMMENDATION]** Cache is not a primary backup. If compatibility is unknown or validation fails, quarantine/invalidate and recompute.
- **[RECOMMENDATION]** Never migrate a cache in place. Preserve the source, write a new version, cryptographically verify object/model/plan identity and content, then atomically publish.
- **[OPEN]** No project RPO, RTO, retention period, compatibility window, or supported downgrade span has been ratified.
