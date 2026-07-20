# P07 current-HEAD matched feature-off primary-model baseline

Status: **bounded matched baseline complete; final G9/G10 remains open**

P07 establishes a repeatable Linux Strix Halo build-and-measurement harness for
the exact 160 GB primary model. It compares current HaloFPX HEAD with the locked
ROCmFPX base under one risk-proportionate C-A-A-C block. It is not a speedup
claim and does not enable persistence or either L14Q candidate.

## Exact authority

The control is ROCmFPX commit
`61f2f2d7bc4955e9bca821095ef69125837133b5`, tree
`0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd`. The candidate is HaloFPX commit
`ed17a504956e410d381c1c2f8aa741953a3a97db`, tree
`f7d67ae2ed7315341fda4ebc8521326241c13162`. The exact clean source archive
used on both nodes has SHA-256
`0d7971bb9e832f251d733bd7dec6ada7245ef43b4412218d2b2e955b38ecf45d`.

The workload is repository
`rcmorano/saricles-MiniMax-M2.7-REAP-172B-A10B-ROCMFPX`, revision
`dba517197f2854f3d362529e13abddcdcad6c10b`, file
`saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf`, size
`159873097824` bytes, SHA-256
`96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.

## Compiler-tuple correction

The first candidate attempt was invalid because CMake selected system Clang
22.1.6 for HIP instead of the ROCm compiler used by the control. It produced
about 156 prompt tok/s, about 17 generation tok/s, and one punctuation-level
decoded-output difference. Focused checks against the P04 binary, P06d source,
clean HEAD, and pre-P06d source localized the problem to the build tuple rather
than the HaloFPX source. Those runs remain raw, explicitly excluded diagnostics.

The reusable build script now exports both
`HIPCXX=/opt/rocm/lib/llvm/bin/clang++` and `HIP_PATH=/opt/rocm`. The admitted
build identifies AMD Clang 22.0.0git at that path, whose SHA-256 is
`eea67555a6140711b04c60b03beac7cda68799446afd6aa501a7094d959a363e`
on both nodes. Control and candidate now match on Release, gfx1151, HIP,
Vulkan, RPC, forced MMQ, no VMM, GCC host C/C++, and AMD ROCm HIP compiler.

| Binary | Control SHA-256 | Candidate SHA-256 |
| --- | --- | --- |
| `rpc-server` | `3327b1d7165d6084aeff8694163b1a69a1110dd7538a89389794bba9d78d5868` | `4d50f74f1a0ede892ce0790303bdda637f7f5899e93ad8cd1d4816880606dbcf` |
| `llama-server` | `d752b7f327b51d50f3a868fda537ffec90f999ae2144ad61416203ccc12d4b4c` | `c630ef90ffac9bd304b364d1edac34a7c5bce022386182a6652600d9982ad0ee` |

Candidate binaries and `CMakeCache.txt` are byte-identical across nimo-1 and
nimo-2. All context-store, registry-lab, HIP quantized-KV, and Vulkan Q8
pre-dequant candidates were off. WebUI and tests were not built.

## Matched execution and results

The tuple is nimo-1 RPC worker, nimo-2 coordinator, `RPC0,ROCm0`, tensor split
`1,1`, layer split, context 4096, parallel 1, 16 threads, all layers on GPU,
flash attention, Q8_0 K/V, batch and ubatch 512, direct I/O, no mmap, seed 1234,
temperature 0, offline, WebUI off, and `HSA_ENABLE_SDMA=0`. The request uses
1129 prompt tokens and requires 128 generated tokens. Its SHA-256 is
`f5b273f95a852d5e34252715bc953fa4eb101273d262b9d778c16138d7c00b7c`.

Execution order was control block 1, candidate block 2, candidate block 3,
control block 4. Each block used one excluded warmup and five retained requests:
20 retained samples total, 10 per variant. Every retained request returned HTTP
200 with exact token counts and byte-identical decoded content, SHA-256
`3c9bcc5624f4a1d6558c51be89e2443ffba4b2206d10b632c89b7ef64fdd026f`.

| Metric | Control, mean +/- sample SD | Candidate, mean +/- sample SD | Candidate delta | Approx. normal 95% interval |
| --- | ---: | ---: | ---: | ---: |
| Prompt processing | 203.7166 +/- 0.1247 tok/s | 203.6683 +/- 0.2405 tok/s | -0.02369% | -0.2161 to +0.1196 tok/s |
| Generation | 16.64088 +/- 0.02318 tok/s | 16.63979 +/- 0.03892 tok/s | -0.00651% | -0.02916 to +0.02699 tok/s |
| End-to-end curl wall time | 13240.498 +/- 13.843 ms | 13242.571 +/- 21.660 ms | +0.01566% | -13.859 to +18.006 ms |

No interval excludes zero, so P07 does not establish a reproducible regression.
All three point estimates are slightly adverse, however, and the intervals do
not prove non-regression. The strict final G9/G10 gate therefore remains open.
Generation above 30 tok/s remains an owner stretch objective, not a baseline or
pass/fail result.

## Rollback and evidence

The preserved six-shard MiniMax deployment was restored after the matrix. On
its first load the nimo-2 RPC worker was OOM-killed; the resulting lost peer
caused the nimo-1 coordinator to abort in `ggml_backend_rpc_buffer_set_tensor`.
Systemd restarted both services once. The second load completed, both services are enabled and
active, nimo-1 returns HTTP 200 `{"status":"ok"}`, and the original deployed
binary hashes match. The first-load incident is retained in the post-restore
journals and is not hidden as a zero-restart recovery. It did not alter the
admitted matrix. It is a follow-up rollback-memory-pressure reliability defect,
not evidence about P07 throughput.

Raw build, request, response, telemetry, excluded-diagnostic, and rollback
evidence is separate from this synthesis under
`/var/tmp/halofpx-p07-head-ed17a50-20260720` on both nodes. Each root has a
verified SHA-256 manifest and a verified `tar.zst` bundle. Authorization-like
environment assignments and private-key PEM markers were not detected.

All five immutable reference repositories remain clean at their locked commits
and trees. P07 changes no product runtime source, default, deployed model,
reference clone, remote, license, notice, or SBOM. No donor or GPL implementation
code is introduced.

## Risk-proportionate boundary

P07 deliberately defers final-volume strict non-inferiority, TTFT/ITL streaming
qualification, exhaustive fault permutations, physical model sharding, and
other-engine comparisons until a concrete optimization or final gate requires
them. The next performance work should use this harness and exact compiler pin;
it should not repeat the excluded compiler archaeology.
