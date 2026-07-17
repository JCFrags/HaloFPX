---
section_id: "01"
title: "Wiki Architecture Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: ["Git 2.x", "YAML 1.2.2", "JSON Schema 2020-12"]
  hardware_revisions: []
related_sections: ["02", "03", "04", "05"]
---

# Procedures and checks

## Section intake

Prerequisites: cleanly identify the incoming section, its prompt manifest, and the current canonical section. Root access: not required.

1. Confirm `section_id`, category, title, and target path against `research/prompts/section_index.yaml`.
2. Validate every required file and Markdown front matter.
3. Reject duplicate IDs, duplicate `canonical_path`, or duplicate `authoritative_for` values.
4. Check relative links and verify every cited source record exists.
5. Compare facts by applicability; record contradictions rather than overwriting them.
6. If replacement is approved, archive the prior applicable version under `_archive/<id>/<date>/` and record the relationship.
7. Regenerate navigation deterministically and review the diff.

## Read-only repository checks

```powershell
# Run from repository root; no elevation required.
rg --files wiki/HaloFPX_Wiki
rg -n '^section_id:|^status:|^last_verified:' wiki/HaloFPX_Wiki
git diff --check
```

**[RECOMMENDATION]** The eventual validator should emit JSON as well as human text and return nonzero for schema errors, unresolved IDs, missing files, absolute internal links, case mismatch, or duplicate authority.

## Required machine/repository experiment

Run the validator from Windows and from both Strix Halo Linux nodes against the same commit. Record OS, filesystem case behavior, Git version, validator version, command, exit status, and output under `experiments/`. The pass condition is identical registry resolution and zero broken links on all three environments.

## Contingent decisions

- Validator language and dependency policy depend on what is already available on all hosts.
- Whether to commit generated navigation depends on deterministic byte-for-byte reproduction.
- CODEOWNERS adoption depends on repository hosting and collaborator identities; do not invent owners.
