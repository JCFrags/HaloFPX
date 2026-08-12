---
section_id: "64"
title: "Lifecycle and isolation tests"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: ["dual Strix Halo"]
related_sections: ["79", "80"]
---

# Procedures and checks

## Isolation and safety gate

Use synthetic non-sensitive principals and a disposable store/service instance with exact resolved paths, store UUID, byte/inode ceilings, isolated cgroup, preserved recovery access, stop conditions, cleanup, and evidence receipt. Never test against production identities, cache/model/workspace/boot paths, backups containing real user data, or sole evidence copies. Ordinary namespace and fixture tests require no root. ENOSPC/EDQUOT, filesystem permission, kernel/device, or sanitization tests must declare minimum privilege and use an approved loopback/sacrificial target under Section 80.

## M64-01 reachability/GC model

Generate shared prefixes, branches, active leases, failed commits, exports, and pinned roots. Run mark/sweep repeatedly and concurrently with restores. Acceptance: no reachable object is deleted; all eligible objects are eventually reclaimed; retries are idempotent.

## M64-02 quota/disk-pressure matrix

Drive synthetic users/models with unequal object sizes and reuse. Inject ENOSPC/EDQUOT only inside the bounded disposable filesystem/quota harness. Record fairness, reclaimed bytes, write amplification, foreground latency, refused stores, and successful target-only inference.

## M64-03 isolation/privacy

Attempt cross-user prefix matches, guessed namespace paths, administrative exports, log correlation, symlink/path traversal, and deletion during active use. Verify filesystem permissions and backups. Acceptance: no cross-principal data/state disclosure and deletion semantics match the documented level.

## M64-04 retention/endurance soak

Run GC/compaction under sustained load; record deleted/rewritten bytes, queue throttling, p95/p99 latency, and SSD health. Select sweep cadence only after this test.
