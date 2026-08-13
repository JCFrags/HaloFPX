---
section_id: "83"
title: "Risk Review, Validation, and Contingency Procedures"
status: "needs-machine-validation"
last_verified: "2026-08-12"
applies_to:
  repositories: ["HaloFPX integration repository"]
  software_versions: ["candidate governance workflow"]
  hardware_revisions: ["dual Strix Halo; exact BOM open"]
related_sections: ["05", "20", "21", "22", "23", "73", "78", "79", "80", "81", "84", "85", "86"]
---

# Risk review, validation, and contingency procedures

## Register update transaction

**[RECOMMENDATION]** Perform this transaction weekly, at every release gate, and whenever a trigger fires:

1. Preserve the new source, log, experiment receipt, issue, or decision before editing a score.
2. Identify affected risk IDs and the exact hardware/software/model/plan scope.
3. Recheck whether the evidence is `[VERIFIED]`, `[MEASURED]`, `[INFERENCE]`, `[ASSUMPTION]`, `[RECOMMENDATION]`, or `[OPEN]`.
4. Record old/new `L`, `I`, `D`, gross score, control state, reason, evidence ID, author, reviewer, and date.
5. Confirm a trigger, mitigation, fallback, accountable owner, acceptance authority, and next review date exist.
6. If critical/high and unowned, block the affected gate. If a fallback is untested, schedule its experiment rather than calling it available.
7. Review linked roadmap, experiment, upstream-watch, issue, ADR, and release artifacts for consistency.
8. Append history; do not silently overwrite the earlier rationale.

Minimum machine-readable event fields:

```yaml
risk_id: R83-NNN
observed_at: YYYY-MM-DDTHH:MM:SSZ
scope_fingerprint: exact-plan-or-baseline-id
old_score: {likelihood: 0, impact: 0, detectability: 0}
new_score: {likelihood: 0, impact: 0, detectability: 0}
control_state: planned
trigger: concise-condition
evidence: [stable-evidence-id]
owner: role
reviewer: independent-role
next_review: YYYY-MM-DD
decision: raise | lower | accept | mitigate | fallback | close | reopen
```

## Release risk gate

**[RECOMMENDATION]** Before a release candidate:

- Resolve all artifacts to full hashes and verify the baseline offline.
- Run the matched correctness suite before performance tests; a faster wrong result fails.
- Run supported single-node and distributed smoke, stress, and fault cells on both nodes.
- Exercise cache-off, one-rail, one-node, rollback, and corrupt-cache fallbacks.
- Confirm no unexpected listeners; verify peer/client authentication, permissions, secret delivery, and firewall state.
- Review current upstream commits, releases, advisories, ROCm limitations, kernel changes, and model revisions.
- Reconcile risk IDs with roadmap gates, open questions, experiments, issues, and ADRs.
- Reject release when any critical/high item lacks current evidence, an owner, or an exercised contingency.

## On-machine validation cards

Commands below are inventory examples, not a complete experiment harness. Run on both nodes, preserve raw stdout/stderr and environment metadata, and attach the Section 84 experiment ID. Read-only inventory requires no root unless access policy blocks a path; kernel logs, fault injection, power changes, and firewall inspection may require root. Fault tests use disposable cache data and an approved maintenance window.

### Experiment alias crosswalk

`M83-*` is an immutable Section 83 request alias, not an experiment definition or run ID. External scheduling, evidence bundles, and claims must use the allocated Section 84 `HLX-EXP-*` definition(s) below and a separate `HLX-RUN-*` per execution. A row is satisfied only when the canonical card captures every listed Section 83 observation; mapping several cards does not authorize combining unmatched results.

| Local request alias | Canonical Section 84 definition(s) | Mapping rationale |
|---|---|---|
| `M83-01` | `HLX-EXP-20260717-841`, `HLX-EXP-20260717-844` | identity/topology inventory plus isolated and simultaneous dual-link qualification |
| `M83-02` | `HLX-EXP-20260717-841`, `HLX-EXP-20260717-843`, `HLX-EXP-20260717-849` | exact installed tuple, matched smoke/correctness, and rollback evidence |
| `M83-03` | `HLX-EXP-20260717-843` | matched HIP/Vulkan/model correctness and performance baseline |
| `M83-04` | `HLX-EXP-20260717-843`, `HLX-EXP-20260717-848` | per-host memory baseline plus service admission/pressure envelope |
| `M83-05` | `HLX-EXP-20260717-841`, `HLX-EXP-20260717-846`, `HLX-EXP-20260717-849` | device identity, HaloKV durability/endurance, and authorized corruption/crash recovery |
| `M83-06` | `HLX-EXP-20260717-848` | combined supported service stress and soak envelope |
| `M83-07` | `HLX-EXP-20260717-849` | authorized listener, identity, permission, replay, and isolation negatives |
| `M83-08` | `HLX-EXP-20260717-849` | cable/rank/coordinator/output-commit fault and fallback matrix |
| `M83-09` | `HLX-EXP-20260717-849`, `HLX-EXP-20260717-850` | rollback plus offline restoration and independent reproducibility |
| `M83-10` | `HLX-EXP-20260717-850` | independent operator reproduction and evidence handoff |

