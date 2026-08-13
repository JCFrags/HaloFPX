# Issue #28: sampling-output synchronization canary

Status: implementation candidate; default off; no target-machine performance
claim.

Tracker: [GitHub issue #28](https://github.com/JCFrags/HaloFPX/issues/28)

## Boundary

The ordinary CPU sampling path reads several pieces of one output row: sampled
token, logits, probabilities, candidate IDs, and their counts. Historically,
each public getter executes a scheduler-wide synchronization even after an
earlier synchronization already completed for the same output epoch.

`HALOFPX_SAMPLING_SYNC_COALESCE_CANARY` is a CMake option that defaults to
`OFF`. When enabled, internal output readers may reuse a completed scheduler
barrier until a graph submission, output-copy publication, buffer reserve, or
lifecycle reset invalidates that epoch. Public `llama_synchronize()`
remains unconditional. State save/load and other lifecycle barriers do not use
the coalescing helper.

The canary also introduces an internal borrowed output-row view with explicit
`RAW`, `SAMPLED_TOKEN`, `SAMPLED_LOGITS`, `SAMPLED_PROBS`, or `UNAVAILABLE`
provenance. Candidate count and token IDs are validated against the same
synchronized epoch. A raw-only request never falls back to sampled data;
partial or invalid sampled metadata fails closed instead of combining sampled
counts with raw storage.

## Concurrency and lifetime

Graph dispatch and every host-output async-copy phase hold an active mutation
guard. Synchronization publishes readiness only if no publication is active and
the mutation serial did not change across the barrier. This prevents a
synchronous scheduler callback from priming readiness before later graph splits
or output copies are queued. Failed or partial graph submission also leaves the
epoch invalid.

The latch is per context, so target and draft contexts do not share readiness.
It does not make same-context access thread-safe. Borrowed row pointers remain
valid only until the next graph submission or output-buffer mutation on that
context.

## Cumulative metrics

When the standard server `--metrics` endpoint is enabled, it exports the five
existing context-lifetime counters from the non-synchronizing output snapshot:

- `llamacpp:halofpx_sampling_sync_output_epochs_total`;
- `llamacpp:halofpx_sampling_sync_completed_barriers_total`;
- `llamacpp:halofpx_sampling_sync_reused_barriers_total`;
- `llamacpp:halofpx_sampling_sync_graph_submissions_total`; and
- `llamacpp:halofpx_sampling_sync_output_transfers_total`.

The output-epoch counter advances at output reserve/reset boundaries; it is not
a request, generated-token, or completion count. All values are exact unsigned
64-bit decimal integers. They are not attached to a completion, slot, or SSE
event, and the `/slots` route does not acquire them. A feature-off build retains
historical barrier behavior and therefore reports zero reused barriers; a
feature-on build may report reuse. All five counters reset with the
context/process, so matched evidence must retain process identity and compare
before/after snapshots from the same fresh process.

## Focused evidence and nonclaims

- **[VERIFIED]** Focused hosted tests exercise historical feature-off barrier
  counts, canary reuse, forced public synchronization, epoch invalidation,
  callback reentry, mutation during a barrier, and independent context latches.
- **[VERIFIED]** The pure row validator tests explicit unavailable results,
  raw-only provenance, coherent full and reduced sampled rows, count equality,
  candidate ID range, selected-token membership, and token-only backend output.
- **[VERIFIED]** The deterministic state-machine fixture submits identical
  simulated graph and output-transfer work for `OFF` and `ON`; one forced
  public synchronization plus five internal reads execute six barriers with
  `OFF`, while `ON` executes one and reuses that completion five times.
- **[VERIFIED]** A deterministic control-host test preserves every counter
  exactly above `2^53`, including `UINT64_MAX`, without a JSON or
  floating-point numeric conversion. A source contract keeps the getter
  metrics-only and the canary default off. This is not target or hosted
  performance evidence.
- **[INFERENCE]** Removing redundant completed barriers could reduce generation
  host/backend synchronization overhead. Hosted counts do not establish an
  end-to-end speed gain.
- **[OPEN]** Exact-output parity and timing on the two CachyOS Strix Halo
  machines, including HIP/RPC, grammar, log-probability, backend-sampling, and
  target/draft paths.

Windows control-host compilation and tests are not Strix Halo performance
evidence. The planned matched target experiment is documented in
[`issue-28-sampling-output-sync-target-plan.md`](issue-28-sampling-output-sync-target-plan.md).
The optional `sampling_output_sync_prometheus_v1` sidecar is documented in the
[model-general Strix A/B harness](strix-ab-harness.md#optional-sampling-output-synchronization-observability).
It imports exact single-fresh-process `/metrics` windows without treating the
cumulative counters as general request, completion, or SSE attribution. Its
fake-adapter tests are control-host contract evidence only.

## Kill gates

- Any token, raw-logit, probability, candidate, output-row, or feature-off
  mismatch blocks timing and promotion.
- Any unavailable row, crash, failed request, or counter inconsistency blocks a
  performance claim until localized.
- The option remains default off unless matched target evidence establishes
  correctness and a useful generation result.
