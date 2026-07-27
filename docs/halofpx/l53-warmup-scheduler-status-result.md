# L53 warmup scheduler-status result

Status: **NOT PROMOTED**
Date: 2026-07-27
Base: `d236e74d2b2c3df96d88ef4cce5269d1baf3f24a`

## Outcome

L53 reconstructed the exact L52 candidate from the preserved source archive
and audited the retained coordinator and worker chronology against the real
warmup, scheduler, RPC, L42, and L44 call paths. The evidence does not support
the proposed unarmed-warmup defect.

The source-defined common warmup must use the ordinary unarmed path. Retained
runtime evidence nevertheless contains an authenticated execution-sequence-1
RPC graph during that warmup; the worker executed it successfully before the
coordinator returned `llama_decode=-3`. This contradicts the reconstructed
explicit arm guards.

The retained evidence contains neither the client arm/disarm transitions nor
the exact coordinator refusal branch. It therefore cannot distinguish a
runtime binary/lineage mismatch from an uncovered arm-state transition.
Several client-side fail-closed branches also collapse to
`GGML_STATUS_FAILED`/`-3` without branch-specific evidence.

No source-backed warmup correction was therefore available within L53. The
candidate was not modified speculatively, no stories qualification was run,
and L53 closes **NOT PROMOTED**. A future decision would need to authorize the
smallest no-primary discriminator that records exact source/binary lineage,
context pending state, RPC arm/disarm transitions, per-split armed state, and
the exact client refusal branch; L53 does not grant that authority.

## Focused checks

- Reconstructed L52 candidate matched the preserved archive.
- Focused controller/result tests passed: `12 passed`.
- Source audit verified explicit canary arming begins in
  `authorized_decode`, while common warmup calls `llama_decode` directly.
- RPC graph authority is gated by the explicit RPC execution-arm state.
- No production host mutation, disposable runtime, primary artifact access,
  model load, or remote build occurred.

The rejected runtime candidate was removed from the terminal worktree. Only
this closeout and its bounded audit evidence remain.

## Evidence and safety

The source audit is retained under
`docs/halofpx/evidence/l53-raw/source-audit.txt`. L52 evidence remains immutable
under `docs/halofpx/evidence/l52-raw/`.

Production was not touched in L53. The accepted L52 production and cleanup
reconciliation remains the last runtime authority. No disposable L53
resources were created.

## Review

Independent adversarial review was requested against the retained chronology,
source gates, and the decision not to invent a warmup correction. Its verdict
is retained in `docs/halofpx/evidence/l53-raw/independent-review.txt`.
