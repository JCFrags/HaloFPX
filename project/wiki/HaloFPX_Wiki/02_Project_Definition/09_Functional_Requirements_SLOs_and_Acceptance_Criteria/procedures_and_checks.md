---
section_id: "09"
title: "Acceptance Procedures and Evidence"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project"]
  software_versions: ["release candidate under test"]
  hardware_revisions: ["exact node A/node B inventory"]
related_sections: ["68", "69", "70", "71", "72", "74", "75", "77", "78", "80"]
---

# Acceptance procedures

## Required manifest

Before testing, record: source/submodule commits and patch hash; build flags/compiler; binary hashes; node/firmware/OS/driver inventory; model/tokenizer/template hashes and licenses; backend; mode/shard plan; context/batch/concurrency; cache/wire versions; power/thermal settings; client/test version; start/end timestamps.

## Acceptance sequence

1. Validate config and compatibility matrix; reject unknown or unsupported combinations.
2. Run unit/format/protocol tests and single-node golden inference on both nodes.
3. Establish matched `B` and `C` baselines with randomized test order and declared warmup.
4. Run each mandatory workload cell for single-node, replication, and each proposed split mode.
5. Exercise streaming, non-streaming, tool calls, cancellation, timeout, overload, and malformed requests.
6. Execute cache restart, mutation, truncation, wrong-model/template/schema/rank, full-disk, and concurrent-eviction cases.
7. Inject one-link loss, both-link loss, worker crash/hang, coordinator restart, slow rank, and storage error.
8. Run authentication/authorization, path, redaction, tenant-isolation, and egress checks.
9. Perform clean-host install, restart, upgrade, rollback, backup/restore, and uninstall rehearsal.
10. Run the soak window and calculate SLOs from client-visible raw events.

## Pass/fail rules

- Each FR has a machine-readable test/result link or an approved `not-applicable` rationale.
- Aggregate by predefined workload cell; publish every run, median, tail, failures, and environment—not best-of results.
- Any accepted invalid cache, cross-user data exposure, unauthorized admin access, or incorrect partial distributed result fails the release.
- Performance passes only against the matched baseline and ratified target.
- Flaky tests remain failures until the cause and policy are recorded.

## Internet follow-up

At release freeze, recheck dependency heads, advisories, license changes, server route documentation, and known cache/distributed correctness issues. Record differences; do not silently advance commits.

