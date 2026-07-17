---
title: Feasibility gates
status: hard-gate checklist
---

# Feasibility gates

A mode is not viable because its formula is attractive. It is viable only after every relevant hard gate passes on the target systems.

## Gate 0 — objective is explicit

Choose one primary objective for each test:

- fit a model/context that cannot run on one node;
- reduce single-request time to first token;
- reduce single-request inter-token latency;
- increase aggregate throughput at a fixed SLO;
- increase concurrent context capacity;
- improve isolation or failover.

A result for one objective cannot be silently promoted to another.

## Gate 1 — physical topology

**Required evidence:** inventory from both systems.

- Both intended USB4 ports are exposed by the OEM and negotiate the expected generation.
- Each cable is certified for the intended rate and length.
- Each host-to-host connection enumerates independently.
- Port-to-controller/router topology is recorded.
- No display, dock, storage, or other tunnel shares the test paths.
- Power/firmware/security settings permit stable interdomain operation.

**NO-GO:** one of the two paths does not enumerate reliably, or the intended OS cannot establish host-to-host communication.

## Gate 2 — per-link transport characterization

Measure each path in both directions and full duplex over the relevant message-size sweep. Record:

\[
B_{j,small},\ell_{j,small},B_{j,bulk},\ell_{j,bulk}.
\]

Use the actual application transport where possible. A TCP benchmark is an initial network check, not a substitute for collective/RPC traces.

**NO-GO:** error/retry rate is material, latency has unbounded tails for the SLO, or payload performance is unstable under sustained execution.

## Gate 3 — simultaneous-link scaling

Run path 1 alone, path 2 alone, then both concurrently with independent flows and with the planned striping implementation.

Define scaling efficiency:

\[
\eta_{link}=\frac{B_{both}}{B_1+B_2}.
\]

Set a project threshold before testing. `B_sum` is allowed only when scaling and tail behavior pass at the mode's message sizes.

**NO-GO for aggregation:** no material gain, severe tail inflation, reordering/backpressure failure, or shared-resource saturation. Single-link and session-hash modes may remain viable.

## Gate 4 — runtime primitive

Verify the exact backend supports:

- model loading and target quantization on both Strix Halo systems;
- RPC or point-to-point activation transfer;
- collective operations for TP, including traced p=2 algorithm;
- stable tensor serialization/layout;
- KV ownership and rollback;
- distributed or local sampler semantics;
- expert routing where used;
- error propagation and cancellation.

AMD's published llama.cpp RPC cluster is evidence that an RPC path exists on Ryzen AI Max+ systems, not evidence that every listed mode is implemented. [AMD technical article](https://www.amd.com/en/developer/resources/technical-articles/2026/how-to-run-a-one-trillion-parameter-llm-locally-an-amd.html).

**NO-GO:** the mode requires a primitive absent from the chosen runtime and no implementation plan is in scope.

## Gate 5 — memory residency

For each rank, measure/estimate:

- actual loaded weight allocations;
- quantization metadata and padding;
- model graph/workspace;
- boundary/collective/expert buffers;
- target maximum KV by session mix;
- allocator fragmentation and safety reserve;
- OS and co-resident service memory.

Require:

\[
W_r+K_r+A_r+R_r\le M_{usable,r}-M_{safety}.
\]

Test at maximum planned context and concurrency, not only model load.

**NO-GO:** out-of-memory, paging/thrashing outside the declared design, or insufficient safety margin.

## Gate 6 — correctness and ownership

Validate:

- identical model/checkpoint revision where required;
- exact tokenizer and special-token IDs;
- boundary tensor dtype/shape/position semantics;
- one owner for every KV shard, sampler, RNG stream, and session state;
- deterministic greedy equivalence;
- stochastic seeded replay/distribution tests;
- MoE routing and combine correctness;
- speculative rollback at every rejection position;
- no external emission of unverified tokens.

**NO-GO:** any mismatch, duplicate KV append, missing expert contribution, sampler divergence, or ambiguous state ownership.

## Gate 7 — communication break-even

Populate the mode equation with measured values.

### TP

\[
2L(m_{AR}\ell+S/B)
<C_1(1-1/(2\eta_{TP})).
\]

### Pipeline

\[
s<c_A+c_B,
\qquad
M>M_{min}.
\]

### Expert service

\[
2\ell+V_l/B+C_{remote,l}+I_l<\Delta C_l.
\]

### Remote speculation

\[
T_d+T_v+2\ell+V/B<E[K]T_{t,1}.
\]

### KV migration

\[
T_{export}+\ell+V_{KV}/B+T_{import}<G\Delta T.
\]

**NO-GO for speed:** a denominator is non-positive or measured communication exceeds the available compute benefit. Capacity-only modes may continue with the slower result explicitly accepted.

## Gate 8 — synchronization and tail behavior

Measure timeline traces, not only average bytes. Required metrics:

- fixed-cost phase count;
- collective/barrier wait by rank;
- stage bubbles and queueing;
- head-of-line blocking between control and bulk traffic;
- p50/p95/p99 time to first token and inter-token latency;
- cancellation response;
- recovery after link or rank failure.

**NO-GO:** tails or stalls violate the objective even when average throughput improves.

## Gate 9 — thermal and sustained-state stability

Run long enough to reach steady power/temperature behavior. Record firmware, power profile, clocks where available, and ambient conditions.

**NO-GO:** results depend on short boost windows, sustained throttling invalidates the break-even, or simultaneous network/compute causes instability.

## Gate 10 — reproducible comparison

The baseline and candidate must match:

- checkpoint and quantization;
- context and prompt tokens;
- active sequence mix;
- sampler configuration and output limit;
- warm/cold state;
- runtime commit and build flags;
- memory reserve and power mode.

Retain raw logs and calculated inputs. A GO without a reproducible baseline is invalid.
