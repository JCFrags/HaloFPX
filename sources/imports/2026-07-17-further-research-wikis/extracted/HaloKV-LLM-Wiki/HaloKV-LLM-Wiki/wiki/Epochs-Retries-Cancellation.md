---
title: "Epochs, retries, and cancellation"
tags: ["epoch", "fencing", "idempotency", "cancellation"]
created: 2026-07-17
updated: 2026-07-17
status: design-proposal
sources: ["COORD-01", "RPC-02", "RPC-03", "RPC-04", "RPC-05"]
related: ["Checkpoint-Commit-Protocol", "Protocol-Overview", "Degraded-Mode-Behavior"]
---

# Epochs, retries, and cancellation

## Monotonic identity fields

| Field | Scope | Change rule | Purpose |
|---|---|---|---|
| `session_generation` | logical session | increments on reset, topology migration, or incompatible recovery | prevents old session state from reappearing |
| `epoch` | generation | issued monotonically by authority | fences stale coordinators/ranks |
| `checkpoint_seq` | generation | strictly increases for committed checkpoint names | orders checkpoints |
| `op_id` | operation | random 128-bit value, stable across retries | deduplicates one intended action |
| `request_digest` | RPC | digest of canonical request body | detects `op_id` reuse with different parameters |

The receiver persists `highest_authorized_epoch` before performing a mutation. Transport reconnect, wall-clock time, process start time, and heartbeat freshness do not grant an epoch.

## Epoch acceptance

- `message.epoch < highest_authorized_epoch`: reject `STALE_EPOCH` without side effects.
- `message.epoch == highest_authorized_epoch`: process idempotently if other fields match.
- `message.epoch > highest_authorized_epoch`: require an authority-signed/verified grant whose revision is newer; persist it, then process.
- A peer cannot bootstrap a higher epoch by merely claiming one.

Old epoch messages remain rejectable after restart because the high-water mark or authority revision is durable.

## Retry contract

| Operation | Retryable? | Required identity | Ambiguous-result action |
|---|---:|---|---|
| `Hello` / status query | yes | request ID | retry within deadline |
| Begin checkpoint | yes | same `op_id` and request digest | query operation; repeated begin returns existing state |
| Put immutable page | yes | page digest, offset, transfer ID | stat/verify page and resume missing ranges |
| Rank prepared | yes | operation identity + rank manifest digest | coordinator returns recorded preparation |
| Commit publication | only as same CAS | checkpoint name + `op_id` + expected authority revision | authoritative query before another attempt |
| Cancel | yes | operation identity + monotonic cancel sequence | query terminal state |
| Delete/GC | yes when reference-checked | object ID + GC run ID | re-evaluate reachability |

Use exponential backoff with jitter and an end-to-end deadline. Do not enable transparent retries for non-idempotent or state-advancing methods without the application identity above. `UNAVAILABLE` may be transient; `FAILED_PRECONDITION`, `ABORTED`, `DATA_LOSS`, `PERMISSION_DENIED`, and validation failures require state resolution or operator action rather than blind retry.

## Cancellation semantics

RPC cancellation means the caller no longer wants work; checkpoint cancellation is a protocol state transition. Keep them separate.

`CancelCheckpoint` carries operation identity, `cancel_seq`, reason code, and request digest. The authority returns one of:

- `CANCELLED_BEFORE_PREPARE`;
- `ABORTED_PREPARED`;
- `ALREADY_ABORTED`;
- `ALREADY_COMMITTED`;
- `REJECTED_STALE`.

Workers poll or receive cancellation while snapshotting, hashing, uploading, waiting for credits, and staging GPU memory. They stop at safe points and release reservations. Immutable objects already published become orphan candidates; cleanup is asynchronous and cannot delay the terminal abort record.

## Commit-versus-cancel linearization

Commit and cancel update the same authority record with conditional revision. Exactly one terminal transition wins. A late `RankPrepared`, upload completion, or reconnect from the cancelled epoch cannot reopen the operation.

## Deadline propagation

All RPCs have realistic deadlines. The coordinator propagates the remaining budget to rank and storage calls, reserving time to record a safe abort or query ambiguous state. Deadline expiry cancels local work but does not imply the globally visible operation aborted; authority remains the source of truth.

## Retry storms and fencing storms

Backoff is scoped by session and tenant, with server-provided retry hints. A coordinator recovering many sessions should randomize reconciliation and prioritize active requests over orphan cleanup. Epoch bumps are rate-limited and audited; repeatedly bumping epochs on transport blips creates avoidable cache invalidation and stale-worker churn.
