---
section_id: "72"
title: "Upgrade, Migration, and Recovery Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: ["operations design candidate 2026-07-16"]
  hardware_revisions: ["dual Strix Halo target; exact nodes open"]
related_sections: ["68", "69", "70", "71"]
---

# Procedures and checks

## OPS-72-E1 — compatibility matrix

For every supported adjacent release pair, test old coordinator/new worker, new coordinator/old worker, both new, both old, no-overlap protocol, missing capability, changed model/plan, and changed security requirement in replication and coupled modes. Record handshake transcript (redacted), result, signals, and exact digests.

## OPS-72-E2 — rolling and coordinated rehearsal

Run sustained canary traffic in a disposable deployment. Rehearse replication drain/upgrade/canary/progress and coupled stop-all/upgrade/self-test/resume through the distinct install, offline-validation, pointer, process, readiness, traffic, state-publication, and rollback phases. Inject a bounded failure between every adjacent phase. Record old/new pointer, PID and loaded executable/build identity, model/plan/dependency identity, health gates, traffic generation, and persistent-state generation. Measure rejected/lost/duplicated requests, readiness, rollback time, and state consistency; a pointer-only reversal fails the rehearsal.

## OPS-72-E3 — cache migration mutation

On an explicitly resolved disposable cache tree or loopback filesystem, create source-version cache fixtures, including truncation, bit flips, wrong model/plan, swapped blobs, old permissions, bounded low disk, interrupted write, and duplicate destination. Declare privileges and resource ceilings, preserve originals, and never use the production cache, model store, boot disk, workspace, or sole evidence copy. Migrate offline to a new tree, validate cryptographic identity/content, sample restore, publish atomically, and retain/quarantine source. Invalid objects must miss/recompute or fail safely.

## OPS-72-E4 — backup/restore disaster recovery

From documented backups, rebuild onto clean storage with no original services available. Restore keys through the approved separate path, validate hashes/permissions/manifests, start single-node safe mode, add the second node, and verify application semantics. Report achieved RPO/RTO; do not infer them from copy speed.

## OPS-72-E5 — hardware replacement runbooks

Rehearse link replacement, disk replacement, worker rebuild, and coordinator rebuild only with approved sacrificial targets, preserved recovery access, exact device/interface resolution, stop conditions, privilege declaration, and cleanup. Physical cable, kernel, block-device, or power faults require the Section 80 authorization procedure. Verify identity re-enrollment, stale-peer rejection, cache/state classification, rank ownership, single-node fallback, reintegration, and support-bundle evidence.

## Rollback gate

- **[RECOMMENDATION]** Roll back on schema/protocol incompatibility, security regression, corrupt state, semantic error, or SLO breach according to ratified thresholds. Preserve failed-release evidence before cleanup.
- **[RECOMMENDATION]** Never point an old binary at state already irreversibly rewritten by a new release; restore the recorded preimage or execute a tested reverse migration.
