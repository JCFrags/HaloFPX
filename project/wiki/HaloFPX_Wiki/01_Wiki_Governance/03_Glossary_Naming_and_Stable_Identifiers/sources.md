---
section_id: "03"
title: "Glossary and Naming Sources"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ROCmFPX", "CachyLlama", "llama-ai", "llama.cpp"]
  software_versions: []
  hardware_revisions: []
related_sections: ["02"]
---

# Sources

| ID | Source and revision | Location | Supports | Limitations |
|---|---|---|---|---|
| S03-01 | ROCmFPX README, commit `a5605a72768c6562241b248e268e33dc92787394` | https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/README.md | ROCmFPX name, weight formats, project claims | Experimental; measurements are repository-specific |
| S03-02 | CachyLlama README, commit `6be745998f568e379ea197fcf827baec73ff9940` | https://github.com/fewtarius/CachyLlama/blob/6be745998f568e379ea197fcf827baec73ff9940/README.md | Project name and persistent-cache purpose | Claims need local validation |
| S03-03 | llama-ai README, commit `1017f3dfdce3ca2b06aa9007b23295db3bb35722` | https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/README.md | Repository identity | Sparse naming authority |
| S03-04 | llama.cpp README, commit `788e07dc91d266ad3162a1ce9037665656269689` | https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/README.md | Upstream identity, GGUF/backend terminology | Fast moving |
| S03-05 | GGUF specification, ggml repository commit `af97976c7810cdabb1863172f31c432dab767de7` | https://github.com/ggml-org/ggml/blob/af97976c7810cdabb1863172f31c432dab767de7/docs/gguf.md | GGUF purpose and native naming | Observed HEAD is not an approved project pin |
| S03-06 | llama.cpp server README, commit `788e07dc91d266ad3162a1ce9037665656269689` | https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/server/README.md | Split-mode enums and descriptions | CLI behavior still requires code/test inspection |
| S03-07 | NIST Guide to SI, chapter 4, updated 2025-08-18 | https://www.nist.gov/pml/special-publication-811/nist-guide-si-chapter-4-two-classes-si-units-and-si-prefixes | Decimal SI and binary-prefix distinction | Not a benchmark schema |
| S03-08 | RFC 8141, Uniform Resource Names, 2017-04 | https://www.rfc-editor.org/rfc/rfc8141 | Persistent location-independent identity intent | No HaloFPX URN namespace is registered |
| S03-09 | RFC 3339, Internet timestamps, 2002-07 | https://www.rfc-editor.org/rfc/rfc3339 | Timestamp format and UTC guidance | Filename basic form is a project convention |

Access date for web sources: 2026-07-16. No external source defines the HaloFPX-specific identifier namespace; those entries are recommendations.
