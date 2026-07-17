---
section_id: "84"
title: "On-Machine Research Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["Custom_Inference_Project", "HaloFPX integration repository (not yet frozen)"]
  software_versions: ["experiment-plan proposal v0.1"]
  hardware_revisions: ["two matched AMD Strix Halo hosts; exact revisions pending"]
related_sections: ["05", "18", "20", "24", "25", "29", "38", "55", "65", "73", "75", "76", "80", "81", "82", "83"]
---

# On-machine research design implications

## Deduplication rule

**[RECOMMENDATION]** Section-local experiment IDs remain traceability aliases, not separate executions. One Section 84 `HLX-EXP-YYYYMMDD-NNN` card may satisfy several upstream asks only when the raw schema contains every requested observation and the controls are matched.

| Master family | Consolidated inputs | Precedence/dependency |
|---|---|---|
| E01 identity/time/topology | Sections 18-23, 27-28, 67, 73, 75 | first; blocks every comparison |
| E02 metrology/collector qualification | Sections 05, 22, 27, 73, 75, 79 | after E01; blocks derived claims |
| E03 single-node baseline | Sections 24-37, 48, 73-74, 78 | after E02 and exact model inventory |
| E04 fabric | `FT-49-*`, `FT-50-*`, `S53-*`, `S54-*`, `S55-*`, Section 75 | after E02; E03 supplies concurrent decode controls |
| E05 buffer/coherence/runtime path | Sections 24-25, 32, 45, 54 | after E03; E04 supplies payload/link envelope |
| E06 HaloKV | Sections 56-65 and 77 | after E03/E05; durability before performance promotion |
| E07 distributed modes | `DR-38-*` through `DR-48-*`, Sections 55, 75-76, 78 | cache-off path after E03-E05; cache-integrated path additionally requires E06 |
| E08 service/stress | Sections 46, 66-72, 79 | after an E07 candidate and single-node fallback exist |
| E09 faults/security | Sections 48, 53, 56-64, 71-72, 80 | after clean correctness and recovery baseline; explicit authorization |
| E10 holdout/reproduction | Sections 73, 76, 78, 81 | last; uses frozen analysis and unseen randomized blocks |

**[INFERENCE]** The physical architecture choices are dominated by four early evidence sets: per-host compute/memory envelope (E03), message-size-dependent dual-link tails (E04), actual copy/coherence path (E05), and state/correctness costs (E06). Distributed mode benchmarking before these gates would diagnose neither compute nor fabric bottlenecks reliably.

## Cache-off versus cache-integrated E07

**[RECOMMENDATION]** `HLX-EXP-20260717-847` has two explicitly separated qualification paths:

1. `cache-off` is the mandatory architecture comparison. Persistent cache must be disabled and its absence recorded. Its dependencies are E03 single-node controls, E04 fabric, and E05 buffer/synchronization proof; it does **not** depend on E06 HaloKV.
2. `cache-integrated` is optional follow-on work. It may run only after E06 passes and must report cache state, hits/misses, restored work, I/O, and correctness separately from cache-off results.

A cache-integrated result cannot replace the cache-off control, and a cache-off result cannot support a HaloKV claim. This prevents an unfinished cache implementation from blocking distributed architecture qualification while preventing cache effects from contaminating its baseline.

## Canonical aliases

[`experiment-aliases.yaml`](experiment-aliases.yaml) is the machine-readable cross-section map. `M82-*`, `M83-*`, `EX85-*`, and `EXP-86-*` remain owning-section aliases; they do not allocate additional physical runs. A multi-card mapping means all named card evidence is required for the complete owning-section ask. `coverage: conditional` means the card runs only when the triggering change or authorized fault applies; it does not mean the alias is already satisfied.

## Gate semantics

Each card declares `gate_result: pass | fail | inconclusive | blocked`. `pass` means the predeclared acceptance rule is satisfied; it never means the hypothesis is universally true. `inconclusive` preserves all observations and schedules a revised card; it must not be converted to pass by post-hoc exclusion.

| Failed gate | Required response |
|---|---|
| identity/hash mismatch | abort comparison; correct manifest and start new run IDs |
| timing uncertainty too large | prohibit cross-host one-way timing; fall back to same-clock measures |
| collector perturbs workload | lower cadence/change collector and requalify E02 |
| correctness mismatch | quarantine candidate; no performance promotion |
| invalid cache accepted | safety failure; disable cache path and review format/validation |
| link or rank loss hangs/splits output | reject coupled mode until bounded failure semantics exist |
| holdout fails | do not write/accept ADR or release claim; investigate overfit/drift |

## Two-host synchronized notebook

**[RECOMMENDATION]** Store a logical notebook per experiment and immutable bundle per run:

```text
experiments/
  HLX-EXP-20260717-844_dual_link_fabric/
    card.yaml
    notebook.md
    runs/
      HLX-RUN-20260717T000000Z-a1b2/
        shared/manifest.json
        shared/time_alignment.json
        shared/event_log.jsonl
        node-01/environment.json
        node-01/stdout.log
        node-01/stderr.log
        node-01/observations.jsonl
        node-02/environment.json
        node-02/stdout.log
        node-02/stderr.log
        node-02/observations.jsonl
        hashes.sha256
        derivation/receipt.json
        derivation/summary.json
```

`notebook.md` is an append-only human narrative: UTC entry time, author/agent, card/run ID, planned action, actual deviation, observation, interpretation, and next action. `event_log.jsonl` is the synchronized machine ledger: `event_id`, run ID, node/rank, local monotonic timestamp, UTC timestamp, offset estimate, uncertainty, source, event type, and payload reference. Never order simultaneous cross-host events more finely than the recorded uncertainty permits.

## Decision linkage

**[RECOMMENDATION]** Every conclusion targets one of three outcomes:

1. close/update a stable open question in the owning technical section;
2. attach evidence to an existing or proposed `HLX-ADR-*` record in [Section 04](../../01_Wiki_Governance/04_Assumption_Open_Question_and_Decision_Ledgers/README.md);
3. update an implementation/release gate in Sections 82/81.

An ADR links the card, exact run IDs, raw hash manifest, derivation receipt, competing options, applicability, and review trigger. The wiki summarizes evidence; it does not replace the raw bundle.

## Architecture decision boundaries

- **[RECOMMENDATION]** Choose HIP versus Vulkan per pinned model/workload only after E03 correctness and paired performance.
- **[RECOMMENDATION]** Choose TCP, USB4STREAM, striping, alternation, or hedging only after E04/E10 matched codec and held-out tails; keep a recovery carrier.
- **[RECOMMENDATION]** Promote mapped/registered/zero-copy paths only after E05 proves synchronization and peer-visible data correctness.
- **[RECOMMENDATION]** Enable persistent cache only after E06 exact-continuation, compatibility-miss, crash, corruption, tenant, and endurance gates.
- **[RECOMMENDATION]** Choose replication, remote speculation, tensor parallelism, pipeline, or MoE hybrid by measured workload region, not a global ranking.
