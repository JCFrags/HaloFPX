---
title: Prefill communication and synchronization
status: symbolic model
---

# Prefill communication and synchronization

Let \(N=\sum_i s_i\) be the total prompt tokens processed in the modeled forward pass or chunk. Let the transmitted hidden activation use \(b_a\) bytes per element.

## Tensor parallelism, TP=2

Megatron-style tensor parallelism performs two forward all-reduces per transformer layer. [Megatron-LM](https://arxiv.org/abs/1909.08053).

Per collective:

\[
S_{pf}=Nhb_a.
\]

Counts and volumes:

\[
n_{collective}=2L,
\]

\[
V_{rank,sent}=2LNhb_a,
\qquad
V_{rank,recv}=2LNhb_a,
\]

\[
V_{cut,bidirectional}=4LNhb_a.
\]

Communication critical path under the fitted p=2 model:

\[
T_{TP,pf}=2L\left(m_{AR}\ell+\frac{Nhb_a}{B}\right).
\]

Synchronization points: \(2L\) collective dependencies. Transport phases: \(2Lm_{AR}\).

Chunking a prompt changes \(N\) per forward and repeats all \(2L\) collectives for every chunk. Total payload across all chunks remains proportional to total tokens, while fixed latency grows with chunk count.

## Contiguous layer split

One activation boundary:

\[
V_{cut,pf}=Nhb_a.
\]

For \(M\) prompt chunks/microbatches with \(N_j\) tokens:

\[
\sum_j V_j=Nhb_a,
\qquad
T_{cut,pf}=M\ell+\frac{Nhb_a}{B}
\]

before overlap and runtime terms.

Synchronization points: one boundary dependency per chunk. No token-ID feedback is needed until the final prefill output is sampled or decode begins.

## Pipeline parallelism

Ownership and total communication are the same as a contiguous split, but the prompt/batch is divided into \(M\) independent microbatches. Let measured per-microbatch stage times be \(c_A,c_B\), and transfer-stage time be:

\[
x=\ell+\frac{N_{mb}hb_a}{B}.
\]

When stage A, the link, and stage B are independently overlappable resources, the ideal forward makespan is:

\[
T_{pipe}=c_A+x+c_B+(M-1)\max(c_A,x,c_B).
\]

If transport consumes a resource that prevents compute overlap, use the conservative service interval:

\[
s=\max(c_A,c_B)+x,
\]

and:

\[
T_{pipe}=c_A+x+c_B+(M-1)s.
\]

Synchronization points: \(M\) boundary messages, plus queue/backpressure dependencies. The pipeline improves aggregate makespan only when the measured service interval is below serial per-microbatch compute \(c_A+c_B\) and \(M\) exceeds the break-even threshold.

## MoE layer-local placement

When all experts stay with their layer owner, prefill network cost is identical to the contiguous layer cut:

\[
V_{MoE,local,pf}=Nhb_a.
\]

Router and expert operations are node-local. The layer cut should account for expert-weight imbalance.

## MoE expert service

At MoE layer \(l\), each token has \(k\) expert assignments. If fraction \(\rho_l\) crosses the node cut:

\[
R_l=N_l k\rho_l.
\]

A baseline dispatch/return protocol sends one hidden vector to the remote expert and one output vector back:

\[
V_{l,pf}=R_l(2hb_a+b_{meta}).
\]

Across all MoE layers:

\[
V_{EP,pf}=\sum_{l\in MoE}N_lk\rho_l(2hb_a+b_{meta}).
\]

Synchronization: at least a dispatch dependency and a return/combine dependency for each layer with remote assignments. In a dense prompt batch, remote assignments will often be present at most or all MoE layers, but this must be observed, not assumed.

A conservative no-overlap time is:

\[
T_{EP,pf}=\sum_l \mathbf 1[R_l>0]
\left(2\ell+\frac{V_{l,pf}}{B}+T_{remote,l}+T_{combine,l}\right).
\]

An overlap-capable implementation should use the measured layer critical path rather than simply subtracting communication.

## Remote speculation

Remote speculation is primarily a decode technique. Both draft and target still need initial state. Common prefill options are:

1. prefill draft on rank 0 and target independently on rank 1 from the same token IDs;
2. send canonical prompt token IDs from tokenizer owner to the other rank;
3. avoid KV transfer between draft and target because their architectures/KV layouts generally differ.

Prompt token transport is:

\[
V_{prompt\_ids}=Nb_t,
\]

plus request metadata. The dominant prefill compute remains local to each model; there is no cross-model KV sharing assumption.

## Replicated decode

For each session, prefill occurs entirely on its assigned replica. Model-path cross-node communication is zero:

\[
V_{replica,pf}=0.
\]

Request ingress can be prompt text or \(Nb_t\) token IDs if a central frontend tokenizes. There is no inter-replica synchronization.

## Prefill decision signal

The ratio between TP and one-cut payload per rank is:

\[
\frac{V_{TP,rank}}{V_{cut}}=2L.
\]

This ratio is independent of \(N,h,b_a\). It does not by itself decide speed, because TP can reduce compute and a layer cut is serial for one microbatch, but it explains why a slow inter-node fabric strongly favors minimizing boundary count when capacity permits.
