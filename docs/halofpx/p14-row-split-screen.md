# P14 row-split configuration screen

Status: **rejected; balanced layer split remains the control**

P14 tests one configuration-only alternative after P13 closed the low-leverage
custom expert-integration lane. The reusable primary-block harness now accepts
and records an optional `layer|row` split mode while preserving `layer` as its
default. This changes no engine source or deployment default.

## Fixed workload and bounded method

The source parent is HaloFPX commit
`ea49690a2b80d2a6c366c8fdc7c306ab41c3f226`, tree
`64315ed51d140941b730994f53223389ffdff5dc`. The candidate used the same P09
feature-off binaries, nimo-1 RPC worker and nimo-2 coordinator, `RPC0,ROCm0`,
MPTCP, `1,1` tensor split, Q8_0 K/V, context 4096, batch/ubatch 512, seed 1234,
temperature zero, and qualified request as the P11 balanced-layer control.

The pinned model is
`rcmorano/saricles-MiniMax-M2.7-REAP-172B-A10B-ROCMFPX` revision
`dba517197f2854f3d362529e13abddcdcad6c10b`, file
`saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf`, size
`159873097824`, SHA-256
`96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.

One excluded warmup and two retained row-split requests were sufficient for
the predeclared early screen. Both retained responses had 1,129 prompt tokens,
128 generated tokens, HTTP 200, and exact decoded-content SHA-256
`3c9bcc5624f4a1d6558c51be89e2443ffba4b2206d10b632c89b7ef64fdd026f`.

| Mode | Prompt samples, tok/s | Generation samples, tok/s | Mean prompt | Mean generation |
| --- | --- | --- | ---: | ---: |
| P11 layer control | 203.901655, 203.721995 | 16.653701, 16.601290 | 203.811825 | 16.627496 |
| P14 row candidate | 203.830938, 203.981451 | 16.556728, 16.667760 | 203.906194 | 16.612244 |

Against the retained P11 control means, row mode changed prompt throughput by
`+0.046302%` and generation throughput by `-0.091725%`. This mixed,
noise-scale result is not an improvement and generation moved in the wrong
direction. It is an early-rejection screen, not a confidence interval or final
G9/G10 non-inferiority result.

## Decision and rollback

Reject row split and stop configuration expansion. Balanced `1,1` layer split
remains the best validated control. Reciprocal placement would require
duplicating the 160 GB artifact onto nimo-1 and is deferred because the model
is absent there and the node has insufficient free space. Fine-grained split,
fault, and full-volume matrices are also deferred absent a positive signal or
concrete risk hypothesis.

The production nimo-2 worker was restored before the nimo-1 coordinator. P14
makes no speedup, universal-superiority, greater-than-30-token/s, persistence,
donor, license, WebUI, reference-clone, or deployment-default claim. Raw
requests, responses, timings, telemetry, systemd journals, input hashes, and
split records remain separate under
`/var/tmp/halofpx-p14-row-split-20260720` on nimo-2.
