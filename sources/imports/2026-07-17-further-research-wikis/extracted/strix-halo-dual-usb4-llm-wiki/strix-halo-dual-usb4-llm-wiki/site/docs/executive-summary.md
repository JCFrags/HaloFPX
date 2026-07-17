---
title: Executive summary
status: decision model
---

# Executive summary

## Platform boundary

**SOURCED FACT.** Ryzen AI Max+ 395 is AMD's Strix Halo part with two native USB4 40 Gb/s ports and up to 128 GB of LPDDR5x system memory. The memory is local to each system; two machines do not become one coherent 256 GB address space. [AMD specification](https://www.amd.com/en/products/processors/laptop/ryzen/ai-300-series/amd-ryzen-ai-max-plus-395.html).

**DECISION RULE.** Treat the interconnect as two independently measured paths:

\[
(B_1,\ell_1),\quad(B_2,\ell_2).
\]

Use \(B_\Sigma=B_1+B_2\) only after an application-level concurrent-transfer test shows scaling at the relevant message sizes. Otherwise, calculate the mode against one selected path or session-hash flows across paths.

## Mode assessment

| Mode | Prefill communication | Decode communication | Synchronization pressure | Best justification | Initial disposition |
|---|---:|---:|---:|---|---|
| Replicated decode | 0 model-path bytes | 0 model-path bytes | None between replicas | Concurrent independent requests | **GO candidate** if full model fits each node |
| Contiguous layer split | \(Nhb_a\) | \(Qhb_a\) + token IDs | One bulk boundary; one decode feedback | Capacity | **GO candidate** after memory/runtime gates |
| Pipeline over layer split | Same total volume, split into \(M\) messages | Same total volume across active sequences | Per-microbatch; per-sequence feedback | Aggregate throughput | **Conditional** on concurrency and stage balance |
| MoE layer-local split | Same as contiguous split | Same as contiguous split | Same as contiguous split | MoE capacity | **Preferred MoE placement** |
| MoE expert service | \(\sum_l R_l(2hb_a+b_m)\) | Same with decode-token assignments | Dispatch + return at every active MoE layer | Expert capacity or measured routing locality | **Experimental** |
| Remote speculation | Candidate IDs; protocol-dependent probabilities | One request/response per speculative round | Round-trip barrier | Single-request decode | **Conditional** on measured acceptance/timing |
| Tensor parallel 2-way | Per rank \(2LNhb_a\) | Per rank \(2LQhb_a\) | \(2L\) collectives; runtime-dependent phases | Per-layer capacity | **Fail closed** until strict break-even passes |
| KV migration / prefill-decode disaggregation | Full prompt KV transfer | Transition only | Transactional handoff | Phase specialization | **No-go without new evidence** |

Here \(N\) is the number of prefill tokens in the forward pass, \(Q\) is the active decode batch, \(h\) is hidden width, \(L\) is layer count, and \(b_a\) is bytes per transmitted activation element.

## Why tensor parallelism is the hardest fit

Megatron-style intra-layer parallelism uses two all-reduces in each transformer layer's forward path. [Megatron-LM paper](https://arxiv.org/abs/1909.08053). For two ranks, each collective makes every rank send approximately one hidden-state tensor; the collective's message-phase count depends on the runtime algorithm.

For a forward pass with tensor size \(S\):

\[
V_{\mathrm{TP,rank}}=2LS,\qquad
T_{\mathrm{TP,comm}}=2L\left(m_{AR}\ell+\frac{S}{B}\right).
\]

The high collective count makes decode especially latency-sensitive even when the per-collective tensor is small. TP remains a capacity mechanism when no whole layer partition fits, but capacity feasibility is not evidence of speedup.

## Why a single contiguous cut is the capacity baseline

Place embeddings and layers \([0,k)\) on rank 0 and layers \([k,L)\), final norm, LM head, and sampler on rank 1. The forward path crosses USB4 once:

\[
V_{\mathrm{cut,prefill}}=Nhb_a,\qquad
V_{\mathrm{cut,decode}}=Qhb_a.
\]

Each rank owns KV only for its local layers. No KV is moved during steady-state inference. Rank 1 returns selected token IDs to rank 0 after each decode step. This layout minimizes the number of inter-node dependencies without duplicating the full model.

It does not, by itself, make one request faster: a single request still executes both stage computes serially plus the boundary transfer. Its default justification is capacity. Pipeline scheduling can use independent requests to overlap stages.

## MoE rule: divide by layer before dividing experts

Expert parallelism conventionally dispatches token representations to expert owners and returns expert outputs, creating all-to-all-style communication. [DeepSpeed-MoE](https://proceedings.mlr.press/v162/rajbhandari22a.html). Across a two-node cut, expected traffic at MoE layer \(l\) is:

\[
R_l=N_l k\rho_l,
\qquad
V_l=R_l\left(2hb_a+b_{meta}\right),
\]

where \(k\) experts are selected per token and \(\rho_l\) is the measured fraction of assignments sent to the remote node.

Because this round trip repeats at every MoE layer, the default is to keep all experts attached to a layer on that layer's owner. Independent expert placement is a capacity fallback requiring routing traces, not a default optimization.

## Remote speculation can be network-light, but not assumption-light

Speculative decoding can preserve target-model output exactly while a smaller model proposes several tokens and the target verifies them in parallel. [Leviathan et al.](https://proceedings.mlr.press/v202/leviathan23a.html).

For synchronous remote speculation with draft length \(\gamma\):

\[
T_{round}=T_d(\gamma)+T_v(\gamma)+2\ell+\frac{V_{round}}{B}.
\]

Under the explicit constant-independent acceptance approximation \(a\), expected committed tokens are:

\[
E[K]=1+a+a^2+\cdots+a^\gamma.
\]

Break-even against measured target one-token time \(T_{t,1}\) requires:

\[
T_{round}<E[K]T_{t,1}.
\]

Greedy verification can send token IDs plus small metadata. Standard exact stochastic speculative sampling also needs sufficient proposal-distribution information for acceptance and residual sampling; token IDs alone are not assumed sufficient. The exact network volume is therefore protocol-specific.

## Hard stop conditions

A mode is an immediate NO-GO for the stated objective when any of the following holds:

- the local weight + KV + workspace budget does not fit;
- the runtime cannot express the ownership or collective;
- dual-link aggregation is assumed rather than measured;
- the communication inequality has a non-positive payload budget even at infinite bandwidth;
- one-sequence pipeline speedup is claimed without independent work;
- stochastic remote speculation lacks an exact probability/RNG protocol;
- a full-logit all-gather is silently omitted from a sharded sampler;
- expert placement uses an unmeasured remote fraction;
- KV migration lacks compatible export/import and transactional ownership.

## Recommended sequence

1. Establish a one-node baseline for every model, context, batch, quantization, and runtime.
2. Measure each USB4 path independently, then simultaneously, over the mode's actual message sizes.
3. Test replicated decode for models that fit.
4. For capacity, test a single contiguous layer cut and preserve local KV ownership.
5. Add pipeline scheduling only after concurrency and stage-service gates pass.
6. Test remote speculation only with measured drafter, verifier, acceptance, and exactness data.
7. Test expert service or TP only when the simpler placements cannot satisfy capacity or their strict break-even equations pass.
