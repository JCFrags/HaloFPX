# Current Project-Lead Status

Verified: 2026-07-20 22:00 PDT

## Overall state

The project is active and materially progressing. The current worker remains
suitable as the primary implementation owner. Earlier excessive test expansion
has been corrected by steering; recent work uses bounded kill gates and removes
slower candidates.

## Repository

- Implementation: `C:\Users\britt\Documents\HaloFPX`
- Branch: `codex/integration-base-61f2f2d`
- Locked ROCmFPX base: `61f2f2d7bc4955e9bca821095ef69125837133b5`
- Latest verified commit: `45ae40532391ccc2ff121624b865ea366fda1036`
- Remote count: zero
- Current work: P13 private Q6 owned-expert canary and closure record

## Product progress

- A real default-off server canary has demonstrated miss, durable publication,
  restart, authenticated hit, corruption rejection, and equal recomputation.
- This remains an explicit-handle laboratory path using a small fixture; it is
  not yet production persistence or a 160 GB model cache-state workload.
- The exact 160 GB primary model is pinned and repeatedly benchmarked.

## Performance truth

- Matched feature-off baseline is roughly 203.8 prompt tokens/s and 16.65
  generation tokens/s for the exact primary workload.
- No accepted full-model speedup exists yet.
- Several rank-local/fused/placement candidates were correctly rejected and
  removed after 0.3–4.3% matched slowdowns.
- Profiling shows alternating rank work and roughly half-duty GPUs; aggregate
  USB4 bandwidth is not the present limiting resource.
- The current private MMVQ micro-canary is correct and locally faster, but its
  estimated full-token contribution is only about 0.1%; the worker is correctly
  closing integration rather than overstating it.

## Lead decision

No steering is warranted at this checkpoint. The worker independently reached
the correct kill decision, preserved the useful proof, and avoided reopening a
large RPC/model seam for a negligible projected benefit. Observe through the
current milestone close, then reassess the next chosen product/performance lane.
