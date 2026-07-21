# P09 MiniMax-M2 masked-expert decode rejection and RPC compute-view repair

Status: **optimization rejected for regression; narrow RPC correctness repair retained**

P09 tested a default-off, one-layer MiniMax-M2 decode candidate intended to
avoid inactive expert-slot work on the pinned 160 GB primary workload. The
candidate produced correct output after exposing and repairing an independent
RPC graph reconstruction defect, but it was clearly slower than the matched
feature-off control. All optimization code was removed. Only the target-native
RPC compute-view repair remains in the implementation tree.

## Authority and scope

The implementation parent is HaloFPX commit
`788e59654bbe5b642020c25ab1428ef40991c179`, tree
`7dfb78a35f202df2fbb5b22aa87edab9a5554ab1`. The retained source change is
limited to `ggml/src/ggml-rpc/ggml-rpc.cpp`: an unresolved graph-compute view
whose source has no buffer remains unresolved until backend allocation, while
an already allocated weight view continues to be rebuilt and validated from
the server-side source allocation.

The rejected candidate was guarded by the strict default-off environment
switch `HALOFPX_MINIMAX_M2_EXPERT_MASKED_DECODE=1`, required the existing P06h
gates, applied only to the exact layer-32 routing-only MoE decode shape, and
used an inactive-slot sentinel with an early Q6 zero path. That complete patch
is preserved only as raw rejected evidence (`p09-final-rejected.patch`,
SHA-256 `ea83fca4c32035c189ee9d128438defb403b1a8474ff8419dd698588243101b1`).
It is absent from the retained source and is not a promotion candidate.

No donor code, GPL llama-ai implementation, CachyLLama code, new dependency,
persistent behavior, WebUI behavior, model data, deployment binary, Git remote,
or reference clone changed.

## Correctness discovery and retained repair

The first exact-model candidate request aborted in the RPC worker at
`ggml-backend.cpp:1984`. The server reconstructed every serialized view by
calling `ggml_backend_view_init`, including graph-compute views whose source
buffer is correctly null before backend allocation. The repair distinguishes
that legitimate unresolved state from allocated weight views; it does not
weaken the P06g server-side weight-view reconstruction or validation.

After the repair, the candidate returned HTTP 200 with 1,129 prompt tokens and
128 generated tokens. Across the excluded warmups and all nine retained C-A-C
requests, decoded content and token sequences were identical. The final
retained source was then rebuilt from a clean Ninja graph on both Linux Strix
Halo nodes. Its focused feature-off and L02 contracts passed 2/2, and the Q6
expanded-device view test passed both directly on ROCm and through a disposable
RPC worker with reference L2 `24.3547155`, NMSE `0`, and maximum absolute error
`0`.

The clean builds on both nodes produced identical hashes: `libggml-rpc.so`
`dd114298cfe6324b93414f692d593e8418ae14729e49d40b9446e2eaedf5e95a`,
`llama-server` `08dc2418b1c212fdd343a3016fd27527302a27a0659fc3357edb58d22175c7f7`,
`rpc-server` `4c064db785e6959bd7ed5a652ee8f70e56b4c4ce1926a5216ece4fb249734293`,
and the focused test
`e5a35264e5df058f088a10b2f8032eb9b155463efcfe33684864ee2e08d94134`.
The sealed roots also preserve `final-source-binaries.sha256` from the
superseded pre-clean incremental build; its focused-test hash `948e11d...` is
not retained-build authority. `final-clean-build.log`, `final-clean-*.txt`, and
the copied clean evidence binary are the authoritative clean-rebuild record.

## Matched rejection screen

The workload remained repository
`rcmorano/saricles-MiniMax-M2.7-REAP-172B-A10B-ROCMFPX`, revision
`dba517197f2854f3d362529e13abddcdcad6c10b`, exact file
`saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf`, size
`159873097824` bytes, SHA-256
`96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.
The request SHA-256 was
`f5b273f95a852d5e34252715bc953fa4eb101273d262b9d778c16138d7c00b7c`.

One excluded warmup and three retained requests were run in each C-A-C phase.
The control combines the six retained A and C observations; this short screen
is sufficient to reject a multi-percent regression, not to establish final
non-inferiority.

| Metric | Control A+C mean (n=6) | Candidate mean (n=3) | Candidate delta |
| --- | ---: | ---: | ---: |
| Prompt processing | 203.5563 tok/s | 194.9888 tok/s | -4.2089% |
| Token generation | 16.6733 tok/s | 16.2130 tok/s | -2.7606% |

Control prompt samples were `203.35, 203.62, 203.49, 203.79, 203.64,
203.45`; candidate samples were `194.57, 195.12, 195.28`. Control generation
samples were `16.69, 16.64, 16.68, 16.70, 16.64, 16.69`; candidate samples
were `16.15, 16.22, 16.27`. The candidate is rejected and its feature gate and
implementation are removed. The owner's greater-than-30 tok/s objective
remains a stretch objective, not a baseline or pass/fail assertion.

## Failure handling, rollback, and deferred work

During teardown, the Control-C coordinator and candidate worker were initially
stopped in parallel. All retained responses had already completed, but the
coordinator queried worker memory after the worker stopped and aborted during
cleanup. This does not affect the measurements; it establishes the operational
rule to stop the coordinator before the worker, sequentially. The known-good
nimo-2 worker and then nimo-1 coordinator were restored in the documented
startup order. They remain active with zero restarts, nimo-2 listens on port
50052, and nimo-1 reports `{"status":"ok"}` on port 8081 with the original
deployed server hash.

Risk-proportionate steering intentionally defers a larger malformed-graph and
fault-injection matrix. The retained repair is covered by the exact crash
reproduction, successful exact-model retry, clean direct/RPC focused test,
feature-off contracts, and independent review. Additional permutations require
a concrete defect hypothesis.

Raw build, request, response, timing, crash, service, source, hash, and test
evidence is sealed in verified mode-0600 bundles on both nodes. The next
product-visible lane is a narrow, separately gated L14Q quantized-KV manual-port
candidate; P09's masked-slot design will not be expanded without new evidence.
