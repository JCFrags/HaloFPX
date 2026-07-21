# L13 primary-model worker-local state canary

Date: 2026-07-21

Authorized base documentation commit: `51922809e068d78c22deb4711a2964362ccb2e6e`

L12 implementation commit: `6444d1e173aa7c0d5bea6c2b3539d1fd936f3f65`

Harness commits: `52f7991b5460d30f294d4152d7b97cfe4b50518a`,
`bed36b7753f3210945a32fccff99d1ed037dc40d`

Outcome: **NOT PROMOTED — SAFETY STOP GATE**

## Decision

[MEASURED] The authorized primary-model scale gate did not complete. No L13
capture, cold, restart-restore, corrupt/missing-object, identity-mismatch, or
matched runtime-off result is admitted. No worker-local primary-model object
was published, no production cache path was enabled, and no correctness or
performance claim follows from this attempt.

[MEASURED] Two bounded operational failures caused the run to stop:

1. The first disposable capture loaded the exact primary model, then the
   coordinator canary asserted because its helper submitted all 1,128 prefix
   tokens to `llama_decode` while the configured batch bound was 512. The
   disposable context was destroyed before state capture. The harness now
   chunks the prefix by `llama_n_batch`, but that correction did not receive
   any successful runtime proof in L13. The retained post-fix smoke attempts
   also did not complete successfully, so the correction remains unqualified.
2. During the retry, the production RPC worker was stopped before the
   production coordinator because the coordinator stop command was issued on
   the wrong host. This violated the explicit stop-order contract. The still-
   active coordinator aborted after the RPC endpoint disappeared. No
   disposable worker or coordinator had been started, and the retry ended
   immediately.

The second event is a production-recovery stop gate, so this milestone is a
negative operational result even though the services recovered.

## Frozen input and preflight

[VERIFIED] The pinned model was present only as the authorized artifact and was
not substituted:

| Item | Exact value |
|---|---|
| repository | `rcmorano/saricles-MiniMax-M2.7-REAP-172B-A10B-ROCMFPX` |
| revision | `dba517197f2854f3d362529e13abddcdcad6c10b` |
| file | `saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf` |
| bytes | 159,873,097,824 |
| SHA-256 | `96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6` |
| request SHA-256 | `f5b273f95a852d5e34252715bc953fa4eb101273d262b9d778c16138d7c00b7c` |
| prompt tokens | 1,129 |
| saved token boundary | 1,128 |
| generated-token target | 128 |

[MEASURED] Preflight retained source, binary, model, request, plan, topology,
placement, compatibility, and checkpoint hashes; exact production unit
definitions, commands, PIDs, listeners, HTTP state, start times and restart
counters; free bytes/inodes, memory/GPU state, disk counters, protected path
permissions, rollback commands, evidence paths, and stop gates. The worker
object estimate was 152,180,736 bytes at the 1,128-token boundary and the
conservative 4,096-token estimate was 552,599,552 bytes, against
27,521,658,880 free bytes on the worker at initial preflight.

Attempt 1 used source `52f7991b5460d30f294d4152d7b97cfe4b50518a`
(tree `7faefec64cf11ea75966cf75d79b07a8e869b994`), coordinator
binary `7b0305951c252906ca08e41459e5981f83b2aa4856f6dbe3e228c3eec959deda`,
and compatibility root
`f74e354d60e4f5e0a8a03b32b70991e9bfc0e393f0baba362c223d9b6bf6c921`.
That is the build which reached the assertion.

The retry preflight used source
`bed36b7753f3210945a32fccff99d1ed037dc40d` (tree
`5281a6d2a3480a62dd34d7d9bb603699ef25c11a`) and coordinator binary
`3a64f1bc143963edfd56040243b9f9f9114751bb31415a8a55631f5c06375963`.
The retry never launched that binary because the production stop-order gate
fired first. Its exact scale identities were:

| Identity | SHA-256 |
|---|---|
| plan | `0268cc6071a8d78983f6351fe45d510e767d8cd26618a8bdffc972b6655f7967` |
| topology | `09b71fe40ae05c841a5be563f6e2b27ad2529d893b9420412e5280541ae53e1f` |
| placement | `d4aa0d3c14a3bec4ba5de733e00b6447f79f94d5dbeda6e3593be74ce84f917e` |
| compatibility | `a8f921ae8742823eac2942004094d1d11f47962bae0607c4b2fce6ce5a81c36f` |
| checkpoint | `421016c41e1af022aa65feef9c7b9329fdc1b49ff0b1c4df4aaad10cf13bf816` |

The nimo-1 retry-preflight disposable worker binary SHA-256 was
`c22013f0f60f9361c151e8382672d4a0a2491c8fcea0f19968ff41b173498b6b`.
Focused protocol probes passed before the primary run, but they do not replace
the missing scale result.

## Production recovery

[MEASURED] After the first harness assertion, the disposable service was
stopped and production was restored worker first, then coordinator, to HTTP
200. After the stop-order violation, the coordinator was explicitly stopped,
the nimo-2 worker was started and verified on port 50052, and then the nimo-1
coordinator was started and allowed to finish loading before HTTP verification.

Final production authority:

| Host / role | State |
|---|---|
| nimo-2 RPC worker | active, PID 1247685, port 50052, `NRestarts=0` |
| nimo-1 coordinator | active, PID 2068256, port 8081, HTTP 200, `NRestarts=0` |

The coordinator journal retains the abort caused by losing the RPC worker and
the subsequent clean service start. The new PIDs and start times are expected
from the explicit recovery. The standard production UD-Q6 model and service
configuration were restored; no model was deleted. All disposable services
were stopped, ports 50176 and 50177 were closed, and disposable state roots,
keys, exact clones, intermediate builds, and redundant intermediate archives
were removed after the final evidence bundle was sealed.

## Evidence and scope closure

The raw evidence bundle is retained on nimo-2 at
`/var/tmp/halofpx-l13-primary-evidence-20260721-v3.tar.zst`. Its SHA-256 is
recorded in the L13 receipt. It contains both-node preflight and production
snapshots, exact identities and request, build and protocol-test logs, source
bundle and hashes, the first capture failure, disposable-worker journals, the
stop-order incident, and final production reconciliation.

L13 does not admit production cache enablement, eviction, shared or prefix
reuse, cable faults, a broad fault matrix, G9/G10 trials, or final performance
claims. A future retry requires a new Project Lead decision and an operator
runbook that binds each production action to its authoritative host and
verifies coordinator inactivity before worker stop. This task does not open
that lane.
