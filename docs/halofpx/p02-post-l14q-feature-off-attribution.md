# P02 post-L14Q feature-off primary-model attribution

Status: **qualified attribution; final G9/G10 zero-regression gate remains open**

P02 rechecks the pinned primary workload after the default-off HIP and Vulkan
quantized-KV candidates landed, then captures one low-overhead profile to choose
the next optimization from evidence. It is not a speedup claim or the final
non-inferiority gate.

## Exact authority and feature-off control

The control remains ROCmFPX commit
`61f2f2d7bc4955e9bca821095ef69125837133b5`, tree
`0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd`. The candidate is HaloFPX commit
`0cc67d1790fe2fee23ea8ffb0dc87ff48bb1be68`, tree
`2bacfc47b152a3b7fdfe0bea76743c3ede8acaff`. Its source archive SHA-256 is
`f76990a28a1f60a91d9524879a6780a19436d5438fbff2a0a81dc4e932c53a18`.

Both nodes used matching Release HIP+Vulkan+RPC gfx1151 builds with MMQ and the
server enabled. WebUI, both context-store canaries, registry-lab mutation,
L14Q-H01 (`GGML_HIP_QUANT_KV_FATTN_TILE`), and L14Q-VK-01
(`GGML_VULKAN_FA_Q8_0_PREDEQUANT`) were all `OFF`. Matching candidate hashes on
nimo-1 and nimo-2 are:

| Binary | Control SHA-256 | Candidate SHA-256 |
| --- | --- | --- |
| `rpc-server` | `3327b1d7165d6084aeff8694163b1a69a1110dd7538a89389794bba9d78d5868` | `76c575b5b8463013764253e9a3f8898a0196b3a5fe96174010a2729b6d104dc7` |
| `llama-server` | `d752b7f327b51d50f3a868fda537ffec90f999ae2144ad61416203ccc12d4b4c` | `6618dc53dc9fbecd492632fedb428af067a61f524313e0d16d2f4a81a148cf9b` |

The model remains exactly:

- repository `rcmorano/saricles-MiniMax-M2.7-REAP-172B-A10B-ROCMFPX`;
- revision `dba517197f2854f3d362529e13abddcdcad6c10b`;
- file `saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf`;
- size `159873097824` bytes;
- SHA-256 `96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.

The runtime tuple matches P01: nimo-1 RPC worker, nimo-2 coordinator, MPTCP
wrapper over `10.44.0.1:50053`, `RPC0,ROCm0`, layer split `1,1`, 4096 context,
parallel 1, 16 threads, all layers on GPU, flash attention, Q8_0 K/V, batch and
ubatch 512, direct I/O, no mmap, seed 1234, temperature 0, offline, and WebUI
disabled. The request-body SHA-256 is
`f5b273f95a852d5e34252715bc953fa4eb101273d262b9d778c16138d7c00b7c`.

## Matched result

Execution order was control block 1, candidate block 1, candidate block 2,
control block 2. Each block has one excluded warm-up and three retained
requests: 16 HTTP-200 requests total and six retained samples per variant. All
responses used 1129 prompt tokens and 128 generated tokens. Every response had
the same SHA-256
`3c9bcc5624f4a1d6558c51be89e2443ffba4b2206d10b632c89b7ef64fdd026f`
over the decoded UTF-8 `.content` bytes with no terminator, as emitted by
`jq -j`. The equivalent `jq -r` representation with one trailing LF hashes to
`a9c38c7f948adcfa8cfab5468ab84cc089b01a34c3f270f1c487a9a5fa74b555`.

| Metric | Control, mean +/- sample SD | Candidate, mean +/- sample SD | Candidate delta | Approx. 95% Welch CI |
| --- | ---: | ---: | ---: | ---: |
| Prompt processing | 203.0012 +/- 0.3800 tok/s | 203.0828 +/- 0.0982 tok/s | +0.0402% | -0.3162 to +0.4793 tok/s |
| Generation | 16.6515 +/- 0.0315 tok/s | 16.6567 +/- 0.0238 tok/s | +0.0312% | -0.0311 to +0.0415 tok/s |

Both point estimates are favorable and deterministic output is unchanged, so
P02 found no observed regression. The intervals cross zero; six retained
samples per side do not close the owner's strict final zero-regression gate.
Under the risk-proportionate testing direction, P02 stops here and defers the
larger interleaved trial set to G9/G10. Generation above 30 tok/s remains a
stretch objective, not a pass/fail baseline.

## Profile attribution

One candidate request was sampled every approximately 250 ms with `rocm-smi`,
with concurrent `pidstat`, `mpstat`, pressure, interface-counter, and MPTCP
snapshots. It retained exact output and measured 199.69 prompt tok/s and 16.79
generation tok/s while profiled. A following unprofiled check measured 202.91
and 16.65 tok/s, so profiled throughput is descriptive rather than a matched
performance result.

Across 50 samples, nimo-1 GPU use averaged 40.1% (71% maximum) and nimo-2
averaged 39.2% (70% maximum). Socket-graphics-package power averaged 49.10 W
and 57.87 W respectively. The full request changed interface counters by:

| Node/direction | thunderbolt0 | thunderbolt1 |
| --- | ---: | ---: |
| nimo-1 receive | 40,365,224 bytes | 0 bytes |
| nimo-1 transmit | 18,205,530 bytes | 0 bytes |
| nimo-2 receive | 18,205,530 bytes | 0 bytes |
| nimo-2 transmit | 40,365,224 bytes | 0 bytes |

An isolated 1129-token, one-output-token prompt check likewise moved only
24,717,292/16,092,082 bytes over thunderbolt0 and zero over thunderbolt1.
During the full request MPTCP reported `subflows_total:1`. After rollback, the
preserved known-good deployment established two TCP subflows, one on each
10.44.0.x rail, and reported `subflows_total:2`. Therefore the single-rail P02
result is specific to the experimental launch/endpoint state rather than proof
that the cluster cannot use both rails.

This is the next performance hypothesis: establish a matched two-subflow P02
launch, confirm both rail deltas during inference, and compare complete-model
throughput. It does not authorize replacing the known-good transport or claim
that dual rail will necessarily improve end-to-end inference.

## Rollback, finding, and boundary

All experiment units stopped cleanly. The preserved nimo-2 RPC worker and
nimo-1 server are enabled, active, have zero restarts, and use their original
binary hashes. Nimo-1 returned `{"status":"ok"}` on port 8081 after its model
load; the restored connection showed both USB4 rails.

Stopping the pre-existing nimo-1 server before P02 produced SIGABRT and retained
a 637.1 MB systemd core (`PID 1637898`). Memory was released, the experiment was
unaffected, and the same deployment subsequently reloaded and passed health.
This is an operational defect to diagnose separately, not a P02 correctness or
rollback blocker; the core and journal were preserved and not deleted.

P02 changes no runtime source, feature default, model, deployment, reference
clone, remote, notice, license, or SBOM. No donor or GPL implementation code is
introduced. Raw evidence remains node-local and separate from this synthesis;
bundle identities are recorded in the machine-readable receipt.
