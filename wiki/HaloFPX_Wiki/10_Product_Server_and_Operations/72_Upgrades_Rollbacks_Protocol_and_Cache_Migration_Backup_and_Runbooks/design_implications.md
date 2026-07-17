---
section_id: "72"
title: "Upgrade, Migration, and Recovery Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: ["operations design candidate 2026-07-16"]
  hardware_revisions: ["dual Strix Halo target; exact nodes open"]
related_sections: ["67", "68", "69", "70", "71"]
---

# Design implications

## Compatibility declaration

Every release manifest should declare exact build/source/artifact digests; minimum/maximum API and peer protocol versions; supported capabilities; config/plan/model/cache schema readers and writers; required host/kernel/ROCm tuple; and known compatible/blocked peer releases.

**[RECOMMENDATION]** A peer handshake exchanges identity, min/max protocol ranges, capabilities, rank/plan intent, model hashes, and security requirements before allocating work. No overlap or security downgrade fails closed with a stable diagnostic.

## Upgrade modes

- **[RECOMMENDATION]** Replication may use a rolling upgrade only when the old/new combination is in the tested matrix. Drain one worker, upgrade and validate it, shift a canary workload, then progress one failure domain at a time. Prevent incompatible cache sharing.
- **[RECOMMENDATION]** Coupled tensor/pipeline execution uses a coordinated drain: stop admission, finish/cancel by policy, checkpoint only documented durable state, stop all coupled ranks, activate matching releases, handshake, self-test, then resume.
- **[RECOMMENDATION]** Install into `/opt/halofpx/releases/<version>-<digest>/`; validate offline; atomically select the candidate pointer; then explicitly restart/re-exec and verify the running executable/build/model/plan/dependency identity. Obtain startup, peer/rank, and readiness gates before canary traffic; commit normal traffic only after the canary passes. Publish migrated state as a separate validated transition. Retain prior pointer, binary, config snapshot, schema readers, and state preimage until the rollback window closes.
- **[RECOMMENDATION]** Configuration and durable-state migrations are copy-on-write with a recorded preimage and explicit downgrade support. A release that cannot read or safely roll back its predecessor must declare a one-way maintenance event.

## Backup and recovery set

| Data | Treatment |
|---|---|
| Release/build manifests and configuration | versioned backup; secrets separately encrypted |
| Hardware profiles, plans, model manifests/licenses | durable, hash-verified backup |
| Model artifacts | back up if not reproducibly reacquirable; otherwise retain verified locator/digest |
| User/session durable state promised by product | consistency-aware encrypted backup after policy is defined |
| Audit and experiment metadata | append-preserving retention with integrity checks |
| Cache | optional acceleration only; never substitute for durable state backup |

**[RECOMMENDATION]** Backups use a quiesced transaction or consistent snapshot, encryption, independently stored keys, content hashes, inventory, retention, and scheduled restore drills. A successful copy without a restore test is not recovery evidence.

## Runtime cutover and rollback record

| Phase | Required evidence | Abort/rollback boundary |
|---|---|---|
| Install | candidate digest, provenance, immutable path | remove only unreferenced candidate after evidence retention |
| Offline validate | dependency/config/model/plan/state-reader results | leave running release unchanged |
| Pointer select | old and candidate pointer values | restore old pointer; running process is still separately identified |
| Process activate | PID/start time and loaded executable/build identity on every rank | stop candidate; restore pointer and prior process set |
| Readiness prove | `/health/startup`, `/health/ready`, peer/rank and semantic self-check evidence | no normal traffic |
| Traffic commit | canary result and admission/router generation | drain candidate and route back only if prior runtime is ready |
| State publish | new generation hash plus validated preimage/reader compatibility | restore preimage or tested reverse migration |
| Rollback complete | prior pointer, prior runtime identity, compatible state, readiness, canary, traffic commit | escalate if any dimension cannot be made coherent |

**[RECOMMENDATION]** Never call pointer restoration a rollback until process, readiness, traffic, and state evidence all identify the prior compatible generation.

## Minimum runbook set

Each runbook records trigger/symptoms, safety and evidence preservation, authority, exact prerequisites, bounded steps, health/data verification, rollback, escalation, and artifacts. Required cases: failed upgrade, protocol mismatch, cache migration failure, coordinator loss, worker rebuild, link replacement, disk replacement, lost/compromised credential, malicious model, and full-site restore.
