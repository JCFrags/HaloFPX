# Qualification checklists

Use only the sections needed for the current authorized phase. Resolve every placeholder into exact identities in an approved experiment card before running commands.

## Candidate build admission

- [ ] Pin canonical base, donor/reference commits, recursive gitlinks, tree IDs, dirty-state/patch digest, and license inventory.
- [ ] Record compiler, linker, CMake/generator, SDK/ROCm, HIP architecture, kernel headers, dependency versions, build options, and environment.
- [ ] Build in an isolated worktree/staging root; do not overwrite a deployed binary or library.
- [ ] Retain complete logs, warnings, test results, binary/library hashes, linked-library inventory, and build receipt.
- [ ] Confirm expected backend/server/RPC/quant/cache options from the produced binary, not from source assumptions.
- [ ] Run static/unit/conformance tests appropriate to the touched patch lane; retain failures.
- [ ] Map every donor capability to source/commit, license disposition, redesign/import/reject decision, tests, and rollback boundary.
- [ ] Define known-good artifact/configuration and exact restoration path before any target cutover.
- [ ] Mark compile-only results as such; do not claim target compatibility or deployment.

## Capacity and storage admission

- [ ] Record exact model/shard count, bytes, SHA-256, GGUF metadata, tokenizer/template, quant, architecture, and runtime pin.
- [ ] Capture per-node physical usable memory, GTT ceiling, `MemAvailable`, RSS/cgroup/GTT views, swap/zswap policy, PSI, and existing service envelope.
- [ ] Budget weights and non-layer tensors by realized placement, not equal-split arithmetic.
- [ ] Budget KV/recurrent/MTP/special state per sequence and context, graph/work buffers, staging/copy buffers, allocator fragmentation, runtime duplication, page cache, telemetry, and reserve.
- [ ] Capture filesystem, mount, snapshot/quota, model/cache/staging roots, current occupancy, and approved free-space floor.
- [ ] Calculate download + verification + conversion + temporary + deployed + rollback + RPC transfer cache + HaloKV peak bytes.
- [ ] Keep model, rollback, and sole evidence copies immutable.
- [ ] Use disposable roots for cache/fault tests; never fill or corrupt the workspace, model root, boot filesystem, or production cache.
- [ ] Stop before staging if either node lacks approved reserve. Treat nimo-1 as blocked until its current headroom is explicitly remediated.

## Identity and measurement admission

- [ ] Map durable node, rank, GPU, NVMe, USB4 controller/domain, physical port/cable, address, and interface identities.
- [ ] Capture BIOS/firmware, kernel/config/module hashes, boot parameters, ROCm/Mesa/packages, clocks, routes, IRQ/offload settings, and services.
- [ ] Record known asymmetries; do not erase them during normalization.
- [ ] Qualify monotonic clocks, cross-node offset/uncertainty, telemetry units/cadence, collector loss/overhead, schema validation, hashing, and derivation reproduction.
- [ ] Freeze workload, prompts, tokenizer, sampling, output policy, warmup, repetitions, order/seed, statistics, failure denominators, and stop rules.

## Matched single-node qualification

- [ ] Start with a small correctness oracle and minimal context/load; advance through preregistered steps only.
- [ ] Test both hosts independently with identical identities and settings; record host as an independent variable.
- [ ] Verify model metadata/tensor recognition, tokenizer/template, backend op coverage, explicit fallbacks, logits/tokens, deterministic state where applicable, and error handling.
- [ ] Record load/warmup, placement, RSS/cgroup/GTT/`MemAvailable`, swap/PSI, CPU/GPU use, power/thermal, TTFT, ITL, throughput, and failures.
- [ ] Verify cancellation, clean shutdown, no residual process/listener/test data, and restart from known-good state.
- [ ] Retain at least one correctness-passing, resource-bounded cell per viable host/backend; reject performance from any mismatched or fallback cell.

## Stable MPTCP/USB4NET qualification

