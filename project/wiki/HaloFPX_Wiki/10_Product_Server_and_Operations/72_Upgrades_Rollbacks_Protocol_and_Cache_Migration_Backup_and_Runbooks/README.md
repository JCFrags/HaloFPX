---
section_id: "72"
title: "Upgrades, Rollbacks, Protocol and Cache Migration, Backup, and Runbooks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project", "CachyLlama"]
  software_versions: ["CachyLlama 6be745998f568e379ea197fcf827baec73ff9940", "Semantic Versioning 2.0.0"]
  hardware_revisions: ["dual Strix Halo target; exact nodes open"]
related_sections: ["67", "68", "69", "70", "71"]
---

# Upgrades, Rollbacks, Protocol and Cache Migration, Backup, and Runbooks

**[RECOMMENDATION]** Install immutable, digest-identified releases, then separately record pointer selection, process restart/re-exec, identity/readiness proof, canary and traffic commit, durable-state publication, and rollback. Negotiate protocol versions/capabilities before work; use rolling upgrades only for explicitly tested replication combinations; coordinate/drain coupled ranks; and treat cache as disposable derived state unless a copy-on-write migration passes validation.

**[VERIFIED]** Semantic Versioning describes compatibility for a declared public API, but it does not automatically version HaloFPX wire protocols, manifests, model requirements, or cache objects [S72-01]. Those surfaces need separate explicit versions.

## Research split

- Online/source research completed: public API versioning, systemd activation mechanisms, OCI digest identity, contingency planning, supply-chain provenance, and current CachyLlama cache format inspection.
- Machine validation required: mixed-version matrix, drain/resume, atomic activation, rollback, cache migration failure, backup restore, node/disk/link replacement, and recovery time/data loss.
- Contingent decisions: compatibility window, release/backup retention, RPO/RTO, rolling eligibility, cache migration support, backup target/encryption, and authority to roll back.

See [facts and constraints](facts_and_constraints.md), [design implications](design_implications.md), [procedures and checks](procedures_and_checks.md), [open questions](open_questions.md), and [sources](sources.md).
