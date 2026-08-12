---
section_id: "10"
title: "Architecture Decision Procedure"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project"]
  software_versions: []
  hardware_revisions: ["actual measured deployment"]
related_sections: ["09", "15", "38", "47", "48", "78", "80", "86"]
---

# Architecture decision procedure

## Decision record template

1. Stable decision ID, owner, date, status, and review trigger.
2. Workload/model/platform profile and user outcome.
3. Constraints and hard gates.
4. Candidates, including “do nothing” and best single-node/replication baseline.
5. Hypothesis and predicted bottleneck; no predicted value presented as measured.
6. Exact experiment manifest, raw data paths, statistical method, and quality tests.
7. Results with uncertainty, failures, and applicability limits.
8. Tradeoff scores, weight sensitivity, decision, rejected alternatives, and dissent.
9. Implementation boundary, feature flag, rollback, observability, and expiry/review date.

## Experiment sequence

1. Reproduce both-node single-node baselines and quantify node asymmetry.
2. Profile prompt fill, decode, memory, CPU, GPU, NVMe, and each transport path separately.
3. Change one factor at a time; randomize order where thermal/history effects matter.
4. Add the smallest candidate optimization and run correctness before performance.
5. Test cold/warm cache, low/typical/high context, and declared concurrency.
6. Inject relevant faults and verify fallback/rollback.
7. Repeat enough runs to report distributions and confidence; preserve every raw run.
8. Promote only for exact applicability keys; leave other profiles on the simpler mode.

## Review triggers

- Upstream commit/backend/driver/kernel/firmware change.
- Model, quantization, tokenizer/template, context, or workload distribution change.
- Hardware/cable/port/NVMe change.
- New correctness, security, reliability, or performance evidence.
- Profile expiry or repeated planner fallback.

## Closeout review

Check correctness, freshness, clarity, provenance, routing, reuse value, rollback, and whether the decision revealed a smaller upstreamable change. Unsafe or unsupported improvements become proposals, not silent production defaults.

