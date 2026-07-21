# P12 MiniMax-M2 Q6 MMVQ opportunity gate

Status: **synthetic opportunity gate passed; product implementation remains unopened and default-off**

P12 answers the narrow question left by P11: can the existing tuned gfx1151
Q6 MMVQ path compute a compact four-owned expert half with enough latency
margin to justify a new ownership/scatter implementation seam? It does not
claim RPC overlap, exact-model speedup, or product readiness.

## Authority and scope

The implementation parent is HaloFPX commit
`a0d9299c89cb50291d6359ea680330000e6d5822`, tree
`5beaea53bfbfa316c5afd81804a31792e6f8c515`. The canary remains behind the
existing default-off `HALOFPX_MINIMAX_M2_EXPERT_PARTITION_CANARY` build option.
The only implementation/test source change is `tests/test-backend-ops.cpp`;
the remaining changes are documentation and evidence. No model graph, public ggml op, RPC
protocol, runtime default, persistence behavior, donor code, or deployment is
modified.

The exact synthetic tuple comes from the pinned MiniMax-M2.7 primary workload:
Q6_0_ROCMFPX weights, 192 global experts, top-8, hidden 3072, intermediate
1536, and one decode token. The compact lower bound uses one physical
96-expert half and four owned IDs. It deliberately does not model an arbitrary
0--8 ownership distribution, scatter-to-eight metadata, cross-rank transport,
or scheduling overlap.

## Harness correction

The first whole-chain perf class was invalid for this gate. The generic perf
runner duplicates and accounts for the output operation, while the proposed
output was a small reduction after the expert projections. Its approximately
2.4 microsecond result and 36 kB/run accounting could not represent the Q6
chain. Those logs are retained under `excluded-invalid-chain-*`; they are not
promoted measurements.

The corrected harness uses the existing `test_mul_mat_id` perf case whose
declared FLOP count and output operation are the MMVQ projection being timed.
It registers full and compact exact-shape gate/up and down projections. A
separate existing 4096-element F32 add case provides a conservative launch and
reduction proxy. The corrected exact-shape cases also run in correctness mode.

## Representative Linux qualification

The candidate was built in Release mode on nimo-2 using the ROCm compiler at
`/opt/rocm/lib/llvm/bin/clang++`, gfx1151, forced MMQ, no VMM, HIP on, Vulkan
off, and the canary on. The admitted perf binary SHA-256 is
`6fa571378696c159fc2ebca19e912e584c5933b2b1ba2c68cda7eeabe195a838`.
Production was quiesced for the timing run and restored worker-first then
coordinator. `HSA_ENABLE_SDMA=0` matched the qualified primary-model tuple.

| Projection | Full top-8, us | Compact owned-4, us | Compact/full |
| --- | ---: | ---: | ---: |
| gate/up, 3072 -> 1536 | 100.74 | 52.20 | 51.82% |
| down, 1536 -> 3072 | 108.60 | 56.25 | 51.80% |

The two-gate-plus-down lower-bound chain is 310.08 microseconds full and
160.65 microseconds compact, or **51.8092%**. This passes the predeclared
`<60%` compute gate.

The 4096-element F32 add proxy measured 2.28 microseconds. Conservatively
charging five such launches for weighting, local fixed-order reduction, and
rank join is 11.40 microseconds, or **3.6765%** of the inherited 310.08
microsecond chain. This passes the `<5%` join/reduction gate. The proxy is a
screen, not a substitute for timing the eventual exact implementation.

The final exact-shape binary SHA-256
`565a4a5a42ad6f7a4833cbbc9b0bdd7fe1eddeedfa714fe689e26d714614478f`
passed all four full/compact gate/down cases against the CPU reference on
ROCm0. The prior test-mode command selected 0/0 cases because the cases were
then perf-only; it is retained as `excluded-zero-selected-correctness.log` and
is not a pass.

## Decision and next seam

The lower-level opportunity is large enough to open exactly one time-boxed
private HIP implementation seam around the existing MMVQ launches. That seam
must compact owned IDs on device, preserve global-ID validation, produce
deterministic scatter/reduction semantics, and remain exact-tuple/default-off.
It must prove direct correctness before any RPC or model-graph integration.

The synthetic ratio does not establish end-to-end benefit. Any later exact
model regression closes and removes the candidate. Generation above 30 tok/s
remains a stretch objective, not a fact or acceptance threshold.

Raw configuration, builds, source/binary hashes, admitted perf and correctness
logs, excluded diagnostics, and manifests are retained on nimo-2 under
`/var/tmp/halofpx-p12-mmvq-a0d9299`. The synthesis-only bundle is
`/var/tmp/halofpx-p12-mmvq-nimo2-20260720.tar.zst`, SHA-256
`0b908153b5c72ca1dc76a861f08378be9411839c44b1fc013aeaa066e5d2afaa`.
