---
section_id: "46"
title: "Scheduler Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX proposed runtime"]
  software_versions: []
  hardware_revisions: ["two matched AMD Strix Halo systems (exact BOM open)"]
related_sections: ["39", "40", "43", "45", "48", "58", "68"]
---

# Design implications

## Admission and ownership

**[RECOMMENDATION]** Use a two-phase reservation for distributed plans: coordinator checks tenant and global queue budgets; all required ranks return a lease containing free KV/activation capacity and transport credits; the coordinator commits one `session_epoch`. Any reject/timeout releases all leases.

**[RECOMMENDATION]** Define a compatibility key: model/content hash, tokenizer, adapter set, backend/plan ID, KV format, attention/recurrent mode, sampling execution location, and deterministic-mode flag. Only compatible sequences share a decode call.

## Policy

**[RECOMMENDATION]** Schedule with hierarchical deficit round robin: tenant/user class first, sequence second. Charge estimated token-work rather than requests. Within a selected class, run deadline-aware decode steps before bounded prefill chunks, but age prefills so they cannot starve.

| Mode | Rank ownership and scheduler consequence | Failure / fallback |
|---|---|---|
| Replication | One node owns the whole sequence; coordinator routes by cache locality and load. | Retry only before externally visible output, or resume from a validated checkpoint; route new work to surviving replica. |
| Remote draft | Target rank owns canonical KV, logits, sampling, and acceptance; draft rank owns disposable proposal state. | Cancel draft and continue target-only. |
| Tensor parallel | Coordinator owns sequence; both ranks jointly own each step's shards and must execute identical collective order. | Abort affected epoch; restart single-node from prompt/checkpoint only if full model fits. |
| Pipeline | Each rank owns contiguous layers and layer-local KV; coordinator owns microbatch order. | Abort affected microbatches; restart under a new plan. |
| MoE hybrid | Ownership follows frozen plan manifest for dense layers, experts, and KV. | Do not reroute an expert mid-step; replan only at a safe checkpoint boundary. |

## Backpressure and streaming

**[RECOMMENDATION]** Bound four layers independently: HTTP body/connections, admission queue tokens, rank work rings, and per-stream output bytes. Stop admitting before ring or KV exhaustion. Return a retryable overload response with `Retry-After`; never accept then wait indefinitely.

**[RECOMMENDATION]** A slow stream consumer must not block the inference thread. Use a bounded replay buffer; when full, either cancel by declared policy or drop the connection while retaining only a bounded resumable window. Cancellation must be idempotent and carry `(session_id, epoch, cancel_after_step)` to every rank.

## Safety margins

**[RECOMMENDATION]** Keep unallocatable headroom for allocator fragmentation, transport buffers, graph variants, and cancellation cleanup. Derive it from worst observed high-water marks plus confidence bounds; do not hard-code another runtime's percentage.
