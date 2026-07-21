# Current Project-Lead Status

Verified: 2026-07-21 04:24 PDT

## Overall state

The project is active and materially progressing. The current worker remains
suitable as the primary implementation owner. Earlier excessive test expansion
has been corrected by steering; recent work uses bounded kill gates and removes
slower candidates.

## Repository

- Implementation: `C:\Users\britt\Documents\HaloFPX`
- Branch: `codex/integration-base-61f2f2d`
- Locked ROCmFPX base: `61f2f2d7bc4955e9bca821095ef69125837133b5`
- Latest verified commit: `a496492c590570e88ac83b511f37e66c52197816`
- Remote count: zero
- Worktree state: clean at the committed L10c boundary
- Primary worker: fresh task `019f83a3-9498-76c3-9398-be80344854ae`
- Prior worker: idle preserved handoff task
  `019f7377-5d73-7ca1-a83c-a0163f7d4780`
- Current work: worker idle at accepted L14 boundary; one L15 controller-managed
  primary-model canary is authorized using application-level CAPS readiness

## Product progress

- L10c is the first normal-completion, client-handle-free default-off server
  canary to demonstrate authenticated exact-key cold miss, prompt-boundary
  publication, process restart, and hit. The tiny-model proof reduced prompt
  processing from 11 tokens cold to 1 restored token with exact continuation.
- A changed prompt recomputed cold without replacing the original entry;
  reserve exhaustion published nothing; corrupt/incompatible state remains a
  cold fallback. This remains a single-entry generation-one laboratory canary,
  not production persistence or a 160 GB model cache-state workload.
- L10a now authenticates the fixed generation-one anchor and automatically
  derives its selected manifest without trusting directory names. Publish,
  reopen, automatic selection, exact restore, wrong-scope miss, and
  corrupted-anchor miss passed on nimo-2. Server runtime lookup/writeback is the
  current next gate rather than a completed claim.
- L10b derives an opaque authenticated checkpoint lineage from private scope,
  closed compatibility, exact fixed-width canonical tokens, and bounded target
  topology/profile. Focused and inherited controls passed on nimo-2, and the
  ordinary server binary contained no exact-session marker. This is committed
  as `d7950c43`; it still has no normal request/runtime behavior by itself.
- L10c passed 7/7 focused tests in both gate-on and gate-off builds. Thirty
  balanced tiny-model feature-off pairs had identical outputs and non-worse
  point estimates (+2.35% prompt, +0.75% paired generation), but the broad
  confidence intervals mean this is not final G9/G10 evidence.
- L10d adds a fixed authenticated catalog of two to eight independent immutable
  generation-one entries while partitioning the existing quota. Two prompts
  each processed 11 tokens cold and 1 after restart; a third prompt at capacity
  processed cold without tree mutation. Focused Linux tests passed 8/8,
  independent review accepted the repaired authority, and production remained
  active with zero restarts. Commit `6862ffb9` is clean.
- L11 proves the current sequence serializer is globally complete only by
  transferring worker-resident KV pages through RPC: four GET payloads totaling
  41,472 bytes during capture and four SET payloads totaling 41,472 bytes during
  restore for the small fixture. The worker has no rank-local serializer,
  persistent writer, readiness authority, or commit-live participant. The
  reviewed documentation-only blocker is committed cleanly as `78a102ac`.
- L12 implements the default-off worker-local RPC state protocol and proves it
  on a disposable two-host small model. Uninterrupted, restart-restored, and
  cold suffixes were exact. Missing/corrupt objects and plan/topology mismatch
  rebuilt cold. Capture and restore state windows contained zero legacy
  GET_TENSOR/SET_TENSOR payload transfers. Independent adversarial review
  accepted the implementation; commits `6444d1e1` and `51922809` are clean.
- Live production authority was reverified after L12: nimo-1 is the current
  coordinator on port 8081 (PID 2053029) and nimo-2 is the RPC worker on port
  50052 (PID 1186396). Earlier project records using the opposite orientation
  describe disposable or prior deployments and are not current runtime truth.
- L13 did not reach capture. Attempt one asserted because the 1,128-token saved
  prefix exceeded `n_batch=512`; its chunking correction is committed but
  runtime-unqualified. Attempt two stopped nimo-2's production worker before
  nimo-1's coordinator due to a wrong-host command, causing the coordinator to
  abort. The stop gate fired, no state object was published, and no L13
  correctness/performance claim is admitted. Production recovered to HTTP 200:
  nimo-1 coordinator PID 2068256 and nimo-2 worker PID 1247685, both with
  `NRestarts=0`. The reviewed negative result is commit `519a4400`.
- L13R prerequisites succeeded: the actual canary processed the 1,128-token
  saved prefix in three chunks at `n_batch=512`, and the host-bound transition
  controller passed 19 focused tests and independent review. The single retry
  then failed before model load or state operations because TCP listener
  visibility preceded RPC application readiness. The enforced rollback restored
  nimo-2 worker PID 1275544 and nimo-1 coordinator PID 2093167/HTTP 200, both
  `NRestarts=0`. No state object or model result exists. Final reviewed closeout
  is `aa3c2cf6`.
- L14 replaces socket/listener readiness with exact HELLO plus runtime-bound
  `HFXCAP2` admission. The probe binds RPC/state versions, command mask and
  ordinal, limits, rank/world, key generation, and channel identity. Thirty
  focused tests passed, including delayed listener-first, timeout, runtime-off,
  malformed and wrong-capability cases. A real disposable ROCm worker admitted
  in 1.225 ms with zero state operations while production remained unchanged.
  Implementation `b688680e`, reviewed closeout `a496492c`.
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

L14 is accepted and resolves the startup-readiness blocker without touching
production. One L15 primary-model canary is now justified: use the current exact
source, executable transition controller, long-prompt batching proof, and exact
CAPS readiness gate. The canary remains bounded to the pinned model/request,
success-path capture/cold/restart-restore, one corrupt-or-missing object fallback,
one identity mismatch fallback, and a small runtime-off cold control. Any gate
failure triggers controller rollback and ends the turn. This is not production
cache enablement or final G9/G10 acceptance.
