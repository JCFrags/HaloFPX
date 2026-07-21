# L13R primary-model retry result

Date: 2026-07-21

Prerequisite implementation commit: `0fd867f118776a7313bc8119ecfb9bb32c781b20`

One-shot runner and controller hardening commit: `c0972cb5b9e779ba10d52dd04e4b8b61169fac50`

Exact inference source commit: `bed36b7753f3210945a32fccff99d1ed037dc40d`

Outcome: **NOT PROMOTED — TERMINAL ONE-SHOT STARTUP-READINESS BLOCKER**

## Decision

[MEASURED] Both authorized prerequisites passed and received independent
acceptance before production mutation. The repository controller then performed
the one allowed production maintenance transition in the required order. The
primary runner failed during its first capture, before it emitted a canary
result or loaded the model. The controller's abnormal-exit trap restored the
standard production orientation. No second primary attempt was made.

The failed run does not admit capture, cold, restart-restore, missing-object,
plan-mismatch, runtime-off, exact-continuation, zero-state-transfer, object-byte,
latency, throughput, or retained-slowdown results. No worker-local primary-model
object was created and no production cache path was enabled.

## Prerequisites and frozen input

[MEASURED] Prerequisite A exercised the actual disposable canary with 1,129
prompt tokens, a saved boundary of 1,128, `n_batch=512`, and three bounded
decode chunks. Capture, worker restart/restore, and clean cold recomputation had
exact suffix equality and zero state-window `GET_TENSOR`/`SET_TENSOR` transfer.

[VERIFIED] Prerequisite B replaced manual maintenance commands with
`scripts/halofpx-production-transition.py`. Fifteen focused transition tests,
the live non-mutating dry run, and independent review accepted its host, unit,
command, listener/PID, ordering, snapshot, and rollback guards. Four additional
primary-runner tests brought the focused combined total to 19 before the real
transition. Independent review accepted the committed runner/controller source
at `c0972cb` with no P1/P2 finding.

The one-shot runner revalidated the exact pinned input before starting the
disposable worker:

| Item | Exact value |
|---|---|
| repository | `rcmorano/saricles-MiniMax-M2.7-REAP-172B-A10B-ROCMFPX` |
| revision | `dba517197f2854f3d362529e13abddcdcad6c10b` |
| file | `saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf` |
| bytes | 159,873,097,824 |
| SHA-256 | `96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6` |
| prompt SHA-256 | `f20c7c7a4137de98b991f1bfe6de27e194a93c7257d9496944e312085923143f` |
| request-body SHA-256 | `f5b273f95a852d5e34252715bc953fa4eb101273d262b9d778c16138d7c00b7c` |
| prompt / saved / generation target | 1,129 / 1,128 / 128 tokens |
| seed / temperature | 1234 / 0 |
| coordinator canary SHA-256 | `6cbfd802403adfd03b20c924f9f38aff8f384710aaabd3c915ae5fd7b547c19d` |
| worker binary SHA-256 | `7bc3b27776ec808f50b40e7fa9bb6d9026f2488dea92b9d768a4dd6e90405d14` |

The compatibility, plan, topology, placement, and checkpoint identities remained
`a8f921ae...`, `0268cc60...`, `09b71fe4...`, `d4aa0d3c...`, and
`421016c4...`, respectively. The retained invocation records the complete
64-character values and exact inference flags; no unrelated setting was tuned.

## One allowed transition and failure

[MEASURED] The controller's create-once preflight snapshot recorded nimo-1
coordinator PID 2068256 on port 8081 with HTTP 200 and nimo-2 worker PID 1247685
on port 50052, both with `NRestarts=0`, together with exact unit, ExecStart,
process command, listener PID, model, and start-time authority.

The controller stopped nimo-1 first at 03:40:41 PDT and verified it inactive
before stopping nimo-2 at 03:40:43. The disposable nimo-1 worker started as PID
2093070 on port 50176 at 03:42:09. Its journal reached ROCm/Vulkan initialization
and the UMA-memory query, but contains no `Starting RPC server`, accepted-client,
or `[halofpx-state]` operation. The primary canary returned nonzero without any
stdout/stderr beyond the retained exact invocation. Cleanup stopped the worker
at 03:42:10 after 1.356 seconds; no canary result file, capture artifact, or
worker object exists.

[INFERENCE] The runner's readiness contract was insufficient for this binary:
it accepted an active unit whose port-50176 listener PID matched `MainPID`, but
the socket became observable before the RPC application emitted its own ready
banner or accepted clients. The evidence supports this startup-readiness race as
the immediate failure explanation. It does not establish a model, protocol, or
state correctness failure because none of those paths produced a result.

Although the failed worker journal contains no `GET_TENSOR` or `SET_TENSOR`
operation, this is not promoted as the requested zero-payload state-path proof:
the run never reached a state operation.

## Recovery and cleanup

[MEASURED] The controller's abnormal-exit recovery started and verified the
nimo-2 production worker first at 03:42:11, then started nimo-1 at 03:42:12 and
waited until the standard UD-Q6 model reported loaded and HTTP 200 at 03:45:40.
The final controller snapshot reconciled exact commands, listeners, PIDs, start
times, model authority, and unchanged restart counters:

| Host / role | Final authority |
|---|---|
| nimo-2 RPC worker | active, PID 1275544, port 50052, `NRestarts=0` |
| nimo-1 coordinator | active, PID 2093167, port 8081, HTTP 200, `NRestarts=0` |

The coordinator command names the standard production
`MiniMax-M2.7-UD-Q6_K_XL-00001-of-00006.gguf`, not the pinned ROCmFPX canary
artifact. Port 50176 is closed. The transient unit is unloaded. The disposable
primary roots, protected control keys, exact build clones, and worker state root
were removed only after evidence sealing; the model and evidence archives were
not deleted.

## Evidence and scope closure

The final raw bundle is retained on nimo-2 at
`/var/tmp/halofpx-l13r-primary-retry-evidence-20260721-v1.tar.zst`, size
72,288,661 bytes, SHA-256
`6abfefa2671d8eaf7993ad3a3fe0458740b7bc4a92aa8457f8a1ed9e13af4a5d`.
It contains the controller preflight/final snapshots, both production journals,
the disposable journal, exact failure invocation, request and identities,
source/binary/model hashes, build/configuration logs, disk counters, root
inventory, unit/launcher evidence, a source archive, the committed source
bundle, and a per-file checksum manifest.

This consumes the single authorized L13 retry. It is a reviewed negative
milestone and does not open another attempt, production cache enablement,
eviction, shared/prefix reuse, cable faults, broad fault injection, performance
tuning, or G9/G10 trials. Further work requires new Project Lead steering.
