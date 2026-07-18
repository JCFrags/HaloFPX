# HaloFPX project knowledge

Purpose: provide compact, evidence-routed reasoning for agents preparing the HaloFPX integration fork.

Does not belong here: raw research, long Wiki prose, implementation code, benchmark output, approved ADRs, or procedures presented as validated.

## Authority boundary

- The user-directed destination and phase order remain in [`PROJECT_GOAL.md`](../PROJECT_GOAL.md).
- Exact repository identity and license observations remain in [`sources/repositories/manifest.yaml`](../sources/repositories/manifest.yaml).
- Target-machine facts remain in the [2026-07-17 live inventory](../sources/measurements/2026-07-17-strix-halo-live-inventory/README.md).
- The [canonical Wiki](../wiki/HaloFPX_Wiki/README.md) owns source-backed research context and OPEN questions.
- Reviews and plans remain review artifacts. This layer makes them easier to retrieve; it does not approve them.
- Preserved imported Wikis remain candidate evidence under `sources/imports/`, even when their own validation passes.

Claim labels are literal: `[VERIFIED]` is source-backed, `[MEASURED]` is scoped to the captured environment, `[INFERENCE]` and `[RECOMMENDATION]` require validation or decision, and `[OPEN]` is unresolved.

## Modules

1. [Source baseline and authority](source-baseline-and-authority.md) — which pins and artifacts control research, qualification, deployment comparison, and future implementation.
2. [Integration and license boundaries](integration-and-license-boundaries.md) — what may be retained, adapted, independently reimplemented, or rejected.
3. [Cache and state safety invariants](cache-state-safety-invariants.md) — the acceptance boundary for model tensors, prefix state, and session continuation.
4. [Dual-node transport and capacity constraints](dual-node-transport-and-capacity-constraints.md) — measured host limits, current carrier, experimental transport, ownership, and failure constraints.
5. [Implementation readiness gates](implementation-readiness-gates.md) — the shortest defensible path from research control to authorized implementation.
6. [Pre-fork research return synthesis](pre-fork-research-return-synthesis.md) — decision routing from the 2026-07-18 PF-IR returns and the immediate next sequence.

Read the baseline and readiness modules before starting work. Load the integration, cache, or transport module only when its lane is active.

> This is a maintained retrieval layer, not permanent truth. Update it only after the controlling source, Wiki status, review decision, or measured environment changes.
