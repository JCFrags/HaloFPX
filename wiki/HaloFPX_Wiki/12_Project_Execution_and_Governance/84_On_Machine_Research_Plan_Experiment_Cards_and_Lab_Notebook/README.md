---
section_id: "84"
title: "On-Machine Research Plan, Experiment Cards, and Lab Notebook"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories:
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
  software_versions: ["HaloFPX experiment-plan proposal v0.1"]
  hardware_revisions: ["two matched AMD Strix Halo systems; exact BOM and link map unresolved"]
related_sections: ["03", "04", "05", "18", "20", "22", "24", "25", "29", "38", "55", "65", "73", "74", "75", "76", "77", "78", "79", "80", "81", "82", "83", "85", "86"]
---

# 84 - On-Machine Research Plan, Experiment Cards, and Lab Notebook

## Decision-useful summary

**[RECOMMENDATION]** Execute the ten gated experiment families in [procedures and checks](procedures_and_checks.md) in dependency order. Do not let a later benchmark compensate for a failed identity, timing, correctness, or safety gate.

**[INFERENCE]** The section-local machine lists across the wiki describe overlapping observations at different abstraction levels. Treating every list item as an independent campaign would repeat setup, fragment provenance, and invite unmatched comparisons. Section 84 therefore deduplicates them into one physical evidence sequence while retaining the originating section IDs in every card.

**[OPEN]** No command in this section was executed on either target host. No throughput, latency, power, temperature, clock-offset, cache, correctness, or failure result is **[MEASURED]**.

## Executable card artifacts

The card contract is implemented, not merely described:

- [`experiment-card.schema.json`](experiment-card.schema.json) is the authoritative Draft 2020-12 JSON Schema. It requires every contract field and rejects undeclared fields.
- [`experiment-card.template.yaml`](experiment-card.template.yaml) is the copyable authoring template. Its `null` values are deliberately unresolved and must be replaced before approval.
- [`cards/`](cards/) contains the ten explicit draft instances, `HLX-EXP-20260717-841` through `-850`. Each instance has every contract field; no blank field is silently omitted.
- [`experiment-aliases.yaml`](experiment-aliases.yaml) maps every Section 82 `M82-*`, Section 83 `M83-*`, Section 85 `EX85-*`, and Section 86 `EXP-86-*` request to one or more canonical cards.
- [`validate_experiment_cards.py`](validate_experiment_cards.py) deterministically validates schema conformance, IDs, filenames, the complete ten-card set, and alias coverage. `--format json` produces machine-readable output.

Draft cards are intentionally not runnable: unresolved commands have `argv: null`, exact targets and authorization are `null`, and the command authorization gate forbids execution until approval. Validation means contract completeness, not permission, machine capability, or a measurement result.

## Gated sequence

| Gate | Experiment family | Unlocks | Must not decide yet |
|---|---|---|---|
| `HLX-EXP-20260717-841` | identity, BOM, topology, clocks, collectors | comparable two-host records | performance or topology |
| `HLX-EXP-20260717-842` | measurement-system qualification | trusted timestamps, sensors, raw bundles | cross-host one-way latency if uncertainty is too large |
| `HLX-EXP-20260717-843` | matched single-node HIP/Vulkan/model baseline | per-host controls and viable backends | distributed winner |
| `HLX-EXP-20260717-844` | dual-link host and GPU-path fabric characterization | admissible carriers and link policy | kernel patch or striping policy before holdout |
| `HLX-EXP-20260717-845` | buffer, coherence, copy, graph, and synchronization proof | safe data-path candidates | zero-copy claim from allocation alone |
| `HLX-EXP-20260717-846` | HaloKV state, restore, durability, and endurance | cache format/tier candidates | cache promotion after silent mismatch/corruption |
| `HLX-EXP-20260717-847` | cache-off distributed-mode matrix; optional cache-integrated branch | architecture break-even regions without cache confounding, then integrated-cache regions if E06 passes | universal mode default or any cache claim from the cache-off path |
| `HLX-EXP-20260717-848` | API, lifecycle, concurrency, observability, stress | operational envelope | production readiness |
| `HLX-EXP-20260717-849` | authorized fault, security, recovery, rollback | bounded degraded modes | fault tolerance after any silent wrong result |
| `HLX-EXP-20260717-850` | randomized holdout and reproducibility gate | ADR/release evidence | promotion when raw reproduction fails |

## Evidence route

```text
approved card -> immutable run bundles -> validated derivation -> section claim
              -> open question closure -> ADR evidence link -> implementation gate
```

Raw bundles belong under [`experiments/`](../../../../experiments/), not in this wiki folder. Stable identifiers follow [Section 03](../../01_Wiki_Governance/03_Glossary_Naming_and_Stable_Identifiers/README.md); evidence handling follows [Section 05](../../01_Wiki_Governance/05_Research_Data_and_Benchmark_Artifact_Conventions/README.md); decisions remain owned by [Section 04](../../01_Wiki_Governance/04_Assumption_Open_Question_and_Decision_Ledgers/README.md).

## Retrieval map

- [Facts and constraints](facts_and_constraints.md) - precedence, timing, reproducibility, and evidence rules.
- [Design implications](design_implications.md) - deduplication map, gates, notebook layout, and decision boundaries.
- [Procedures and checks](procedures_and_checks.md) - standardized card contract and ten executable card families.
- [Experiment cards](cards/) - ten schema-valid draft card instances.
- [Alias mapping](experiment-aliases.yaml) - section-local request IDs to canonical cards.
- [Open questions](open_questions.md) - twelve machine, Internet, and cross-project gaps.
- [Sources](sources.md) - pinned primary and internal authorities.

## Deterministic validation

From the repository root:

```powershell
python wiki/HaloFPX_Wiki/12_Project_Execution_and_Governance/84_On_Machine_Research_Plan_Experiment_Cards_and_Lab_Notebook/validate_experiment_cards.py
python wiki/HaloFPX_Wiki/12_Project_Execution_and_Governance/84_On_Machine_Research_Plan_Experiment_Cards_and_Lab_Notebook/validate_experiment_cards.py --format json
```

## Research split

| Track | State |
|---|---|
| Internet/source research | completed for this draft; exact current revisions are pinned in [sources](sources.md) |
| Two-host measurements | not started; all ten experiment families require approved cards and target access |
| Decisions contingent on measurements | backend, carrier, multipath, zero-copy, cache, distributed mode, service envelope, patch, and release policy remain open |
