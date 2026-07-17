---
section_id: "03"
title: "Glossary and Naming Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: []
  hardware_revisions: []
related_sections: ["01", "04", "05"]
---

# Procedures and checks

## Add or change a term

Prerequisites: evidence for upstream terms or a stated project need for proposed terms. Root access: not required.

1. Search the wiki, code, manifests, logs, and tests for existing spelling and aliases.
2. Determine whether the term is upstream-defined, standard-defined, project-defined, or an assumption.
3. Add one canonical definition, prohibited/legacy aliases, scope, and source.
4. Link affected schemas and create a decision record for breaking enum/identifier changes.
5. Never recycle an identifier; deprecate it with a replacement.

## Static checks

```powershell
# Run at repository root; no elevation required.
rg -n -i '\b(TP|GPU split|cache|memory|bandwidth)\b' wiki research experiments
rg -n '[^\x00-\x7F]' wiki/HaloFPX_Wiki
git diff --check
```

The ambiguity scan is triage, not an automatic failure: contextual definitions may be valid. A future validator should enforce ID regexes, unique IDs, known enum values, canonical unit symbols, and RFC 3339 timestamps.

## Two-node inventory validation

On each node, record without modification:

- chassis/board identifiers and firmware revision;
- OS/kernel, ROCm, driver, and runtime build identities;
- host alias and stable inventory ID;
- accelerator enumeration and backend-visible device names;
- both USB4 interface names, MACs, addresses, negotiated state, and topology;
- cache storage device/filesystem identity.

Store raw output under an experiment run and map observations to stable project IDs. Root may be required for firmware/topology commands; mark each command accordingly before execution.

## Decision gate

Do not finalize rank roles, device enum names, or compatibility-ID fields until distributed-runtime and cache experiments show actual ownership and invalidation boundaries.
