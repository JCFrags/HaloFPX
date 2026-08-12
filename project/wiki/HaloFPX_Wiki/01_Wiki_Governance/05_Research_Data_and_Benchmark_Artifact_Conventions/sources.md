---
section_id: "05"
title: "Research Data and Benchmark Sources"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project", "ROCmFPX"]
  software_versions: ["DataCite Metadata Schema 4.7", "MLPerf Inference Rules 5.1"]
  hardware_revisions: []
related_sections: ["02", "03"]
---

# Sources

| ID | Source and revision | Location | Supports | Limitations |
|---|---|---|---|---|
| S05-01 | MLPerf Inference Rules 5.1, repository HEAD `c547732b539cb3a14cc5680597714c8c1df4cad0` observed 2026-07-16 | https://github.com/mlcommons/inference_policies/blob/c547732b539cb3a14cc5680597714c8c1df4cad0/inference_rules.adoc | Replicability, consistency, nondeterminism controls | HaloFPX is not claiming MLPerf compliance |
| S05-02 | DataCite Metadata Schema 4.7, released 2026-03-03 | https://schema.datacite.org/ | Resource identification/citation metadata | Broader publication schema, not run metadata |
| S05-03 | W3C PROV-O Recommendation, 2013-04-30 | https://www.w3.org/TR/2013/REC-prov-o-20130430/ | Provenance lineage | Simplified local representation |
| S05-04 | Git LFS specification, repository HEAD `d72db1e533a1d6ee5543e02e9f8ccac97e0fcd34` observed 2026-07-16 | https://github.com/git-lfs/git-lfs/blob/d72db1e533a1d6ee5543e02e9f8ccac97e0fcd34/docs/spec.md | Pointer/content separation and integrity identity | Requires LFS service/operations if adopted |
| S05-05 | NIST Guide to SI, chapter 4, updated 2025-08-18 | https://www.nist.gov/pml/special-publication-811/nist-guide-si-chapter-4-two-classes-si-units-and-si-prefixes | Decimal SI usage | Local schemas still choose unit enums |
| S05-06 | RFC 3339, 2002-07 | https://www.rfc-editor.org/rfc/rfc3339 | Unambiguous timestamps/UTC | Does not cover synchronized measurement clocks |
| S05-07 | ROCmFPX README, commit `a5605a72768c6562241b248e268e33dc92787394` | https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/README.md | Example scoping of local benchmark claims | Results are not transferable to HaloFPX |
| S05-08 | Citation File Format 1.2.0 schema guide, repository commit `0c5b4aa07071490eaf261775ce96ccdd13a6e2d5` | https://github.com/citation-file-format/citation-file-format/blob/0c5b4aa07071490eaf261775ce96ccdd13a6e2d5/schema-guide.md | Software/data citation metadata | Not an experiment bundle schema |
| S05-09 | Agent Harness source and wiki templates, accessed 2026-07-16 | `C:/Users/britt/Documents/Agent_Harness/templates/source.md`; `C:/Users/britt/Documents/Agent_Harness/templates/wiki-page.md` | Provenance, limitations, verification/review fields | Local project authority |

Access date for web sources: 2026-07-16. No cited standard dictates HaloFPX's exact run folder; it is a project recommendation requiring validation.
