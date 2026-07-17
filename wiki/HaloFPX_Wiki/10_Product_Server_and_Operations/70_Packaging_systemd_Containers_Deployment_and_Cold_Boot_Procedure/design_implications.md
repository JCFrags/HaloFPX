---
section_id: "70"
title: "Deployment and Cold-Boot Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: ["deployment design candidate 2026-07-16"]
  hardware_revisions: ["dual Strix Halo target; exact nodes open"]
related_sections: ["67", "68", "69", "71", "72"]
---

# Design implications

## Proposed filesystem and identity contract

| Purpose | Candidate path | Policy |
|---|---|---|
| Immutable releases | `/opt/halofpx/releases/<version>-<digest>/` | Root-owned, read-only; atomic `current` activation |
| Configuration | `/etc/halofpx/` | Root-owned, service-readable; secrets externalized |
| Durable state | `/var/lib/halofpx/` | Service-owned; backed up according to section 72 |
| Recomputable cache | `/var/cache/halofpx/` | Service-owned; corruption rejects/misses; quota enforced |
| Runtime state | `/run/halofpx/` | Recreated at boot; sockets/PIDs only |
| Models | manifest-selected local store | Hash-verified, read-only during serving |

**[RECOMMENDATION]** Begin with a static `halofpx` system account because persistent state and GPU device permissions need stable, inspectable ownership. Revisit `DynamicUser=` only after `OPS-70-E2` proves managed directories, device ACLs, and recovery behavior.

## Unit topology

- **[RECOMMENDATION]** Use `halofpx-worker@.service` per active rank/host and `halofpx-coordinator.service`; add a narrowly scoped network/link-preparation oneshot only when declarative network management cannot own that state.
- **[RECOMMENDATION]** Use `Type=notify` when the process can truthfully report readiness. Coordinator startup waits for a versioned worker handshake and selected plan; ordering directives alone are insufficient.
- **[RECOMMENDATION]** Candidate hardening includes `NoNewPrivileges=yes`, `ProtectSystem=strict`, `ProtectHome=yes`, `PrivateTmp=yes`, `RestrictSUIDSGID=yes`, empty capability bounds, explicit writable paths, and cgroup device allow rules. Do not use `PrivateDevices=yes` where it hides required GPUs.
- **[RECOMMENDATION]** Exact `/dev/kfd` and render-node access is derived from machine inventory and limited to the service. Broad `privileged`, host PID, and host network container modes are not defaults.

## Cold-boot state machine

1. Firmware, kernel, IOMMU/device driver, clocks, storage, and physical links become available.
2. Stable interface/device naming and host firewall/network configuration converge.
3. Required model/state filesystems mount; configuration and manifests validate.
4. Workers start, identify exact build/device/rank, and reject unsafe cache objects.
5. Coordinator negotiates versions/capabilities, selects replication or coupled plan, and verifies rank ownership.
6. Optional preload runs within resource/time limits; a minimal semantic self-check completes.
7. Readiness becomes true. If coupled mode cannot form, only an explicitly allowed single-node/replication fallback may become ready, visibly degraded.

**[INFERENCE]** Splitting workers from the coordinator gives failures narrower restart domains, but only machine tests can establish correct ROCm cleanup and device-reset behavior.

## Runtime activation phases

**[RECOMMENDATION]** Treat release installation and live activation as distinct, recorded transitions:

1. **Install:** place the digest-identified candidate in a new immutable directory without changing the running service.
2. **Offline validation:** verify signatures/digests, dependency tuple, configuration/manifests, state-reader compatibility, permissions, and self-tests.
3. **Pointer:** atomically select the candidate activation pointer while retaining the old pointer value; this does not change an existing process.
4. **Process:** start/restart or explicitly re-exec workers and coordinator so their executable identity matches the pointer.
5. **Readiness:** prove reported build, model, plan, dependency, peer/rank, and state-schema identities, then pass `/health/startup` and `/health/ready` gates.
6. **Traffic:** canary the candidate and only then commit normal admission/traffic.
7. **State:** publish migrated durable state only after independent validation; cache remains quarantinable/recomputable.
8. **Rollback:** drain/stop candidate traffic, restore a compatible state preimage when required, restore the prior pointer, restart prior processes, re-prove identity/readiness, canary, and recommit traffic.

Each phase emits an audit record and has a bounded abort path. **[OPEN]** Canary thresholds, drain deadlines, and rollback authority remain unratified.