- [ ] Preserve management LAN as out-of-band recovery and exclude it from bulk routes/MPTCP endpoints.
- [ ] Prove rail A/B mapping by private address and sysfs ancestry on both hosts.
- [ ] Freeze MTU, qdisc, congestion control, socket buffers, offloads, CPU/IRQ affinity, stream count, payloads, queue depth, direction, duration, warmup, repetitions, and contention.
- [ ] Prove traffic binding with route/socket state and per-interface counter deltas.
- [ ] Measure A only, B only, A+B same direction, A+B opposite directions, and simultaneous bidirectional operation.
- [ ] Record per-rail goodput/tails, MPTCP endpoints/subflows/bytes/fallback, retransmits/reorder/errors/drops, CPU/IRQ/softirq, memory pressure, power/thermal, and payload integrity.
- [ ] Compare adjacent randomized single-rail controls; do not infer independence or additivity from topology alone.
- [ ] Defer cable pulls, route mutation, and process/node faults to an explicitly authorized fault card.

## Distributed inference qualification

- [ ] Freeze coordinator/rank IDs; tensor/layer/expert/non-layer placement; KV/recurrent/MTP/cache ownership; sampler/RNG/grammar/output owner; plan hash; protocol/epoch; timeout/retry/cancel rules.
- [ ] Prove realized placement using logs, tensor/layer maps/hashes, and per-rank memory deltas.
- [ ] Begin cache-off to avoid persistence confounding; add cache only after its independent correctness/durability gate passes.
- [ ] Compare local/distributed correctness with the approved oracle across prompt/context/output bins and relevant architecture state.
- [ ] Include matched single-node and two-replica controls when admissible. Label unfit same-model cases as capacity extension, not speedup.
- [ ] Measure full client and engine boundaries separately; preserve all failed, rejected, cancelled, and timed-out requests.
- [ ] Verify explicit bounded behavior for rank/link/process failure, late completion, cancellation, restart/rejoin, and output commit.
- [ ] Qualify a smaller/reduced single-node fallback separately if selected; always start a new epoch.

## Optional USB4STREAM branch

- [ ] Confirm an approved need after stable MPTCP qualification; document the expected bottleneck and decision metric.
- [ ] Review exact upstream kernel/source/config/module/ABI; retain package/source/config/module hashes.
- [ ] Define separate-kernel installation, known-good boot entries, console/out-of-band access, one-node-first sequence, ROCm/storage/USB4NET smoke, cleanup, and rollback. Do not build kernels on inference nodes.
- [ ] Obtain explicit authorization before installing, rebooting, loading modules, creating ConfigFS streams, or changing interfaces.
- [ ] Preserve same-kernel USB4NET controls before enabling streams.
- [ ] Use a carrier-neutral framed codec with length/type/version/sequence/epoch/integrity, bounded credits/queues/timeouts, short-I/O state machine, peer identity/authentication, and cancellation.
- [ ] Begin with one stream and generated disposable payloads; verify byte count/digest, EOF/error behavior, device permissions, exact teardown, no residual ConfigFS/device state, and USB4NET smoke.
- [ ] Advance to dual streams only after the one-stream gate passes. Treat cables as full-duplex; directional preference must allow failover/capacity borrowing unless an approved design says otherwise.
- [ ] Compare TCP and USB4STREAM with identical payloads, queueing, affinity, directions, integrity, and endpoints; measure latency tails, goodput, CPU cycles/byte, syscalls, context switches, IRQs, copies/staging, power/thermal, and errors.
- [ ] Prove GPU-produced to peer-GPU-consumed integrity and synchronization. Do not infer GPU-direct, zero-copy, or lower latency from bypassed TCP layers alone.
- [ ] Reject on corruption, stale epoch, deadlock, unbounded memory, kernel/GPU/storage fault, cleanup/rollback failure, silent management-LAN fallback, or no approved matched benefit.

## Release/readiness checklist

- [ ] Every passed claim identifies its exact applicability envelope and immutable evidence.
- [ ] Both nodes build from documented inputs and map to artifact hashes.
- [ ] Baseline, cache-off, cache-on, corruption, crash/recovery, and single-node recovery paths pass where in scope.
- [ ] No unresolved security, storage, rollback, correctness, or provenance gate is waived by performance.
- [ ] Confirmatory results use predeclared counts/order/statistics and independent reproduction.
- [ ] An authorized decision record, not this skill or the experiment artifact, approves promotion.
