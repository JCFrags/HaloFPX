---
section_id: "08"
title: "Boundary and Dependency Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project and pinned dependencies"]
  software_versions: []
  hardware_revisions: ["actual two-node deployment"]
related_sections: ["11", "15", "16", "18", "23", "49", "60", "71"]
---

# Boundary and dependency checks

## Source-code checks

1. Record repository URL, full commit, commit date, submodule SHAs, license, patch-base ancestor, and local patch hash.
2. For every imported feature, identify source files, tests, persistent formats, public APIs, and upstream conflicts.
3. Generate an endpoint/option compatibility diff against the pinned upstream server.
4. Scan dependency licenses and notices; preserve source and required attribution.
5. Re-run the source audit before each upstream rebase or release candidate.

## On-machine checks

1. Inventory the exact nodes and confirm whether CPU, memory, GPU, NVMe, firmware, and USB4 paths are actually matched.
2. Boot the pinned Linux/software matrix and prove `/health`, `/v1/models`, a bounded inference request, metrics, and clean shutdown on each node.
3. Test Vulkan and HIP/ROCm per supported model; record unsupported combinations explicitly.
4. Verify one-node operation with the peer absent, then one-link and node-loss behaviors.
5. Create cache entries, restart, upgrade schema, change model/template/shard plan, corrupt bytes, and verify miss/recompute or explicit rejection.
6. From an unauthorized client, verify inference/admin/metrics/cache access matches the threat model.

## Scope-change gate

A scope addition needs: named user/workload, source and license, compatibility impact, new tests, operational owner, security impact, performance hypothesis, rollback, and a decision record. README claims alone are insufficient.

