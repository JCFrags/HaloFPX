# P08 exact-model critical-path profile

Status: **diagnostic profile complete; serialized rank phases are the next optimization target**

P08 profiles one representative request against the pinned 160 GB primary
artifact on the two Linux Strix Halo nodes. It is deliberately a bottleneck
map, not a throughput promotion trial. One excluded uninstrumented warmup and
one retained instrumented request were sufficient because the result exposes a
large utilization and wait-time signal rather than a marginal performance
difference.

## Authority and boundary

The implementation parent is HaloFPX commit
`4ba28db38e2667e8b3b278b02b070130c7cf163f`, tree
`c5d0b7675407908076f6afff834d3c3894abb114`. The profiled P06h Release binary
contains the same runtime source semantics as this parent: the intervening P06i
commit is documentation-only. This is not a byte-identical source-archive
claim. The node copy of `src/models/minimax-m2.cpp` has SHA-256
`db3bc908c5363ed5295ec304849e857d1c45ba3bc5039750cded6a470f76cf46`;
its 146 extra CR bytes are CRLF line endings, and normalizing them to LF yields
the exact committed-file SHA-256
`d7043590080f882e35f8428a39b1a9cbe0bcb4acbbbbd1d46d2402f5eec8346f`
with no textual delta. Its server and RPC SHA-256 values are respectively
`d451de4d870d1cc16e42039aa3a46baaea4d80d6ca53a28e73f3ec1c5fd209b9`
and `e9dfe21d0099883305630fe51a12b18d69c3bbfb5e1c92d0974750ab4fd1aea3`.
All HaloFPX experimental environment gates were absent.

The exact model remains repository
`rcmorano/saricles-MiniMax-M2.7-REAP-172B-A10B-ROCMFPX`, revision
`dba517197f2854f3d362529e13abddcdcad6c10b`, file
`saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf`, size
`159873097824` bytes, SHA-256
`96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.
P08 reused the previously verified model lock rather than rereading 160 GB only
to repeat the hash.

The runtime tuple matches P07: nimo-1 RPC worker, nimo-2 coordinator,
`RPC0,ROCm0`, layer split `1,1`, MPTCP, Q8_0 K/V, context 4096, batch/ubatch
512, all layers on GPU, FlashAttention, direct I/O, seed 1234, temperature 0,
WebUI off, and `HSA_ENABLE_SDMA=0`. `--no-warmup` affected only startup; the
explicit excluded request warmed the complete request path.

## Correct retained request

The request SHA-256 was
`f5b273f95a852d5e34252715bc953fa4eb101273d262b9d778c16138d7c00b7c`.
The retained request returned HTTP 200, 1,129 prompt tokens, 128 generated
tokens, and exact decoded-content SHA-256
`3c9bcc5624f4a1d6558c51be89e2443ffba4b2206d10b632c89b7ef64fdd026f`.
It reported 200.7557 prompt tok/s, 16.74805 generation tok/s, and 13.299162 s
curl wall time. The excluded uninstrumented warmup took 13.279694 s, so the
observed whole-request profiler margin was 19.468 ms, or 0.1466%. These two
samples are not a speed comparison or a variance estimate.

## Bottleneck evidence

The 100 ms GPU samples and filtered syscall trace show balanced but serialized
rank work:

| Request phase | nimo-1 worker GPU busy mean | nimo-2 coordinator GPU busy mean |
| --- | ---: | ---: |
| Prompt | 42.69% | 35.15% |
| Generation | 47.65% | 49.28% |
| Whole request | 45.53% | 43.30% |

Worker phase windows use the coordinator's realtime boundaries and therefore
carry cross-host NTP uncertainty; the roughly half-duty conclusion is much
larger than the 100 ms sample and clock uncertainty. Neither rank is close to
sustained saturation during decode.

The nimo-1 RPC process recorded 129 filtered `recvfrom` calls totaling
6,260.411 ms; nimo-2 recorded 128 totaling 6,531.586 ms. These duration sums
can overlap other threads and are not themselves wall-clock fractions, but the
repeated approximately 29--31 ms waits on both ranks align with alternating
remote/local compute phases. Monitor-window process CPU means were only 65.3%
and 62.6% of one core-equivalent across the full processes.

Both USB4 rails were active and balanced. During the 20-second monitor window,
nimo-2 transmitted 20,484,400 bytes on `thunderbolt0` and 19,863,341 bytes on
`thunderbolt1`, receiving 8,644,496 and 9,578,932 bytes. Nimo-1 observed the
inverse. Tens of megabytes over the window are far below link capacity, so
aggregate bandwidth saturation is not supported.

The AMDGPU scheduler tracepoints emitted no usable records for this HIP path;
the empty decoded reports are retained and excluded. ROCprofiler CLI was not
installed, and installing it or repeating a full model load was not justified
after the coarse GPU, socket-wait, and network signals converged.

## Decision

The evidence supports an `[INFERENCE]` that single-token decode alternates
roughly 30 ms remote and local rank phases. That explains about 60 ms per token
and makes the owner's greater-than-30 tok/s stretch objective physically
plausible only if substantial rank work can be overlapped or removed. It does
not prove a particular source-level cause.

The next performance candidate should reduce the per-token rank dependency,
for example through a compact, fused expert-parallel exchange that evaluates
only locally owned selected experts and overlaps the two ownership domains.
P06i already closed the naive eight-slot-per-rank implementation. Small-send
coalescing, dual-rail-only tuning, and another L14Q permutation remain
deprioritized unless new evidence shows they dominate.

## Rollback, evidence, and nonclaims

The disposable coordinator and worker stopped successfully. The original
nimo-2 worker was restored first, followed by the nimo-1 coordinator. Both are
active with zero restarts and their original binary hashes; nimo-2 listens on
port 50052 and nimo-1 health is HTTP 200.

Raw commands, environments, responses, timing, CPU, syscall, GPU, network,
journals, and summaries are preserved in verified mode-0600 bundles on their
originating nodes. P08 changes no engine runtime, build default, deployment,
model, persistence, WebUI, dependency, provenance boundary, reference clone,
or Git remote.
