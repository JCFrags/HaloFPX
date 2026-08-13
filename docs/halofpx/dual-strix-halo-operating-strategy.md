# Dual-Strix-Halo operating strategy

Status: **[RECOMMENDATION] source/evidence audit complete at
`3d9a0c3cc52168f696d600099742c7caf964161f`; target execution is `REFUSE`
under issue [#41](https://github.com/JCFrags/HaloFPX/issues/41)**

This document chooses operating modes and the next bounded measurements for
model-general ROCmFPX-family GGUF inference on the two CachyOS Strix Halo
machines. It is not a current-main speed claim. Historical MiniMax results are
used only where named; MiniMax remains a capacity/stress fixture, not the model
the engine is optimized around.

## Current-main routing

The exact audited main adds no target result. Its four newest merged lanes
change how this strategy is executed, not the evidence boundary:

- **[VERIFIED]** PR [#53](https://github.com/JCFrags/HaloFPX/pull/53), exact
  commit `b6b0d46c461819edafe81a631ba9500d04fae008`, adds the default-off strict
  `n=1` local-HIP Q/K/V MMVQ Q8_1 reuse candidate. It has GPU-less `gfx1151`
  compile/link evidence, but no target runtime, performance, real-model, or
  remote-RPC reachability result.
- **[VERIFIED]** PR [#55](https://github.com/JCFrags/HaloFPX/pull/55), exact
  commit `49af358dd37dd3bb0d8f6f2daf8581cd5d6f0ba2`, supplies the accepted
  plan-v2 `512/512` versus `2048/512` runtime comparison contract. It keeps the
  source and each role's complete artifact object identical across conditions
  and changes only the typed outer-batch argument; its real target path remains
  hard-disabled.
- **[VERIFIED]** PR [#57](https://github.com/JCFrags/HaloFPX/pull/57), exact
  commit `323fa3ee684a76ca5f4f42ef33f5bc38487fc74d`, exports five exact
  context-lifetime sampling/output counters through the `--metrics`-enabled
  `/metrics` endpoint. They reset with the context/process and are not request,
  slot, or completion attribution.
- **[VERIFIED]** PR [#56](https://github.com/JCFrags/HaloFPX/pull/56), exact
  commit `3d9a0c3cc52168f696d600099742c7caf964161f`, hardens offline fresh-PC
  continuation and retains same-PC recovery provenance. A full fresh-PC `PASS`
  and target execution remain open; it does not relax issue #41.

## Decision summary

| Model/workload condition | Preferred mode | What two machines buy |
|---|---|---|
| The exact model, target context, KV, graph, scratch, and operating headroom fit safely on either node | Two independent single-node replicas with request/session affinity | Potential aggregate throughput, independent cache locality, and failure isolation. Do not claim lower latency for one request. |
| The exact model or required context does not fit one node | Current contiguous RPC layer split | Capacity. One logical remote-prefix/local-suffix boundary, possibly a tensor bundle, per work unit and layer-local KV. Do not call capacity a speedup. |
| Several independent sequences or prompt chunks are continuously ready, and every non-CPU backend has negotiated async compute and events | Future bounded layer-pipeline mode | Potential prompt and multi-request throughput from overlapping stages. Current RPC cannot enter this mode. |
| One autoregressive sequence needs lower inter-token latency | Per-rank kernel work, verified MTP/speculation, or a later intra-token parallel plan | Removes or amortizes work inside a token. Ordinary layer pipelining cannot overlap token `t+1` before token `t` is committed. |

Do not select a mode from model byte size alone. Admission must include exact
weight allocation, KV at the intended context and slot count, graph copies,
scratch, allocator headroom, and OS/HMM ownership. The 2026-08-12 global-OOM
incident proves that ordinary `MemAvailable` and process RSS are insufficient
while KFD/render/HMM pages are owned by another workload.

## Exact current two-rank architecture

At the audited commit:

- **[VERIFIED]** An explicit non-tensor `--device` list is retained in the
  supplied order; default discovery separately inserts RPC devices before
  local GPUs. Thus explicit `--device RPC0,ROCm0` makes the worker the first
  accelerator and the coordinator's local GPU the second (`src/llama.cpp`).
- **[VERIFIED]** Layer mode assigns contiguous layer ranges from normalized
  split points. The input embedding remains on CPU, repeating layers follow
  the resolved device map, and the output layer follows the final logical
  layer assignment (`src/llama-model.cpp`).
- **[VERIFIED]** Offloaded K/V buffers use the device that owns each repeating
  layer (`src/llama-kv-cache.cpp`). KV therefore remains layer-local during
  ordinary inference.
- **[VERIFIED]** RPC advertises `async=false` and `events=false`, and its
  tensor async/copy/event interface functions are absent. The context disables
  the ggml multi-copy pipeline when any accelerator lacks either capability
  (`ggml/src/ggml-rpc/ggml-rpc.cpp`, `src/llama-context.cpp`).
- **[VERIFIED]** Ordinary RPC graph compute/recompute submission is already
  send-only. The following ordered output/boundary `GET_TENSOR` is the blocking
  completion boundary. **[MEASURED]** In the historical P05 fixture,
  [P05](p05-rpc-small-command-coalescing-rejection.md) found no useful
  end-to-end gain after removing 252 sends.
  **[INFERENCE]** Small-command framing was therefore not that fixture's main
  missing round trip.

For the historical 62-repeating-layer MiniMax ROCmFPX fixture, the exact
`RPC0,ROCm0` / `1,1` resolver result is:

| Owner | Repeating layers | Other work/state |
|---|---:|---|
| RPC worker | prefix `[0,32)` | K/V for those 32 layers; remote graph/buffers |
| Local coordinator ROCm GPU | suffix `[32,62)` | K/V for those 30 layers; output layer |
| Coordinator process/CPU | none | input embedding placement, context, request/slot/token history, sampler, output commit |

These are logical roles, not permanent hostnames. **[MEASURED]** In the scoped
historical P07/P08 disposable experiment tuples, nimo-1 was worker and nimo-2
was coordinator; their restored production state and the latest retained
production snapshot in issue #41 have the roles reversed. Every receipt and
cache plan must record both logical rank and physical host/boot/process
identity.

### Logical placement is not physical GGUF sharding

**[VERIFIED]** The current coordinator parses the GGUF and supplies tensors
allocated on the RPC device; the worker does not independently open a model
shard and construct its own model context. The optional RPC tensor cache names
transfers larger than 10 MiB with 64-bit FNV-1a and can avoid retransmission
when that filename exists. The selected file is not rehashed before it is
applied. That mechanism is useful only as restart/load-path evidence: its hash,
namespace, object validation, and lifecycle are not a HaloFPX model-integrity
or persistent-KV contract.

Keep startup separate from steady inference:

- **[INFERENCE]** Current remote load traffic is approximately the bytes of weights and other
  immutable buffers assigned to RPC, minus any admitted tensor-cache hits;
- **[VERIFIED]** Request prompt/decode traffic is graph/control plus boundary and output
  tensors, not the model file again; and
- **[RECOMMENDATION]** A future rank-local loader should open only manifest-owned tensors from a
  strong content-addressed model source, prove exact SHA-256 identities on both
  ranks, and declare ready only after allocation and a boundary self-test.

**[INFERENCE]** Rank-local loading could improve restart time and remove large
model transfers. It cannot be reported as a prompt or generation speedup unless
a matched steady-state request changes.

## Mode A: two independent replicas

Use replication whenever each node safely fits the whole runtime tuple.

- Keep model weights, live KV, prompt-cache objects, slots, and sampler state
  local to the node serving the session.
- Route new sessions to the less-loaded healthy node, then preserve session
  affinity. Do not move live KV over USB4 during normal service.
- Compare two-replica aggregate throughput at the intended concurrency with
  one-node throughput. The optimistic ceiling is the sum of the independently
  measured node rates, not an assumed `2x`.
- A failed node loses or replays only its own active sessions. The other node
  may accept new work; it must not treat the failed node's incompatible or
  absent state as a cache hit.

Replication is the default throughput architecture for fitting models. Sending
one request through both nodes adds a fabric dependency and should be justified
by matched latency or capacity evidence.

## Mode B: contiguous layer-split capacity

Use layer split when the model/context cannot run safely on one node. Keep the
remote rank as a contiguous prefix and the coordinator-local rank as the
suffix/output owner so only a bounded activation bundle crosses the main
forward boundary.

For a work unit of `M` tokens, hidden width `H`, and `e` bytes per boundary
element, the simplest logical boundary payload is:

`B_boundary = M * H * e`

The real manifest must add every residual, mask, routing, recurrent, or
multimodal tensor that crosses the cut. Its measured transfer time is
`X(B_boundary)`, including staging copies and completion latency.

Current single-sequence decode is structurally serial:

`T_decode(k) = C_rpc([0,k)) + X_k + C_local([k,L)) + O`

The best current cut minimizes that sum while satisfying memory and
correctness constraints. It does **not** minimize `max(C_rpc,C_local)` unless
useful stage overlap actually exists.

### Exact cut screen

For `L` repeating layers plus an output layer, let `N = L + 1`. With all `N`
assignments offloaded, a candidate with exactly `k` prefix assignments can be
requested as `--tensor-split k,N-k` and must be verified by the same
pre-allocation resolver before opening the model. The output assignment remains
local only when `k <= L`; a smaller `n_gpu_layers` changes the mapping and must
not reuse this formula.

Use this bounded, model-specific screen:

1. Start from the resolver-verified `1,1` cut `k0`.
2. Capture one untimed/instrumented run at `k0`; derive which direction, if
   any, can reduce measured serial stage time without violating memory.
3. Screen only `k0-2` and `k0+2` that pass allocation headroom. Expand once to
   `k0-4` or `k0+4` only if a stage trace predicts the direction and the first
   candidate improves the selected end-to-end metric by at least 2% in three
   interleaved preliminary pairs.
4. Compare the survivor with `1,1` using five interleaved pairs before a
   `[MEASURED]` claim, but do not route that comparison through the current
   harness: it has no `runtime_tensor_split` contract. Add and independently
   review a typed plan/schema/adapter that makes the split the sole condition
   difference before target execution.

For the 62-layer fixture, exact nearby maps are `28,35`, `30,33`, `32,31`,
`34,29`, and `36,27` total assignments; `1,1` resolves to 32 RPC repeating
layers and 31 local assignments including output. **[MEASURED]** In its
historical bounded screen,
[P11](p11-layer-placement-screen-and-expert-overlap-decision.md) found
the remote-heavy `1.1,0.9` MiniMax candidate adverse by 0.223% prompt and 0.287%
generation. [P14](p14-row-split-screen.md) found row split noise-scale/mixed
with generation adverse. Do not broaden beyond those prior bounded MiniMax
screens unless new per-split evidence or an engine change reopens them.

Record prompt tokens/s, client TTFT, generation tokens/s, p50/p95/p99 ITL,
request wall time, per-stage submit-to-completion time, boundary bytes and copy
direction, graph build/reuse, GPU busy/kernel time, peak HMM/GPU-active memory,
power/temperature, and exact output/token hash. A parity difference, rank or
feature counter miss, OOM/reset/fault, identity drift, or recovery failure ends
the experiment.

## Mode C: prompt and multi-sequence pipeline

**[VERIFIED]** The existing scheduler can use multiple graph copies only when every
accelerator reports async compute and events. Current RPC reports neither, so
this is a code candidate, not an available operating mode.

For `m` ready work units, an optimistic two-stage makespan is:

`T_m = C0 + X + C1 + (m - 1) * max(C0, X, C1)`

This can help only when `m >= 2` and work exists while the other stage runs:

- known prompt chunks whose absolute positions and KV commits stay ordered;
- independent decode sequences batched at an iteration boundary; or
- a mix of bounded prefill chunks and ready decode rows after ragged-position
  correctness is proven.

**[INFERENCE]** One ordinary autoregressive sequence supplies only one decode
work unit because the next token is unknown until suffix compute, sampling, and
output commit finish. Enabling RPC async/events cannot by itself turn the
historical roughly 30 ms remote plus 30 ms local phases into 30 ms per token.

The first implementation should negotiate an ordered completion/event
capability, retain feature-off behavior, use bounded copy rings and credits,
and fail closed on disconnect or ambiguous completion. Close or redesign it
before a broad matrix if three interleaved pairs show less than a clear 5%
directional prompt/TTFT gain, as required by the
[performance work plan](../../project/PERFORMANCE_WORKPLAN.md).

## Batch and ubatch

Keep the two knobs separate. **[INFERENCE]** For `P` prompt tokens, outer batch
`B`, and physical ubatch `U`:

- outer decode calls are approximately `ceil(P/B)`;
- physical graph work units are approximately `ceil(P/U)`.

The accepted plan-v2 contract in
[ADR-0056](decisions/0056-versioned-strix-ab-runtime-comparison.md) freezes the
`512/512` versus `2048/512` screen. It changes a 1,129-token prompt from three
outer calls to one while leaving three physical ubatches, with identical source
and role-matched complete artifacts across conditions and no other argv
difference. Any gain can therefore come from amortizing high-level
context/scheduler setup; it does not remove remote/local graph work units. Stop
expanding this screen if three interleaved pairs have no consistent direction
or less than 2% TTFT/prompt improvement. Keep `U` fixed until per-ubatch stage
and memory data identify a separate reason to change it. Its CachyOS adapter
remains hard-disabled while issue #41 says `REFUSE`.

## Authenticated scheduler-plan reuse

**[VERIFIED]** Ordinary RPC already retains one last graph per endpoint/device
and can submit a 13-byte recompute request. The optional authenticated composed lane is
different: `process_ubatch()` excludes it from ordinary graph reuse, so it
rebuilds/resplits/reallocates locally and sends a fresh authenticated graph.

For one RPC split, issue
[#58](https://github.com/JCFrags/HaloFPX/issues/58) records exact serialized
graph bytes:

`G = 12 + 8 * N_nodes + 296 * N_tensors`

Its Phase 1 retained scheduler plan can remove only local graph/split/allocation
work; it still sends the full authenticated graph and preserves the two
preparation/execute receipt boundaries. Only a later versioned prior-lineage
Phase 2 can omit `G`. Neither phase removes conditional output transfer.

Do not enable ordinary `can_reuse`, carry an old authority object into a fresh
attempt, or fuse preparation with execution. Retention must bind a fresh
attempt, socket/allocation epoch, backend/split/copy mapping, buffer ranges,
graph identity, and exact one-reusable-split cardinality or a bounded
multi-graph table. Require exact retained-plan/full/recompute counters and byte
accounting. Stop either phase when the intended counter never increments or
three matched pairs show less than 2% end-to-end gain; attribute graph-wire
byte savings only to Phase 2.

## KV and persistent-cache ownership

During ordinary layer-split inference, each layer's live KV is physically on
its owning device. The coordinator nevertheless owns the `llama_context` and
ordinary sequence-state API. [ADR-0039](decisions/0039-rpc-tensor-split-distributed-restore-blocker.md)
records a **[MEASURED]** small two-host fixture where monolithic sequence save
fetched remote KV through RPC and restore sent it back. It produced a
behaviorally complete coordinator blob for that exact fixture, but it is not
rank-local persistence.

For an attention-KV-only saved prefix of `P` tokens, define the worker-owned
state payload:

`B_worker(P) = sum(bytes(K_l[0:P]) + bytes(V_l[0:P]) + component_overhead_l)`

over worker-owned stateful layers `l`. A monolithic capture plus later restore
must move at least approximately `2 * B_worker(P)` payload bytes over the
fabric, excluding command framing and any duplicated validation traversal.

The worker-local protocol in
[ADR-0040](decisions/0040-worker-local-rpc-state-protocol-canary.md) keeps
worker objects under an operator-configured local cache root and the
coordinator-local object under its checkpoint-derived root; it sends only
bounded authenticated descriptors/readiness/commit records. Wiki section 58
recommends placing immutable rank-local objects on each rank's local NVMe; that
storage choice still needs machine validation. The intended hit-time model is:

`T_hit = T_lookup + max(T_read_rank0,T_read_rank1) + T_all_ready + T_apply + T_residual`

and the useful saving is:

`T_saved = T_cold_prefill - T_hit`

Any missing, corrupt, incompatible, stale, wrong-plan, or uncertain rank makes
the whole restore a miss and cold recomputation. A failure after possible live
apply requires fresh context recreation. Single-node fallback uses a
separately compatible world-1 checkpoint or recomputes; it never concatenates
or relabels world-2 objects. Issue
[#26](https://github.com/JCFrags/HaloFPX/issues/26) owns product wiring. Current
world-1 cache work is not evidence of a dual-rank cache hit.

## Generation candidates

Rank work must be removed, overlapped inside a token, or amortized across
verified tokens to materially improve one-sequence generation.

Current main contains separate default-off local-HIP Q/K/V activation-reuse
candidates for prompt MMQ and strict `n=1` generation MMVQ. The generation
candidate is governed by [ADR-0055](decisions/0055-rocmfpx-strict-n1-mmvq-qkv-q8-reuse.md)
and has exact GPU-less `gfx1151` composition compile/link evidence, not a target
runtime or speed result. RPC graph splits do not run its local HIP optimizer,
so qualify it on each single node first and require a separate protocol-aware
design before claiming dual-node reachability.

### MTP or other speculation

For one verification round, let `A` be accepted draft tokens,
`T_draft` draft cost, `T_verify(n)` target verification cost for `n` proposed
tokens, and `T_control` bookkeeping/copy cost. The effective cost is:

`T_spec_per_token = (T_draft + T_verify(n) + T_control) / E[A + 1]`

Compare that directly with plain decode `T_plain_per_token`. Use the historical
MTP profiles in the root README only as starting points for the exact models
measured there. Tune per model and workload, and include structured/predictable
plus free-form prompts.

Promotion requires exact greedy token parity, accepted/rejected draft counts,
verification latency, TTFT, ITL, and a positive matched end-to-end generation
gain. No accepted drafts, any parity failure, or no gain leaves speculation
off. Cache compatibility must include every draft/MTP state component or treat
the request as a cache miss.

### Intra-token distributed work

For MoE expert parallelism, a useful candidate must make:

`max(E_rank0,E_rank1) + T_dispatch + T_reduce + T_jitter < E_serial`

with deterministic ownership, routing, reduction order, and failure behavior.
**[MEASURED]** The historical P12 one-node synthetic MiniMax screen passed its
private lower-level `owned-rank < 60%` and `join < 5%` gates, but that is not an
end-to-end or model-general result. Require at least a 10% preliminary
end-to-end generation direction before broadening a new expert-parallel
prototype.

Dense tensor parallelism adds collectives at many layers. Screen it only after
measuring `max(C_rank0,C_rank1) + sum(T_collective)` against the layer-split
control; reject it when p99 ITL or end-to-end generation is not better. Keep
per-rank ROCmFPX kernel improvements ahead of new distributed protocols because
they help both replica and capacity modes without adding fabric failure points.

## USB4 transport policy

**[MEASURED]** In the historical target observation, the installed 7.1.3
CachyOS kernels exposed USB4 and `thunderbolt-net`, not USB4STREAM. In the
[2026-07-17 transport baseline](../../project/experiments/2026-07-17-usb4-transport-baseline/RESULTS.md),
one MPTCP connection with two observed subflows carried 20.687--20.714 Gb/s.
P08's instrumented request moved only tens of megabytes during its 20-second
monitor window and did not support aggregate-bandwidth saturation.

Keep TCP/MPTCP over `thunderbolt-net` as the baseline. Compare forced rail A,
forced rail B, and two-subflow MPTCP only after instrumentation attributes at
least 10% of request wall time to boundary transfer/socket wait, or sustained
payload reaches at least half the matched effective link rate. Stop if three
interleaved pairs show less than 2% end-to-end benefit. USB4STREAM remains a
separate reversible kernel/transport experiment after same-kernel correctness;
its existence in a newer kernel is not evidence of lower-copy or faster HaloFPX
execution.

## Ranked next A/B matrix

Every target cell remains blocked until issue #41 admits a production-free
maintenance window with a closed-world KFD/render/HMM census. Then run:

| Rank | Comparison | Why/stop gate |
|---:|---|---|
| 1 | Small pure-ROCmFPX fixture: nimo-1 single, nimo-2 single, two replicas at concurrency 2, current `RPC0,ROCm0` layer split at concurrency 1 and 2 | Establish the mode break-even. Do not call replication a latency win or layer mode a speed win without matched evidence. |
| 2 | `n_batch/n_ubatch` `512/512` vs `2048/512`, with one separately frozen plan and request for each 512/2048/8192 prompt bucket | Cheap scheduler-amortization screen; plan-v2 binds one request path/hash and prompt-token count, so never change buckets inside a plan. Stop under the batch gate above. |
| 3 | Resolver-verified `k0` and one trace-directed exact nearby layer cut | Stop absent at least 2% preliminary direction; do not repeat the closed MiniMax broad screen. |
| 4 | Isolated model-general ROCmFPX kernel candidates already tracked by #15/#25/#29/#42, single node first and dual rank only after its implementation proves reachability with a counter on every eligible rank | Feature reachability and parity precede timing; PR #53's strict `n=1` local-HIP candidate currently has no RPC reachability, and a microbenchmark-only win is not promoted. |
| 5 | Cold cache-off vs publication/fresh-process/rank-local hit, world 1 first and world 2 only after #26 product wiring | Require exact state/output parity, corruption/mismatch as miss, positive `T_saved`, and zero state-page payload on the world-2 control plane. |
| 6 | Plain decode vs exact-model MTP/speculation profiles | No accepted drafts or no end-to-end gain leaves it off. |
| 7 | Negotiated RPC async/event pipeline, prompt and concurrency buckets | Require a clear 5% prompt/TTFT direction; do not expect one-sequence decode overlap. |
| 8 | Authenticated retained scheduler plan Phase 1, then versioned lineage Phase 2 | Phase 1 saves local setup only; claim `G` wire-byte savings only in Phase 2 and stop under the counter/2% gate. |
| 9 | Rail A vs rail B vs MPTCP, then optional USB4STREAM prototype | Open only after transport attribution crosses the policy threshold. |
| 10 | Expert/tensor parallel prototypes | Open from a measured compute/collective cost model; require at least 10% preliminary end-to-end direction before broadening. |

Use one excluded warmup and three interleaved pairs for direction. Require five
interleaved pairs, exact identities, raw receipts, and intervals before a
`[MEASURED]` gain. The existing accepted plan-v2 harness encodes **only** row 2's
closed `runtime_n_batch` comparison. It cannot express replicas versus layer
split, tensor-split cuts, cache modes, speculation, async/events, scheduler-plan
reuse, rail policy, or expert/tensor parallelism. Each such row needs a
separately reviewed typed plan/schema/adapter that preserves the evidence method
and admits exactly one independent variable before target execution. MiniMax is
a final capacity/stress confirmation for survivors, not the exploratory matrix.

## Observability required to choose correctly

Current prompt profiling exposes host-wall graph build/reset, scheduler
dispatch/synchronization, ubatches, and graph build/reuse counts. Current main
also exports exact context-lifetime `output_epochs`, completed/reused barrier,
graph-submission, and output-transfer counters through the `--metrics`-enabled
`/metrics` endpoint, but they are not request-attributed and
`rpc_stats_available` remains false. Retain same-PID before/after snapshots of
those counters, then add the missing cumulative counters and per-work-unit
timestamps for:

- split ordinal, backend/rank/host, graph UID, node/tensor count, serialized
  graph bytes, full/recompute selection, and retained-plan invalidation reason;
- boundary `GET`/`SET` count, bytes, direction, host submit, first-byte, and
  completion time; distinguish GPU work, copy, socket wait, and scheduler wait;
- per-rank GPU kernel/busy time, peak GPUActive/HMM memory, OOM/reset/fault
  journals, power, temperature, and clocks;
- pipeline copy slot, event/credit occupancy, stage idle/busy time, bubble time,
  sequence count, and prompt/decode work-unit class;
- MPTCP subflow identity, per-path bytes/retransmits/failover and effective
  payload rate;
- cache source, selected/restored/avoided/residual tokens, rank-object bytes,
  lookup/read/hash/stage/apply/barrier time, and state-page wire bytes;
- startup model-source bytes, local file reads, RPC weight `SET`/hash hits,
  allocation time, and readiness time, reported separately from requests; and
- exact existing `halofpx_sampling_sync_*` snapshots plus missing request-window
  identity/timestamps, sampler calls, MTP proposals/acceptance, exact generated
  token IDs, and output hash.

These counters turn the next code choice into a measured decision. Scheduler
sync wall alone must not be labeled remote compute, and GPU utilization alone
must not be labeled a causal bottleneck.

## Failure and fallback contract

- Replica mode: fence the failed node/session epoch; keep the healthy replica
  available for new work and restore/replay only from state compatible with it.
- Layer/pipeline/intra-token distributed mode: loss or ambiguity on either rank
  ends the global attempt. Discard uncommitted output and partial state, advance
  the epoch, and restart/reconcile the complete plan.
- Restore only an exact two-rank checkpoint whose model, runtime/state ABI,
  topology, placement, ownership, token prefix, and component manifest match.
- Use single-node fallback only if the exact model/context safely fits. Restore
  separately compatible world-1 state when available; otherwise cold-recompute
  that exact model. If it does not fit, cold-run an approved smaller model or
  return a bounded retriable failure.
- Never continue a same-epoch request after a worker reconnect, accept partial
  KV, or infer recovery from `/health` alone. Exercise a real minimal two-rank
  request after worker-first/coordinator-second production recovery.
- PR #56's offline continuation tooling and same-PC receipt do not prove a
  fresh-PC recovery or authorize target work. Issue #2 remains open, and issue
  #41's current `REFUSE` decision remains the target-execution authority.

## Evidence routing

The detailed authorities remain:

- [performance work plan](../../project/PERFORMANCE_WORKPLAN.md)
- [P07 historical matched baseline](p07-current-head-feature-off-matched-baseline.md)
- [P08 exact-model critical-path profile](p08-exact-model-critical-path-profile.md)
- [P11 layer-placement screen](p11-layer-placement-screen-and-expert-overlap-decision.md)
- [P14 row-split rejection](p14-row-split-screen.md)
- [ADR-0055 strict n=1 local-HIP Q/K/V reuse](decisions/0055-rocmfpx-strict-n1-mmvq-qkv-q8-reuse.md)
- [ADR-0056 exact runtime outer-batch comparison](decisions/0056-versioned-strix-ab-runtime-comparison.md)
- [sampling-output synchronization counters](issue-28-sampling-output-sync-canary.md)
- [two-rank cache coordinator contract](two-rank-cache-coordinator-contract.md)
- [fresh-PC and target-machine boundary](../../project/TARGET_MACHINES.md)
- [Wiki section 43: contiguous layer pipeline](../../project/wiki/HaloFPX_Wiki/07_Distributed_Runtime/43_Contiguous_Layer_Pipeline_Parallelism_and_Microbatching/README.md)
- [Wiki section 58: rank-local restore](../../project/wiki/HaloFPX_Wiki/09_HaloKV_Persistent_Cache/58_Rank_Local_Ownership_and_Distributed_Restore_Coordination/README.md)
- GitHub issues [#15](https://github.com/JCFrags/HaloFPX/issues/15),
  [#26](https://github.com/JCFrags/HaloFPX/issues/26),
  [#41](https://github.com/JCFrags/HaloFPX/issues/41), and
  [#58](https://github.com/JCFrags/HaloFPX/issues/58)
