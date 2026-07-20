# L14Q-H02 HIP four-element KV loader rejection

Status: **RUNTIME CANDIDATE REJECTED; GENERATOR REPAIR RETAINED.**

L14Q-H02 tested the narrow performance suggestion left by the H01 review:
widen the target-native HIP Q8_0/Q4_0 tile loader from two to four values per
thread. The experiment remained under `GGML_HIP_QUANT_KV_FATTN_TILE`, which is
compile-time only and defaults `OFF`. No selector, admitted head shape, public
surface, dependency, or persistent-store behavior changed.

The candidate was correct, but it did not beat the current feature-off control
on the primary workload. The runtime widening was therefore removed. The only
retained source change repairs `generate_cu_files.py` so regeneration preserves
H01's already admitted guarded Q8_0/Q4_0 instantiations for exact D128 and D256.
All 130 generator-owned `.cu` files match the tracked files byte-for-byte after
line-ending normalization. The generator now deletes only files bearing its
exact ownership header, so all 132 tracked units remain present and the two
manual TurboQuant units remain byte-identical. The type and head-size roster is
unchanged.

## Bounded correctness

A fresh nimo-1 Release gfx1151 HIP build of the four-element candidate passed
the four admitted KV-length-257 cells: D128 and D256 with symmetric Q8_0 and
Q4_0 K/V. Its CSV is byte-identical to the H01 pair-loader result at SHA-256
`e1769daaa4d17aa4ccc0eb3749b0126993bbd5a13775a2ecccdb322d2fc694b0`.
The explicit D160 control remained unsupported. The focused binary SHA-256 is
`d9810a72366566e5b29251a12edf0949215d06603cad08cada598e7e9a645f93`.

The broader inherited FlashAttention inventory, selector permutations, and
additional fault cases were not repeated. H01 had already qualified those
surfaces, and H02 changed only the width of an admitted loader loop. This was
the risk-proportionate boundary requested by the owner.

## Matched primary-model screen

The exact workload remained:

- repository `rcmorano/saricles-MiniMax-M2.7-REAP-172B-A10B-ROCMFPX`;
- revision `dba517197f2854f3d362529e13abddcdcad6c10b`;
- file `saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf`;
- size `159873097824` bytes;
- SHA-256 `96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.

Fresh OFF, H01-pair, and H02-four-wide builds were byte-identical across both
nodes within each variant. The runtime tuple matched P04/P05: nimo-1 RPC
worker, nimo-2 coordinator, `RPC0,ROCm0`, layer split `1,1`, two MPTCP
subflows, Q8_0 K/V, 4096 context, batch/ubatch 512, all layers on GPU, flash
attention, direct I/O, seed 1234, temperature 0, no WebUI, and the exact P01
request body.

Execution order was OFF, H01, H02, H02, H01, OFF. Each block had one excluded
warmup and three retained requests: 24 HTTP-200 requests, 18 retained samples,
1129 prompt tokens and 128 generated tokens each, and zero prompt-cache reuse.
Every response matched content SHA-256
`3c9bcc5624f4a1d6558c51be89e2443ffba4b2206d10b632c89b7ef64fdd026f`.

| Variant | Prompt tok/s, mean +/- SD | Generation tok/s, mean +/- SD | End-to-end ms, mean +/- SD |
| --- | ---: | ---: | ---: |
| Feature OFF | 203.8443 +/- 0.1432 | 16.6742 +/- 0.0208 | 13218.958 +/- 9.129 |
| H01 pair | 203.9675 +/- 0.1677 | 16.6548 +/- 0.0325 | 13224.865 +/- 12.133 |
| H02 four-wide | 203.8792 +/- 0.1148 | 16.6671 +/- 0.0252 | 13221.164 +/- 9.789 |

Against H01, H02 generation had a favorable `+0.0739%` point estimate, while
prompt processing was adverse by `-0.0433%`. Against the authoritative
feature-off control, H02 prompt processing was favorable by `+0.0171%`, but
generation was adverse by `-0.0425%` and end-to-end latency was adverse by
`+0.0167%`. Approximate 95% Welch intervals all crossed zero; the H02-minus-OFF
generation interval was `[-0.03323, +0.01905]` tok/s.

The promotion rule required H02 not to have an adverse generation point
estimate against feature OFF. H02 therefore failed without rescue trials. The
result is not a proven regression, speedup, or final G9/G10 non-inferiority
decision. Generation above 30 tok/s remains a stretch objective, not a
baseline or pass/fail fact.

Both USB4 interfaces carried traffic in every block and every retained MPTCP
snapshot reported `subflows_total:2`. Each transient coordinator and worker
stopped with result `success` and status 0. The initial orchestrator invocation
timed out locally after its first model load and before any benchmark request;
that excluded setup was preserved separately and did not enter the matrix.

## Provenance, evidence, and rollback

The candidate was target-native. No donor patch, donor expression, GPL
llama-ai code, CachyLLama code, new dependency, WebUI, remote, model mutation,
or deployment promotion entered HaloFPX. The rejected runtime patch remains
only as node-local experiment evidence; it is absent from the implementation
worktree.

Sanitized evidence is retained at:

- nimo-1 `/var/tmp/halofpx-h02-evidence-nimo1-20260720`, manifest SHA-256
  `63b27b0fa86d009ef55619d9d21c40034a59ecabf176f987185b884227181981`;
- nimo-1 bundle SHA-256
  `275905c9d303bea2ff8e22bc8fe251245ba342ee3533a7fc5952a1ee94853fe1`;
- nimo-2 `/var/tmp/halofpx-h02-evidence-nimo2-20260720`, manifest SHA-256
  `9ad5fe88d11ebea40b932654f76187fb1e5fab9304548a1a7357c39e4bd86fe8`;
- nimo-2 bundle SHA-256
  `39ce3541286974fc10ca6bad5cd7224570bc324e49fb83bc1233698d87c7f7e7`.

The preserved nimo-2 RPC worker was restored first, followed by the nimo-1
server. Both are active and enabled with zero restarts and their original
binary hashes. Nimo-1 health is HTTP 200 and the known-good connection again
reports two MPTCP subflows.

The next performance milestone should move to a different measured bottleneck;
neither loader width nor small-command RPC coalescing has provided a promotable
gain. Final G9/G10 volume remains deferred until a positive candidate or final
acceptance boundary.
