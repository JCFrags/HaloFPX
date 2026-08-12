---
title: Notation
status: normative model
---

# Notation

## Topology

| Symbol | Definition |
|---|---|
| \(A,B\) | Node A/rank 0 and node B/rank 1. |
| \(B_j\) | Measured effective one-direction payload bandwidth of physical path \(j\), bytes/s. |
| \(\ell_j\) | Measured/fitted one-way fixed message cost of path \(j\), seconds. |
| \(B_\Sigma\) | \(B_1+B_2\), permitted only after validated application-level striping. |
| \(V_j\) | Bytes assigned to path \(j\) by a striping policy. |
| \(m_{AR}\) | Message phases in the actual p=2 all-reduce. |

For one message with \(n\) fixed-cost phases:

\[
T_{msg}(V;n,B,\ell)=n\ell+\frac{V}{B}.
\]

For an explicitly striped message, the critical path is:

\[
T_{stripe}=\max_j\left(n_j\ell_j+\frac{V_j}{B_j}\right)+T_{reassembly}.
\]

The optimal ideal split with negligible reassembly is bandwidth-proportional, but the implementation must account for unequal fixed costs and minimum chunk sizes.

## Model

| Symbol | Definition |
|---|---|
| \(L\) | Number of transformer layers. |
| \(L_{MoE}\) | Number of MoE layers. |
| \(h\) | Residual hidden width. |
| \(f\) | Intermediate dense/MLP width, used for memory/compute accounting where needed. |
| \(H_q\) | Query attention heads. |
| \(H_{kv}\) | Key/value heads. |
| \(d\) | KV head dimension. Use the explicit model field when available. |
| \(|\mathcal V|\) | Vocabulary size. |
| \(P\) | Parameter count used for an ideal packed-weight lower bound. |
| \(E\) | Experts per MoE layer. |
| \(k\) | Experts selected per token. |

Qwen3-30B-A3B exposes an explicit head dimension that is not equal to \(h/H_q\); the cost model uses the explicit head dimension for KV bytes and \(h\) for boundary activations.

## Workload

| Symbol | Definition |
|---|---|
| \(N\) | Total prefill tokens in the modeled forward pass or chunk. |
| \(Q\) | Active sequences in a decode scheduling step. |
| \(c_i\) | Current context tokens for sequence \(i\). |
| \(M\) | Pipeline microbatches. |
| \(\gamma\) | Speculative draft length. |
| \(a\) | Simplified constant independent candidate acceptance probability. |
| \(R_l\) | Remote expert assignments at MoE layer \(l\). |
| \(\rho_l\) | Remote fraction of expert assignments at layer \(l\). |

## Element widths

| Symbol | Definition |
|---|---|
| \(b_a\) | Bytes per transmitted hidden activation element. |
| \(b_{kv}\) | Bytes per KV element. |
| \(b_t\) | Bytes per transmitted token ID. |
| \(b_p\) | Bytes per transmitted logit/probability element. |
| \(b_{meta}\) | Per-routed-assignment metadata bytes in expert service. |

## Compute and memory

| Symbol | Definition |
|---|---|
| \(C_1\) | Measured one-node compute time for the exact modeled forward unit. |
| \(\eta_{TP}\) | Measured TP compute efficiency relative to ideal 2-way compute: sharded compute \(C_1/(2\eta_{TP})\). |
| \(c_A,c_B\) | Measured stage compute time per pipeline microbatch. |
| \(x\) | Measured or modeled transfer-stage time per microbatch. |
| \(W_r\) | Weight residency on rank \(r\). |
| \(K_r\) | KV residency on rank \(r\). |
| \(R_r\) | Runtime/workspace/allocator residency on rank \(r\). |
| \(M_{usable,r}\) | Measured memory budget available to inference on rank \(r\). |

The hard memory gate is:

\[
W_r+K_r+R_r+M_{OS,r}\le M_{physical,r},
\]

or, equivalently, \(W_r+K_r+R_r\le M_{usable,r}\) when the usable budget is measured directly.
