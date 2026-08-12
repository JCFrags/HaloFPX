---
section_id: "01"
title: "Wiki Architecture Sources"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project", "Agent_Harness"]
  software_versions: ["YAML 1.2.2", "JSON Schema 2020-12"]
  hardware_revisions: []
related_sections: ["02"]
---

# Sources

| ID | Source and revision | Location | Claims supported | Limitations |
|---|---|---|---|---|
| S01-01 | YAML 1.2.2 specification, YAML Language Development Team, 2021-10-01 | https://yaml.org/spec/1.2.2/ | YAML revision and data model | Does not define HaloFPX fields |
| S01-02 | JSON Schema Draft 2020-12, published 2022-06-16 | https://json-schema.org/draft/2020-12 | Machine-validatable schemas | A schema must still be authored and tested |
| S01-03 | GitHub Docs, "Getting permanent links to files," accessed 2026-07-16 | https://docs.github.com/en/repositories/working-with-files/using-files/getting-permanent-links-to-files | Commit-addressed permalinks | Hosting-specific UI guidance |
| S01-04 | GitHub Docs, "About READMEs," accessed 2026-07-16 | https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/about-readmes | Relative repository links | Rendering details may change |
| S01-05 | GitHub Docs, "About code owners," accessed 2026-07-16 | https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/about-code-owners | Ownership and review integration | Requires host configuration and real identities |
| S01-06 | Agent Harness architecture, local canonical document, accessed 2026-07-16 | `C:/Users/britt/Documents/Agent_Harness/guide/architecture.md` | Layer boundaries and promotion | Conceptual local authority, not an external standard |
| S01-07 | HaloFPX prompt section index, package 1.0, generated 2026-07-17 | `research/prompts/section_index.yaml` | Global IDs and canonical target paths | Prompt input, not evidence of implementation |

Access date for all web sources: 2026-07-16. No source conflicts were found; policy choices remain recommendations.
