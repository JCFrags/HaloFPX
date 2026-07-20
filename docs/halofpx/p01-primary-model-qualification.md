# P01 pinned primary-model qualification

Status: **qualified early matched control; final G9/G10 zero-regression gate remains open**

P01 establishes the first versioned primary performance workload for HaloFPX. It
does not claim a speedup or universal engine superiority. It demonstrates that
the pinned 160 GB ROCmFPX artifact loads and produces identical deterministic
output on the selected two-node Strix Halo topology, and that the HaloFPX
candidate has no observed point-estimate regression against the locked ROCmFPX
base in a small interleaved qualification.

## Artifact authority

- Repository: `rcmorano/saricles-MiniMax-M2.7-REAP-172B-A10B-ROCMFPX`
- Revision: `dba517197f2854f3d362529e13abddcdcad6c10b`
- File: `saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf`
- Published and observed size: `159873097824` bytes
- SHA-256: `96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`
- Qualified copy: nimo-2 under the revision-addressed
  `/opt/llm-usb4-cluster/models/rcmorano_saricles-minimax-m2.7-reap-172b-a10b-rocmfpx/`
  directory

The artifact was hashed again after the timing blocks. “Latest” is not an
accepted identity for this workload.

## Matched runtime tuple

The locked control is ROCmFPX commit
`61f2f2d7bc4955e9bca821095ef69125837133b5`, tree
`0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd`. The candidate is HaloFPX commit
`7e7c224947cc3844b40fef12cf5731aae24a1101`, tree
`2608ddbbecf157dd010431fb57e43a31ec103f83`.

Both were clean builds on both nodes with the same Release, HIP, Vulkan, RPC,
gfx1151, MMQ, server, and WebUI-off configuration. The HaloFPX context-store
canary was explicitly `OFF`. Matching binary hashes across nimo-1 and nimo-2
were:

| Binary | Control SHA-256 | HaloFPX SHA-256 |
| --- | --- | --- |
| `rpc-server` | `3327b1d7165d6084aeff8694163b1a69a1110dd7538a89389794bba9d78d5868` | `c90e36904effd643d7acb2646cb1ef1b935c18e9e0ad83654eb497c4b9ab0aae` |
| `llama-server` | `d752b7f327b51d50f3a868fda537ffec90f999ae2144ad61416203ccc12d4b4c` | `26d58b4e1fafa1a4f18eaa0c589b14afae18183e7ca8db65b3a4024ee23dff7c` |

The experiment intentionally placed the RPC worker on nimo-1 and the
coordinator/model on nimo-2 because the pinned artifact is local to nimo-2.
Both processes used the existing MPTCP wrapper over `10.44.0.1:50053`.
Placement was `RPC0,ROCm0`, tensor split `1,1`, layer split, context 4096,
parallel 1, 16 CPU threads, all model layers on GPU, flash attention on,
Q8_0 K/V cache, batch/ubatch 512, direct I/O, no mmap, fixed seed 1234,
temperature 0, offline operation, and no WebUI.

## Workload and result

The raw completion workload has 1129 prompt tokens and forces 128 generated
tokens with EOS ignored. Prompt text, sampling controls, token return, and
timings are fixed; the retained block-2 request body SHA-256 is
`f5b273f95a852d5e34252715bc953fa4eb101273d262b9d778c16138d7c00b7c`.

Execution order was candidate block 1, control block 1, candidate block 2,
control block 2. Each block contains one excluded warm-up and five retained
requests. All 24 timing requests returned HTTP 200. All generated content,
including warm-ups, has canonical extraction SHA-256
`a9c38c7f948adcfa8cfab5468ab84cc089b01a34c3f270f1c487a9a5fa74b555`.
That digest is over the UTF-8 `.content` value followed by the single LF emitted
by `jq -r`; the JSON string value without that LF hashes to
`3c9bcc5624f4a1d6558c51be89e2443ffba4b2206d10b632c89b7ef64fdd026f`.
An additional matched chat check returned exactly `HALOFPX_PRIMARY_OK` for both
binaries.

| Metric | Control, mean ± sample SD | HaloFPX, mean ± sample SD | HaloFPX delta | Approx. 95% Welch CI |
| --- | ---: | ---: | ---: | ---: |
| Prompt processing | 203.1247 ± 0.1275 tok/s | 203.1604 ± 0.1244 tok/s | +0.0176% | -0.0827 to +0.1540 tok/s |
| Generation | 16.6580 ± 0.0276 tok/s | 16.6710 ± 0.0167 tok/s | +0.0783% | -0.0087 to +0.0348 tok/s |
| End-to-end completion | 13.2460 s | 13.2388 s | -0.0544% | descriptive only |

The point estimates are favorable, so no slowdown was observed. Both
throughput intervals cross zero; therefore P01 does not close the final strict
zero-regression non-inferiority gate and does not claim that HaloFPX is faster.
The owner’s generation objective above 30 tok/s remains an aspirational stretch
goal, not a baseline or pass/fail threshold. The present 16.67 tok/s result is
the first controlled value for this exact tuple.

## Versioned matrix and follow-on

Matrix version `halofpx.performance-matrix.v1` currently has one admitted row:
this exact model, ROCmFPX weight format, HIP+RPC two-node backend, 4096 context,
Q8_0 K/V, and deterministic raw completion API. A future row is admitted only
with a pinned model artifact and matched control tuple. Universal “fastest
engine” language is prohibited until representative model, backend, context,
and API rows support it.

The next performance work is:

1. narrow, separate, default-off L14Q HIP decode and Vulkan prefill candidates;
2. interleaved control/candidate trials after each admitted optimization;
3. enough final trials, telemetry, and topology coverage to close G9/G10;
4. other compatible engines only when the exact artifact and matched tuple can
   actually be run.

The exhaustive cache fault matrix remains deferred under the owner’s
risk-proportionate testing direction. P01 changes no runtime code, feature
default, model, deployed service, reference clone, remote, notice, or SBOM.
After qualification, the original nimo-2 RPC worker and nimo-1 UD-Q6 server
were restored and directly health-checked.

Machine-readable details and raw bundle hashes are in
[`evidence/p01-primary-model-qualification-receipt.json`](evidence/p01-primary-model-qualification-receipt.json).
