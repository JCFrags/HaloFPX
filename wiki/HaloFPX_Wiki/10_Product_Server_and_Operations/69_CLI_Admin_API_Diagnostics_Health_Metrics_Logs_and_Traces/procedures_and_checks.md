---
section_id: "69"
title: "Operator Interface and Observability Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: ["design candidate 2026-07-16"]
  hardware_revisions: ["dual Strix Halo target; exact nodes open"]
related_sections: ["68", "70", "71", "72"]
---

# Procedures and checks

## OPS-69-E1 — observability conformance

1. Record exact binaries, configuration, model/plan hashes, node identities, and clock synchronization.
2. Exercise startup, ready, request, cancellation, unload, fallback, and shutdown.
3. Assert each transition appears consistently in CLI/API, metrics, logs, and traces.
4. Verify all timestamps/units/schema versions and retain raw output under `experiments/OPS-69-E1/`.

Pass condition: signals agree on state and identifiers, expected spans/events exist, and no unknown/unbounded labels appear.

## OPS-69-E2 — cardinality and privacy

Generate many request/session/model display names containing secrets and unique values. Scrape metrics, logs, traces, and a support bundle. Pass only if metric series remain bounded and secret canaries are absent from default artifacts. Record any opt-in disclosure separately.

## OPS-69-E3 — diagnostic fault matrix

Before fault injection, use a disposable deployment and cache/store on an explicitly resolved sacrificial path or loopback filesystem. Preserve the only model/source/evidence copies, declare whether root or device privileges are required, set free-space/resource ceilings and stop conditions, retain out-of-band recovery access, and record cleanup. Never fill the boot/workspace/model filesystem or target production state. Physical link, kernel, device, power-loss, or host-level faults require the Section 80 authorization and safety procedure.

Inject one fault at a time: missing copied model fixture, corrupt disposable cache object, rank loss, link loss, bounded ENOSPC on the sacrificial filesystem, incompatible peer, authorization failure, and timeout. Verify `/health/live`, `/health/ready`, `/health/startup`, degraded behavior, actionable stable error codes, safe recovery advice, and no evidence destruction. Stop immediately on unexpected target resolution, host instability, loss of recovery access, or mutation outside the disposable root.

## Review gate

- **[RECOMMENDATION]** Do not mark this section validated until tests cover both hosts, replication and coupled modes, authorized and unauthorized clients, and restarts.
- **[RECOMMENDATION]** Treat a diagnostic that mutates state without preview/audit, or a bundle that leaks a canary, as release-blocking.
