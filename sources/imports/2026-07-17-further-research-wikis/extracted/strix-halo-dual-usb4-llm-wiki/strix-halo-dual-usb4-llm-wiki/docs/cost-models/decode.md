---
title: Decode communication and synchronization
status: symbolic model
---

# Decode communication and synchronization

Let \(Q\) be the number of active sequences processed in one decode scheduling step. Each contributes one current token and attends to its own KV cache.

## Tensor parallelism, TP=2

Per collective hidden tensor:

\[
S_{dec}=Qhb_a.
\]

Per step:

\[
V_{rank,sent}=2LQhb_a,
\qquad
V_{cut,bidirectional}=4LQhb_a,
\]

\[
T_{TP,dec}=2L\left(m_{AR}\ell+\frac{Qhb_a}{B}\right).
\]

Synchronization points: \(2L\) all-reduces before logits, followed by the sampler protocol. Small \(Q\) makes the payload term smaller but leaves the collective latency count unchanged.

### Vocabulary sampling

If the LM head is vocabulary-sharded, exact sampling must be modeled explicitly.

- **Greedy.** Each rank computes its local maximum logit and token ID; a small global max reduction selects the winner.
- **Stochastic.** A correct distributed protocol must compute global normalization and selection under the configured logits processing, or otherwise gather sufficient logits/probabilities.
- **Full-logit fallback.** Gathering all logits adds approximately \(Q|\mathcal V|b_p\) bytes per step and can dominate small hidden-state collectives for large vocabularies.

The placement defaults to distributed sampling and treats full-logit gathering as an explicit fallback, never a hidden omission.

## Contiguous layer split

Rank 0 sends one layer-boundary tensor to rank 1:

\[
V_{A\rightarrow B}=Qhb_a.
\]

Rank 1 samples and returns token IDs:

\[
V_{B\rightarrow A}=Qb_t+b_{control}.
\]

Communication time without overlap:

\[
T_{cut,dec}=\ell_{bulk}+\frac{Qhb_a}{B_{bulk}}+
\ell_{control}+\frac{Qb_t+b_{control}}{B_{control}}.
\]

Synchronization points: one hidden-state boundary and one autoregressive feedback response per scheduling step. KV stays local by layer.

For one sequence, compute remains serial:

\[
T_{token}=C_A+T_{A\to B}+C_B+T_{B\to A}.
\]

A layer split therefore needs measured locality/runtime gains to beat a one-node latency baseline; its default value is capacity.

## Pipeline scheduling during decode

The next token of sequence \(i\) cannot begin stage A until the sampler has finalized its previous token. Pipeline utilization comes from other sequences:

\[
\text{sequence }i,t \to A \to link \to B \to sampler \to \text{sequence }i,t+1.
\]

With \(M\) independent microbatches of sequences, the forward service interval uses the same pipeline equations as prefill, plus token feedback and scheduling. Report:

- aggregate tokens completed per scheduling interval;
- per-sequence inter-token latency distribution;
- pipeline occupancy and bubble fraction;
- queueing delay.

Do not report only aggregate throughput when tail latency is an objective.

## MoE layer-local placement

Decode network volume remains the one-cut formula:

\[
V_{MoE,local,dec}=Qhb_a+Qb_t+b_{control}.
\]

MoE routing and expert work remain local to each layer owner.

## MoE expert service

At layer \(l\):

\[
R_l=Qk\rho_l,
\]

\[
V_{l,dec}=R_l(2hb_a+b_{meta}).
\]

Across \(L_{MoE}\) layers:

\[
V_{EP,dec}=\sum_l Qk\rho_l(2hb_a+b_{meta}).
\]

Even at \(Q=1\), top-\(k\) routing can create multiple remote assignments per layer. Fixed round-trip cost repeats at every active MoE layer. This is why decode is the strictest expert-service gate.

Critical-path speed break-even for layer \(l\) requires the measured reduction in local expert critical-path work, \(\Delta C_l\), to exceed communication, remote execution, and imbalance:

\[
2\ell+\frac{V_{l,dec}}{B}+T_{remote,l}+T_{combine,l}+T_{imbalance,l}<\Delta C_l.
\]

For capacity-only deployments, failure of this speed inequality is acceptable only when latency/throughput objectives explicitly tolerate the cost.

## Remote speculation

### Greedy / deterministic verification

Rank 0 drafts \(\gamma\) token IDs; rank 1 verifies them with the target and returns accepted length plus a correction/next token.

\[
V_{req}=\gamma b_t+b_{meta,req},
\]

\[
V_{resp}=b_t+b_{meta,resp}.
\]

Synchronous round time:

\[
T_{round}=T_d(\gamma)+\ell+\frac{V_{req}}{B}+T_v(\gamma)+
\ell+\frac{V_{resp}}{B}.
\]

### Exact stochastic verification

Standard lossless speculative sampling uses the draft distribution \(q\) in acceptance and residual sampling. A remote protocol must either:

- transmit enough \(q\) information;
- replicate/recompute the draft distribution on the target rank; or
- use another proven exact protocol.

The network template is:

\[
V_{req}=\gamma b_t+V_q+b_{meta}.
\]

An eager full-vector protocol has:

\[
V_q=\gamma|\mathcal V|b_p,
\]

which can materially alter feasibility. On-demand/compressed protocols must state exactness, expected volume, and extra rounds.

### Expected progress and break-even

Under the explicit constant-independent acceptance approximation \(a\):

\[
E[K]=\sum_{i=0}^{\gamma}a^i=
\begin{cases}
\frac{1-a^{\gamma+1}}{1-a},&a\ne1,\\
\gamma+1,&a=1.
\end{cases}
\]

Break-even against measured target one-token time \(T_{t,1}\):

\[
T_{round}<E[K]T_{t,1}.
\]

This is evaluated over the empirical acceptance distribution in production analysis; the constant-\(a\) expression is a sensitivity model.

## Replicated decode

Each replica performs all model computation, sampling, and KV access locally for its assigned sessions:

\[
V_{replica,dec}=0
\]

on the cross-node model path. There is no synchronization between replicas. The mode improves system capacity only when there are independent requests; it cannot accelerate one sequence by adding the second idle replica.

## KV migration during decode

For context \(c\):

\[
V_{move}=2cLH_{kv}db_{kv}.
\]

A migration pauses or duplicates session state and adds export/import. It is not part of normal replicated decode and should not occur on routine load balancing without a future-work amortization gate.
