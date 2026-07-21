# L13R primary-model retry result independent review

Date: 2026-07-21

Scope: read-only adversarial review of the accepted prerequisites, committed
controller and one-shot runner, exact source/configuration, raw evidence,
negative outcome boundary, production recovery, cleanup, milestone narrative,
and receipt.

Verdict: **ACCEPT THE NEGATIVE TERMINAL OUTCOME — NO P1/P2 FINDING REMAINS**

## Admission and transition

The prerequisite implementation and independent acceptance predate both the
final `c0972cb` runner/controller commit and the first production mutation at
03:40:41 PDT. The retained journals show exactly one maintenance stop/start
cycle: nimo-1 was stopped and deactivated by 03:40:42 before nimo-2 was stopped
at 03:40:43. This satisfies the executable transition authority and confirms
that no manual or second production attempt occurred.

The exact `bed36b7` source, coordinator and worker binaries, model size and
SHA-256, prompt and request hashes, compatibility/plan/topology/placement/
checkpoint identities, and complete retained invocation reconcile.

## Failure boundary

The first capture emitted only its exact invocation. The disposable PID 2093070
journal contains backend initialization and the UMA-memory query, then a
verified stop after 1.356 seconds. It contains no RPC ready banner, accepted
client, or state operation. No result, model-load completion, capture artifact,
or worker-local object exists.

The narrative correctly labels listener visibility before application readiness
as an inference rather than a proven root cause. It also correctly denies
zero-state-transfer, correctness, object-size/component, latency, throughput,
fallback, and retained-slowdown claims because the run did not reach those
gates.

## Recovery, cleanup, and evidence

The abnormal-exit recovery was worker-first: nimo-2 started at 03:42:11 on port
50052, followed by nimo-1 at 03:42:12. The standard production UD-Q6 model was
loaded and HTTP 200 was reached at 03:45:40.

The live read-only review confirmed:

- nimo-2 worker PID 1275544, exact command, listener/MainPID agreement on port
  50052, and `NRestarts=0`;
- nimo-1 coordinator PID 2093167, exact standard UD-Q6 command,
  listener/MainPID agreement on port 8081, HTTP 200, and `NRestarts=0`;
- port 50176 closed and its transient unit unloaded/inactive/dead; and
- disposable roots, keys, exact clones, and worker state absent, with the model
  untouched.

The evidence archive is 72,288,661 bytes and independently matches SHA-256
`6abfefa2671d8eaf7993ad3a3fe0458740b7bc4a92aa8457f8a1ed9e13af4a5d`.
Its zstd integrity test passes, the embedded checksum manifest matches the live
evidence root, and all 45 archived files verify.

The milestone and receipt therefore accurately close the single authorized
retry as **NOT PROMOTED**. No further lane is admitted.
