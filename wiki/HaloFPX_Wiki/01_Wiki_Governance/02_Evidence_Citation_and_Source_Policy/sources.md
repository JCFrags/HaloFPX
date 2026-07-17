---
section_id: "02"
title: "Evidence Policy Sources"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project", "Agent_Harness"]
  software_versions: ["CFF 1.2.0", "W3C PROV-O 2013"]
  hardware_revisions: []
related_sections: ["01", "05"]
---

# Sources

| ID | Source and revision | Location | Supports | Limitations |
|---|---|---|---|---|
| S02-01 | GitHub Docs, permanent links, accessed 2026-07-16 | https://docs.github.com/en/repositories/working-with-files/using-files/getting-permanent-links-to-files | Commit-addressed exact file links | GitHub-specific |
| S02-02 | W3C PROV-O Recommendation, 2013-04-30 | https://www.w3.org/TR/2013/REC-prov-o-20130430/ | Provenance entities and relations | HaloFPX uses a simplified mapping |
| S02-03 | Git `gitrevisions` documentation, accessed 2026-07-16 | https://git-scm.com/docs/gitrevisions | Revision/object naming | Abbreviations can become ambiguous |
| S02-04 | Citation File Format schema guide 1.2.0, repository HEAD `0c5b4aa07071490eaf261775ce96ccdd13a6e2d5` observed 2026-07-16 | https://github.com/citation-file-format/citation-file-format/blob/0c5b4aa07071490eaf261775ce96ccdd13a6e2d5/schema-guide.md | Citation metadata fields | CFF is not a claim database |
| S02-05 | RFC 8174, BCP 14 update, 2017-05 | https://www.rfc-editor.org/rfc/rfc8174 | Normative keyword convention | Applies only when explicitly invoked |
| S02-06 | Agent Harness `AGENTS.md` and architecture, accessed 2026-07-16 | `C:/Users/britt/Documents/Agent_Harness/AGENTS.md`; `C:/Users/britt/Documents/Agent_Harness/guide/architecture.md` | Promotion, memory, review discipline | Local project authority |
| S02-07 | ROCmFPX README at `a5605a72768c6562241b248e268e33dc92787394` | https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/README.md | Observed repository claims/revision | Benchmarks are repository-local, not HaloFPX results |
| S02-08 | CachyLlama README at `6be745998f568e379ea197fcf827baec73ff9940` | https://github.com/fewtarius/CachyLlama/blob/6be745998f568e379ea197fcf827baec73ff9940/README.md | Observed repository purpose/revision | Function not locally validated |
| S02-09 | llama-ai README at `1017f3dfdce3ca2b06aa9007b23295db3bb35722` | https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/README.md | Observed repository revision | Function not locally validated |
| S02-10 | llama.cpp README at `788e07dc91d266ad3162a1ce9037665656269689` | https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/README.md | Observed upstream revision | Fast-moving repository |

Access date: 2026-07-16. Remote HEAD observations were made with `git ls-remote`; they must be refreshed before pinning.
