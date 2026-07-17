---
title: Break-even methods
status: decision rules
---

# Break-even methods

Break-even analysis separates three cases:

1. **speed break-even** — distributed execution is faster than the measured one-node baseline;
2. **throughput break-even** — system completion rate improves under the same latency/SLO constraints;
3. **capacity feasibility** — the model or context runs only when partitioned, even if slower.

Do not use a speed inequality to reject a capacity-only mode, and do not label capacity feasibility as speedup.

## Tensor parallel speed gate

Let measured one-node compute for the exact forward unit be \(C_1\). Model two-way sharded compute as:

\[
C_{TP,compute}=\frac{C_1}{2\eta_{TP}},
\]

where \(\eta_{TP}\) is measured. With \(S=Nhb_a\) for prefill or \(S=Qhb_a\) for decode:

\[
\frac{C_1}{2\eta_{TP}}+2L\left(m_{AR}\ell+\frac{S}{B}\right)<C_1.
\]

Equivalent communication budget:

\[
2L\left(m_{AR}\ell+\frac{S}{B}\right)
<C_1\left(1-\frac{1}{2\eta_{TP}}\right).
\]

The right-hand side is non-positive when \(\eta_{TP}\le0.5\); speedup is then impossible even with zero communication.

Required effective payload bandwidth:

\[
B_{req}=\frac{2LS}
{C_1(1-1/(2\eta_{TP}))-2Lm_{AR}\ell},
\]

provided the denominator is positive. A non-positive denominator is a hard NO-GO for speed at the measured latency and compute efficiency.

## Contiguous layer split speed gate

For one request:

\[
T_{split}=C_A+C_B+T_{boundary}+T_{feedback}.
\]

Compare against the same-runtime one-node time \(C_1\):

\[
C_A+C_B+T_{boundary}+T_{feedback}<C_1.
\]

There is no theoretical two-way compute division in this layout; the layer computes remain serial. Any speed benefit must come from measured effects such as memory residency, reduced paging, different kernel behavior, or quantization choices. Otherwise the mode is capacity-driven.

Memory gate for cut \(k\):

\[
W_{[0,k)}+K_{[0,k)}+R_A\le M_{usable,A},
\]

\[
W_{[k,L)}+K_{[k,L)}+R_B\le M_{usable,B}.
\]

## Pipeline throughput gate

With \(M\) microbatches and service interval \(s\):

\[
T_{pipe}=c_A+x+c_B+(M-1)s.
\]

A serial two-stage compute baseline is:

\[
T_{serial}=M(c_A+c_B).
\]

Pipeline benefit requires:

\[
s<c_A+c_B
\]

and:

\[
M>\frac{c_A+x+c_B-s}{c_A+c_B-s}.
\]

Use strict integer rounding. `tools/cost_model.py` returns the minimum \(M\). Also enforce queue-memory and tail-latency limits.

## MoE expert-service gate

For layer \(l\), let \(\Delta C_l\) be measured critical-path work saved by placing some experts remotely. Let \(I_l\) be measured routing/load imbalance and \(C_{remote,l}\) remote expert compute on the critical path.

\[
2\ell+\frac{R_l(2hb_a+b_{meta})}{B}+C_{remote,l}+I_l<\Delta C_l.
\]

Summing byte volumes is useful for link capacity; speed must be evaluated on the per-layer critical path because communication and expert work can overlap differently by implementation.

Capacity gate:

\[
W_{dense,A}+W_{experts,A}+K_A+R_A\le M_{usable,A},
\]

\[
W_{experts,B}+R_B\le M_{usable,B}
\]

for the remote-expert-service placement. A layer-local split uses the corresponding contiguous layer budgets instead.

## Remote speculation gate

For draft length \(\gamma\), empirical expected committed tokens \(E[K]\), measured draft time \(T_d\), target block verification time \(T_v\), round bytes \(V\), and target one-token time \(T_{t,1}\):

\[
T_d+T_v+2\ell+\frac{V}{B}<E[K]T_{t,1}.
\]

The network budget before payload is:

\[
T_{net,budget}=E[K]T_{t,1}-T_d-T_v.
\]

The latency-only gate is:

\[
T_{net,budget}>2\ell.
\]

Required bandwidth:

\[
B_{req}=\frac{V}{T_{net,budget}-2\ell}.
\]

Use measured acceptance by prompt class and decode position. Optimize \(\gamma\) by evaluating the entire empirical distribution; longer drafts reduce round frequency but can lower acceptance and increase wasted verifier/drafter work.

## Replicated decode gate

Memory:

\[
W_{full}+K_{planned,r}+R_r\le M_{usable,r}\quad\text{for }r\in\{A,B\}.
\]

Concurrency:

\[
\text{independent runnable sessions}\ge2
\]

is a practical minimum to use both replicas for aggregate throughput. Compare measured admission-to-completion and per-token distributions under the same offered load and SLO. No formula grants single-request latency speedup.

## KV migration gate

\[
T_{export}+\ell+\frac{cK_{token}}{B}+T_{import}
<G\Delta T_{future}.
\]

The left side includes one-time handoff; the right side is measured remaining benefit. Fail if KV formats differ, ownership cannot be transactional, or memory requires deleting the source before validating the destination.

## Nominal-floor triage

Before benchmarking a mode, calculate a protocol-free payload floor using 5 GB/s per nominal 40 Gb/s link. This is useful only for rejection:

- if the payload-only floor already exceeds the entire measured compute benefit, NO-GO;
- if it does not, proceed to real link/runtime measurement;
- never mark GO from the floor.
