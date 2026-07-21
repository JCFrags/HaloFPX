# P11 layer-placement screen and expert-overlap decision

Status: **remote-heavy placement rejected; narrow MMVQ overlap design retained but not opened**

P11 applies the project's risk-proportionate performance steering to the
serialized-rank signal from P08. It first tests whether a configuration-only
placement correction can improve the pinned exact-model workload before
opening another multi-file custom RPC operation.

## Authority and fixed workload

The source parent is HaloFPX commit
`94d3e18c6e417502fa2fdefeac6093eae6cb8d23`, tree
`7a4fdfa56a1fe2bee01d4ee55f817323d71b42aa`. Both variants use the same
feature-off P09 binaries, compiler/runtime tuple, nimo-1 RPC worker and nimo-2
coordinator topology, `RPC0,ROCm0`, MPTCP, layer split, Q8_0 K/V, context 4096,
batch/ubatch 512, seed 1234, temperature zero, and qualified request.

The model is repository
`rcmorano/saricles-MiniMax-M2.7-REAP-172B-A10B-ROCMFPX`, immutable revision
`dba517197f2854f3d362529e13abddcdcad6c10b`, exact file
`saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf`, size
`159873097824`, SHA-256
`96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.

## Bounded screen

The reusable primary-block harness now accepts and records an optional tensor
split while preserving `1,1` as its default. Input validation admits only a
nonnegative numeric pair with a nonzero total. This is a measurement-harness change, not an engine
default or runtime behavior change.

Each variant used one excluded warmup and two retained requests. All four
retained responses returned 1,129 prompt tokens and 128 generated tokens with
the qualified exact decoded-content SHA-256
`3c9bcc5624f4a1d6558c51be89e2443ffba4b2206d10b632c89b7ef64fdd026f`.

| Tensor split (`RPC0,ROCm0`) | Prompt samples, tok/s | Generation samples, tok/s | Mean prompt | Mean generation |
| --- | --- | --- | ---: | ---: |
| `1,1` | 203.901655, 203.721995 | 16.653701, 16.601290 | 203.811825 | 16.627496 |
| `1.1,0.9` | 203.379669, 203.336776 | 16.579050, 16.580648 | 203.358223 | 16.579849 |

The remote-heavy candidate changed prompt throughput by `-0.22256%` and
generation throughput by `-0.28656%`. This small screen is not a confidence
interval or final non-inferiority trial, but both candidate samples are below
both balanced samples. It is sufficient for the predeclared early rejection;
no reciprocal or fine-grained placement sweep is justified.

## Expert-overlap decision

Independent design review found one materially different source candidate:
specialize the existing tuned MMVQ path for the exact MiniMax layer-32 Q6
tuple, translate global expert IDs to the physical 96-expert rank allocation
on device, produce deterministic zero for unowned slots, fuse gate/up plus
SwiGLU, then run the down projection and a fixed-order reduction. This avoids
P10's one-wave-per-row custom kernel and P09's graph-level mask/cast overhead.

The candidate remains credible but unopened. It requires a marked operation,
strict RPC validation, exact ownership dispatch, and direct/RPC synthetic
coverage for a possible gain in only one of 62 layers. It may be opened only
as one time-boxed prototype with a device-event kill gate: owned-rank latency
below 60% of the inherited eight-expert chain and join overhead below 5%.
Failure closes the expert-overlap lane without loading the 160 GB model.

## Decision and rollback

Balanced `1,1` remains the best validated placement. P11 makes no speedup
claim, no engine-source change, and no persistence, WebUI, cache, provenance,
license, model, reference-clone, deployment-default, or remote change. The
broader placement matrix, reciprocal split, exhaustive faults, and full-volume
statistics are deferred because no high-risk hypothesis or positive signal
justifies them.

After the disposable screen, the nimo-2 production worker was restored before
the nimo-1 coordinator. Raw request, response, timing, GPU, network, systemd,
input-hash, and split records remain separate from this synthesis under
`/var/tmp/halofpx-p11-placement-20260720` on their originating nodes.
