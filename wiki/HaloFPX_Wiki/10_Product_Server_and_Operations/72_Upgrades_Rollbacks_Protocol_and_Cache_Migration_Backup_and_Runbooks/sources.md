---
section_id: "72"
title: "Upgrade, Migration, and Recovery Sources"
status: "verified"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project", "CachyLlama", "systemd"]
  software_versions: ["source snapshot 2026-07-16"]
  hardware_revisions: []
related_sections: ["70", "71"]
---

# Sources

| ID | Source | Revision/access | Scope and use | Limitations |
|---|---|---|---|---|
| S72-01 | [Semantic Versioning 2.0.0](https://semver.org/spec/v2.0.0.html) | Version 2.0.0; accessed 2026-07-16 | Declared public API compatibility and immutable releases | Does not version HaloFPX protocol, state, or model surfaces automatically |
| S72-02 | [systemd manuals](https://github.com/systemd/systemd/tree/8009fa49845cd6fb7b7014ab06218b68fe702006/man) | Commit `8009fa49845cd6fb7b7014ab06218b68fe702006`; accessed 2026-07-16 | Service lifecycle, readiness, directories, credentials | Pointer changes and unit mechanisms do not prove live process/state cutover |
| S72-03 | [OCI image specification](https://github.com/opencontainers/image-spec/tree/af26a05fba5ee648512f4ea3c9fda1fcc1b6d6dc) | Commit `af26a05fba5ee648512f4ea3c9fda1fcc1b6d6dc`; accessed 2026-07-16 | Digest-addressed release artifacts | Artifact identity does not establish runtime or data compatibility |
| S72-04 | [NIST SP 800-34 Rev. 1](https://csrc.nist.gov/pubs/sp/800/34/r1/upd1/final) | Revision 1 with Update 1; accessed 2026-07-16 | Contingency planning, testing, training, maintenance | General guidance; project RPO/RTO and authority remain open |
| S72-05 | [CachyLLama `kv-ssd-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.cpp) and [`kv-ssd-cache.h`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.h) | Commit `6be745998f568e379ea197fcf827baec73ff9940`; source paths rechecked 2026-07-17 | Bounded version/compatibility, integrity, and write-path source audit | Two files at one commit; machine mutation and migration tests remain required |
| S72-06 | [SLSA specification 1.2](https://slsa.dev/spec/v1.2/) | Version 1.2; accessed 2026-07-16 | Release provenance and verification | Does not choose HaloFPX build/signing policy |
| S72-07 | [Agent Harness architecture](../../../../references/agent-harness.md) | Local canonical pointer; accessed 2026-07-16 | Evidence promotion, knowledge and workflow boundary | Conceptual authority; not release or recovery evidence |

**[VERIFIED]** These sources justify mechanisms and constraints, not project recovery performance. RPO, RTO, rollback, and migration claims require retained experiment evidence.
