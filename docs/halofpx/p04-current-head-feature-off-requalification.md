# P04 current-HEAD feature-off primary-model requalification

Status: **bounded requalification complete; strict zero-regression gate remains open**

P04 compares the first complete default-off full-v1 server-canary source against the locked ROCmFPX control on the exact 160 GB primary model. It is a feature-off compatibility checkpoint, not a speedup claim or final G9/G10 non-inferiority result.

## Exact authority and build

The control remains ROCmFPX commit `61f2f2d7bc4955e9bca821095ef69125837133b5`, tree `0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd`. The candidate is HaloFPX commit `6c01b9769c3d0c0034acfdd3c1e47c3f632ea670`, tree `d0905b9c2d77290d1f0b4dcf898aaeb4c3c60432`. Its exact source archive SHA-256 is `6fed59764da4f3d9c02737382c6fc1da52c92e539a0fe9c487cf0971e8650154`.

The documentation-only P03 commit followed this source archive and changes no runtime path. P04 deliberately identifies the exact benchmarked source rather than relabeling it as the later repository tip.

Fresh candidate Release HIP+Vulkan+RPC gfx1151 builds used GCC 16.1.1 and CMake 4.3.4 on both Linux Strix Halo nodes. Their `CMakeCache.txt`, `rpc-server`, and `llama-server` hashes are byte-identical. The control binaries are the locked-base P01 builds under the same build/runtime tuple.

| Binary | Control SHA-256 | Candidate SHA-256 |
| --- | --- | --- |
| `rpc-server` | `3327b1d7165d6084aeff8694163b1a69a1110dd7538a89389794bba9d78d5868` | `3e8d0d940fb040cf9940cc3d36660428e8cc0dddcc4367cedaa12febb12cd16d` |
| `llama-server` | `d752b7f327b51d50f3a868fda537ffec90f999ae2144ad61416203ccc12d4b4c` | `8cb6f890930afac6d26fb0cbd81be209bfb085f86600a3d4706e722d24f028e4` |

All HaloFPX context-store gates, registry-lab gates, L14Q-H01, and L14Q-VK-01 were `OFF`. WebUI and tests were not built. The experiment therefore exercises the ordinary inherited server path; no persistent-cache or quantized-KV candidate was enabled.

The pinned workload remains repository `rcmorano/saricles-MiniMax-M2.7-REAP-172B-A10B-ROCMFPX`, revision `dba517197f2854f3d362529e13abddcdcad6c10b`, file `saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf`, size `159873097824` bytes, SHA-256 `96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.

## Matched execution

The runtime tuple matches P01/P02: nimo-1 RPC worker, nimo-2 coordinator, `RPC0,ROCm0`, layer split `1,1`, 4096 context, parallel 1, 16 threads, all layers on GPU, flash attention, Q8_0 K/V, batch and ubatch 512, direct I/O, no mmap, seed 1234, temperature 0, offline, and WebUI disabled. Both MPTCP subflows were present in every retained block: the primary `10.44.0.1`/`10.44.0.2` path and secondary `10.44.0.5`/`10.44.0.6` path. Both rails had nonzero traffic.

Execution order was control block 1, candidate block 1, candidate block 2, control block 2. Each block used one excluded warmup and three retained requests: 16 admitted HTTP-200 requests and six retained samples per variant. A setup mistake sent four raw-prompt requests to the chat endpoint before the admitted matrix; all returned HTTP 400 without inference, were excluded, and are disclosed in the raw operator note. The rail counters were recaptured before admitted work.

Every admitted response used 1129 prompt tokens and 128 generated tokens. Decoded content was byte-identical across variants: SHA-256 `3c9bcc5624f4a1d6558c51be89e2443ffba4b2206d10b632c89b7ef64fdd026f` with `jq -j`, or `a9c38c7f948adcfa8cfab5468ab84cc089b01a34c3f270f1c487a9a5fa74b555` with the one LF added by `jq -r`.

| Metric | Control, mean +/- sample SD | Candidate, mean +/- sample SD | Candidate delta | Approx. 95% Welch CI |
| --- | ---: | ---: | ---: | ---: |
| Prompt processing | 203.8572 +/- 0.0992 tok/s | 203.7837 +/- 0.2847 tok/s | -0.0361% | -0.3725 to +0.2254 tok/s |
| Generation | 16.6555 +/- 0.0263 tok/s | 16.6499 +/- 0.0210 tok/s | -0.0333% | -0.0364 to +0.0253 tok/s |
| End-to-end curl wall time | 13227.233 +/- 13.542 ms | 13231.449 +/- 16.038 ms | +0.0319% | -14.951 to +23.383 ms |

The candidate point estimates are slightly adverse, by roughly three hundredths of one percent, while every confidence interval crosses zero. This does not establish a reproducible slowdown, but it also does not satisfy the owner's strict final rule that the point estimate be no worse and the interval support non-regression. P04 therefore leaves G9/G10 open and does not promote a performance claim. More retained trials belong at the final gate or after a concrete optimization candidate, not in this bounded safety milestone.

Generation above 30 tok/s remains an aspirational stretch objective, not a known baseline or P04 pass/fail criterion.

## Transport observation and rollback

Total nimo-1 interface deltas for each four-request block were:

| Block | Variant | Receive bytes, both rails | Transmit bytes, both rails |
| --- | --- | ---: | ---: |
| 1 | Control | 161,254,140 | 73,260,298 |
| 2 | Candidate | 161,259,247 | 72,829,082 |
| 3 | Candidate | 161,276,606 | 72,808,844 |
| 4 | Control | 161,226,001 | 72,821,476 |

The stable byte volume and active second rail confirm the P03 launch correction. They do not show that two rails improve end-to-end generation; P03 already deprioritized a larger dual-rail-only matrix.

All eight disposable experiment units stopped with result `success` and status 0 because each coordinator stopped before its worker. The preserved nimo-2 RPC worker was restored first, followed by the nimo-1 server. Both are active with zero restarts and original binary hashes; nimo-1 returned HTTP 200 `{"status":"ok"}` after reload, and the restored connection again showed both USB4 subflows.

All five immutable reference clones remain clean at their recorded commits and trees. P04 changes no runtime source, feature default, deployment, model, reference clone, remote, notice, license, or SBOM. No donor or GPL implementation code is introduced. Raw node evidence and build logs remain separate from this synthesis and are hash-manifested in the receipt.
