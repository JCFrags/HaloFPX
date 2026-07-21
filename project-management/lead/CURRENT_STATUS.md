# Current Project-Lead Status

Verified: 2026-07-20 22:37 PDT

## Overall state

The project is active and materially progressing. The current worker remains
suitable as the primary implementation owner. Earlier excessive test expansion
has been corrected by steering; recent work uses bounded kill gates and removes
slower candidates.

## Repository

- Implementation: `C:\Users\britt\Documents\HaloFPX`
- Branch: `codex/integration-base-61f2f2d`
- Locked ROCmFPX base: `61f2f2d7bc4955e9bca821095ef69125837133b5`
- Latest verified commit: `522dd90cb52428db76d38f473e0b956774e75a23`
- Remote count: zero
- Worktree state: L10 ADR draft in progress; no runtime implementation yet observed
- Current work: L10 default-off authenticated exact-key operational cache canary

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
- The P13 private MMVQ micro-canary is correct and locally faster, but its
  estimated full-token contribution is only about 0.1%. It was retained as a
  default-off exact-shape proof at `ea49690a` and its product integration lane
  was correctly closed rather than overstated.
- P14 RPC row split preserved exact output but changed prompt throughput by only
  about +0.046% and generation by about -0.092%. It was correctly rejected and
  committed at `522dd90c`; production recovered to HTTP 200 with zero restarts.

## Lead decision

No steering is warranted at this checkpoint. The worker closed the exhausted
low-yield performance lane and selected the highest-value product gap: replacing
client-supplied manifest handles with private authenticated exact-key discovery
and writeback under a distinct default-off canary. This directly advances the
usable persistent-cache objective while retaining bounded generation-one scope.
