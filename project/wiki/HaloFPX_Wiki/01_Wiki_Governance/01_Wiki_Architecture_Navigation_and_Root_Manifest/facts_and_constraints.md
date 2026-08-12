---
section_id: "01"
title: "Wiki Architecture Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: ["YAML 1.2.2", "JSON Schema 2020-12"]
  hardware_revisions: []
related_sections: ["02", "03", "04", "05"]
---

# Facts and constraints

## Source-backed facts

- **[VERIFIED]** YAML 1.2.2 is a complete, human-oriented data serialization specification and its revision date is 2021-10-01 [S01-01].
- **[VERIFIED]** JSON Schema Draft 2020-12 provides machine-validation vocabulary and a published metaschema [S01-02].
- **[VERIFIED]** A GitHub branch URL is mutable; replacing the branch with a commit ID creates an exact-version permalink [S01-03].
- **[VERIFIED]** Repository-relative links remain usable across clones and branches; absolute web links can fail outside the hosting site [S01-04].
- **[VERIFIED]** CODEOWNERS can identify responsible people or teams and can participate in required-review enforcement when repository settings support it [S01-05].
- **[VERIFIED]** Agent Harness defines the Wiki layer as cited orientation, distinct from Sources, Knowledge, Skills, Memory, and Reviews [S01-06].

## Project constraints

- **[VERIFIED]** The supplied section index assigns globally unique section IDs `01` through `86`; category ID and section ID are separate fields [S01-07].
- **[RECOMMENDATION]** Never reuse a retired section ID. Keep its registry entry with `status: superseded` and a `superseded_by` link.
- **[RECOMMENDATION]** A section path may change only through a manifest migration that leaves a redirect stub or archive record. Human-readable titles may evolve without changing the ID.
- **[RECOMMENDATION]** Canonical Markdown filenames are ASCII `snake_case.md`; canonical directories retain the prompt package's numbered names. Paths are case-sensitive even when authored on Windows.
- **[RECOMMENDATION]** One topic has one authoritative section. Other sections link to it and may state scoped deltas, but must not silently fork the same normative explanation.

## Proposed directory contract

```text
wiki/HaloFPX_Wiki/
  README.md
  manifest.yaml
  manifest.schema.json
  <NN_Category>/
    README.md
    <NN_Section>/
      README.md
      facts_and_constraints.md
      design_implications.md
      procedures_and_checks.md
      open_questions.md
      sources.md
      section.yaml
  _archive/<section-id>/<YYYY-MM-DD>/
```

**[VERIFIED]** The root `manifest.yaml` and `manifest.schema.json` now implement the canonical path and structural-artifact registry. They are generated from the supplied section index and each present `section.yaml`; they do not approve claims, decisions, or software baselines. The required cross-platform validator experiment remains open.