| ID | Risks | Procedure and required evidence | Pass/decision output |
|---|---|---|---|
| M83-01 | 001, 002 | Capture `readlink -f /sys/bus/thunderbolt/devices/domain*/device`, `lspci -Dtvnn`, domain/router/retimer attributes, `ethtool`, IRQ affinity, cable/port labels; run randomized A-only/B-only/A+B paired trials | Controller/root/resource map and confidence interval for additive capacity |
| M83-02 | 003, 010 | Capture `uname -a`, `/proc/config.gz`, packages, `modinfo`, loaded firmware hashes, `rocminfo`, `hipconfig`, `vulkaninfo`; verify required KFD commit ancestry/backports; boot known-good rollback | Supported tuple manifest, smoke/correctness result, rollback receipt |
| M83-03 | 004, 006 | Run matched HIP/Vulkan graphs for every required op/shape/model/quant with golden logits/tokens and quality fixtures before throughput | Backend support/performance matrix; no unsupported cell hidden by averaging |
| M83-04 | 007 | In an authorized isolated window, first prove no protected/foreign KFD/render/HMM owner and record exact identities plus kernel-OOM baseline; then sweep model/context/batch/ubatch/concurrency/cache/transport buffers and collect RSS, cgroup, HSA/GTT/VRAM/HMM `gpu_active`, swap, PSI, allocation errors | Safe envelope with recovery reserve, explicit rejection, and proof that ordinary `MemAvailable` never overrides GPU/HMM ownership |
| M83-05 | 008, 015 | Inventory `nvme list -v`, identify/SMART logs, `lspci -Dvv`, `lsblk`, `findmnt`; measure application bytes versus host writes and available device telemetry; run disk-full/corruption/crash tests on disposable cache | Write budget, reserve threshold, miss/recompute proof, endurance projection |
| M83-06 | 009 | Calibrate sensors; soak worst-case compute+fabric+NVMe at each supported ambient/power profile; preserve clocks, throttling, errors, temperatures, fan/power state | Sustainable envelope and fallback cap; no extrapolation from a short run |
| M83-07 | 013 | Enumerate listeners/firewall/routes/units/credentials/permissions; test unauthenticated, wrong-identity, replay, path, model, and cache-isolation cases from a controlled peer | Negative tests fail safely; secrets absent from argv/logs; least privilege proven |
| M83-08 | 016, 018 | Pull each cable, kill each rank/coordinator, delay/drop/reorder messages, restart stale incarnations, and interrupt client streams while tracing request-to-output IDs | Bounded failure, fencing, no duplicate committed output, usable fallback |
| M83-09 | 005, 014, 017 | Restore source/dependency/model bundles offline, rebuild twice, compare hashes or documented nondeterminism, rehearse one upstream sync and rollback | Provenance-complete reproducible baseline and measured sync effort |
| M83-10 | 011, 012 | Give a second operator only the frozen bundle and runbook; require restore, deploy, diagnose, fallback, and evidence capture without author assistance | Bus-factor drill and schedule estimate based on observed work |

### Safety prerequisites

- Apply [issue #41](https://github.com/JCFrags/HaloFPX/issues/41) before any target command. Reject builds, quantization, disposable inference, and benchmarks while protected production or unaccounted KFD/render/HMM ownership exists; ordinary free/available/RSS/swap values do not clear the gate.
- Preserve source, model, cache, configuration, and current health evidence before mutation.
- Preserve exact PID, InvocationID, restart count, GPU-owner census, and kernel-OOM baseline. If either rank changes identity, require both-rank reconciliation and a real minimal inference; health alone is insufficient.
- Use a disposable cache namespace for corruption, disk-full, and power-loss testing.
- Never pull storage power from a mounted production filesystem.
- Confirm console/out-of-band recovery and a known-good boot entry before kernel or firmware tests.
- Stop on unexplained data corruption, repeated GPU reset, unsafe temperature, filesystem error, or inability to recover the control baseline.

## Internet and source-code follow-up

| ID | Cadence/trigger | Query | Evidence destination |
|---|---|---|---|
| I83-01 | Weekly and pre-release | Resolve all four remote heads/releases; diff against frozen pins; inspect security/correctness changes | Section 85 snapshot plus R83-005/014 |
| I83-02 | Every ROCm lane change | AMD compatibility, Ryzen limitations, release notes, gfx1151 issues, required kernel/firmware tuple | Section 23 plus R83-003 |
| I83-03 | Every kernel lane change | USB4/thunderbolt, KFD/amdgpu, USB4STREAM ABI/patch lineage, distribution backports and CVEs | Sections 23/50/55 plus R83-010 |
| I83-04 | Per target model | Pin model/config/tokenizer/license/weights; map converter and backend support at exact commit | Sections 29/31/78 plus R83-006/017 |
| I83-05 | Monthly and pre-release | llama.cpp advisories/policy, RPC changes, dependency advisories, SLSA/SSDF implications | Sections 16/71/81 plus R83-013/017 |
| I83-06 | When SSD/BOM known | Exact vendor datasheet, firmware, warranty, TBW, thermal and power-loss claims | Sections 18/21/65 plus R83-008/015 |

## Trigger response times

**[RECOMMENDATION]** Security exposure, wrong-result, accepted corrupt state, filesystem corruption, or unrecoverable rollback is immediate stop-work for the affected surface. Other critical triggers receive triage before further integration; high triggers before the next gate; medium/low triggers at weekly review. Severity determines speed, not permission to skip evidence preservation.

## Closeout quality check

Before marking an item mitigated or accepted, independently review correctness, freshness, provenance, scope, rollback, and whether the task revealed a reusable control improvement. Apply a small supported correction in scope; otherwise route a proposal through the project review process [S83-22].
