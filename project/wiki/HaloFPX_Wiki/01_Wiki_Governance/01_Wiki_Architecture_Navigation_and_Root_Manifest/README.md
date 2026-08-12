---
section_id: "01"
title: "Wiki Architecture, Navigation, and Root Manifest"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: ["YAML 1.2.2", "JSON Schema 2020-12"]
  hardware_revisions: []
related_sections: ["02", "03", "04", "05"]
---

# Wiki Architecture, Navigation, and Root Manifest

**[RECOMMENDATION]** Treat `wiki/HaloFPX_Wiki/` as a retrieval index over evidence-backed sections, not as the evidence store itself. The discovery path is root manifest -> category -> section `README.md` -> focused page -> cited source or experiment. This implements the Agent Harness routing contract `sources -> wiki -> knowledge -> skills` without promoting this wiki into a procedure library.

## Authority contract

1. `manifest.yaml` is the machine-readable registry of current canonical sections.
2. The root and category `README.md` files are human navigation views derived from that registry.
3. Each section's `section.yaml` is authoritative for section-level status and applicability; Markdown front matter must agree with it.
4. Source records and experiment artifacts support claims; Git history alone does not make a claim true.
5. Decisions in section [04](../04_Assumption_Open_Question_and_Decision_Ledgers/README.md) may promote recommendations into project requirements.

**[VERIFIED]** GitHub recommends relative links for repository portability, and a URL containing a commit ID identifies an exact repository state [S01-03, S01-04].

## Research split

- Internet/source-code research completed: YAML and JSON Schema versioning, relative-link behavior, commit-addressed permalinks, CODEOWNERS responsibility mapping, and Agent Harness promotion rules.
- Machine/repository inspection required: validate all manifests, case-sensitive paths, links, duplicate IDs, and generated navigation on Windows and the two Linux nodes.
- Contingent decision: select the validator implementation and decide whether generated root views are checked in.

See [facts and constraints](facts_and_constraints.md), [design implications](design_implications.md), [procedures and checks](procedures_and_checks.md), [open questions](open_questions.md), and [sources](sources.md).
