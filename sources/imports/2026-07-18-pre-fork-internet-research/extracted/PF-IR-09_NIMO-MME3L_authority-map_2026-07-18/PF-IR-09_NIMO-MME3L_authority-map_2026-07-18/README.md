# PF-IR-09 — NIMO MME3L exact hardware firmware, security, errata, and RAS authority map

Open `index.html` for the LLM-Wiki view. Machine-readable decision data is in `06_decision_tables/`; provenance and checksums are in `08_manifests/`; the read-only local collection script is in `07_local_evidence/`.

## Decision state

The measured BIOS cannot be treated as covering the latest public Strix Halo PI floors. The OEM support page captured here does not publish an exact MME3L/board-v1.0 BIOS package, release notes, PI contents, payload signature model, or rollback procedure. The BIOS update path remains `[HOLD] [OPEN]`.

AMD publishes exact Strix Halo mitigations through `StrixHaloPI-FP11_1.0.0.2b`, while `CVE-2025-48516` is explicitly listed for Ryzen AI Max 300 with no fix planned and hardware changes targeted for future platforms. That item remains a residual-risk watch entry, not a firmware closure.

Crucial's official P310 support index states no firmware update is available at capture time, but does not enumerate `VACR001`, signing, downgrade, or slot behavior. USB4 exact applicability remains open because the measured router/controller identifiers were not supplied in the request payload.

## Use boundary

This pack supports watch-list and pre-deployment decisions. It does not authorize firmware rollout, service replacement, destructive testing, or threshold changes. Those require local inventory, rollback proof, recovery media, maintenance-window controls, and human approval.
