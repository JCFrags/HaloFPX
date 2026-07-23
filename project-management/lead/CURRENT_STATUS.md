# Current Project-Lead Status

Verified: 2026-07-23 11:24 PDT

## Overall state

The project is active and materially progressing. The current worker remains
suitable as the primary implementation owner. Earlier excessive test expansion
has been corrected by steering; recent work uses bounded kill gates and removes
slower candidates.

Project-lead monitoring is event-driven only. The 30-minute heartbeat was
deleted. Because a worker final response does not itself inject an event into
the manager task, workers must now send their boundary report directly to the
Project Lead task before ending their own turn. No periodic polling remains.

## Repository

- Implementation: `C:\Users\britt\Documents\HaloFPX`
- Branch: `codex/integration-base-61f2f2d`
- Locked ROCmFPX base: `61f2f2d7bc4955e9bca821095ef69125837133b5`
- Latest verified commit: `851dc6f1af55c856532a5908516ebed9a5679891`
- Remote count: zero
- Worktree state: clean at the reviewed terminal L21 closeout
- Primary worker: fresh task `019f83a3-9498-76c3-9398-be80344854ae`
- Prior worker: idle preserved handoff task
  `019f7377-5d73-7ca1-a83c-a0163f7d4780`
- Current work: L21 passed. Its closed manifest/evidence controller survived a
  real early-allocation refusal, retained mandatory PID/InvocationID/cursor,
  journal, disk, archive, production-equality, and cleanup evidence, passed 13
  focused tests, and was independently accepted. Production remained unchanged.
  L22 preflight verified the exact primary artifact and production authority,
  but two disposable 15M-fixture configurations failed before production
  mutation: Q8_0 KV is structurally incompatible with its 48-wide heads, and
  F16 KV reached a flash-attention abort. Production and the primary artifact
  remained untouched. One narrow no-production compatible-fixture
  qualification is authorized; kernel debugging and broad fixture exploration
  are not.
- The authorized fallback fixture then passed the single-residency smoke and
  exact three-residency lifecycle with F16 KV and flash attention off, explicitly
  without representing primary-model kernel performance. Independent review
  nevertheless blocked production: the controller validated manifest
  `child_argv` but did not bind the actual `maintenance_command` passed to
  `Popen` before shutdown. Production remained untouched. A narrow
  no-production exact-argv/evidence-path binding repair and re-review is now
  authorized.

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
- L15 consumed one guarded transition but stopped before RPC connection because
  the nimo-2 expected-channel key was created mode 0644. No model load, state
  operation, object, or result occurred. Worker-first rollback restored the
  standard production deployment at nimo-2 worker PID 1291141 and nimo-1
  coordinator PID 2125672/HTTP 200, both NRestarts=0. The reviewed terminal
  closeout is `0db5a561`; no primary cache claim is admitted.
- L16 proved the secure binary-stdin key gate and exact HFXCAP2 readiness, then
  failed during the first model load when current placement requested one
  159,231,007,232-byte RPC0 buffer from a worker with 133,143,986,176 bytes
  total. No prompt/state/object result exists. Production recovered worker-first
  at nimo-2 PID 1305879 and nimo-1 PID 2144857/HTTP 200, both NRestarts=0.
  Reviewed terminal closeout `20f19a2d` narrowly attributes this to current
  placement, not aggregate capacity or model incompatibility.
- L17 makes `RPC0,ROCm0` executable pre-allocation authority through a shared
  loader/probe resolver. Its focused negative matrix, isolated real RPC probe,
  and two exact-output tiny-model loads passed with nonzero allocations on both
  devices. Independent re-review found no remaining P1/P2 issue. Production was
  never mutated and remains healthy at nimo-1 PID 2144857/HTTP 200 and nimo-2
  PID 1305879, both with `NRestarts=0`. Commit `730e9633` is clean. L17 does not
  prove the 159.9 GB artifact's allocation shapes or capacity fit.
- L18 uses the real architecture loader in metadata-only `no_alloc` mode and
  accounts for all 809/809 source tensors with zero unknown or unaccounted.
  Planned requests are 80,950,550,528 bytes on RPC0, 78,280,456,704 bytes on
  ROCm0 device memory, and 633,802,752 bytes in the ROCm host group. With
  simulated context/compute, a 10% fragmentation assumption, and a fixed 16 GiB
  reserve, margins remain 26,500,867,072 bytes on RPC0 and 28,406,681,241 bytes
  on ROCm0. The real loader places `output.weight` on RPC0, overriding L17's
  resolver-only prediction. No primary weights were allocated, production was
  not mutated, cleanup completed, independent review found no P1/P2 issue, and
  commit `93c61eadd` is clean.
- L19 correctly stopped before mutation after independent review found two P1
  execution-contract defects and one P2 evidence defect: six process modes
  implied six material loads rather than one; controller key/child/cleanup
  authority remained L16-specific; and allocation-failure evidence would be
  collected too late. No primary load, inference, cache operation, key
  provisioning, unit transition, or disposable listener occurred. The exact
  artifact and current production authority were revalidated, focused tests
  passed 52/52, production stayed at the known-good PIDs with zero restarts,
  and terminal closeout `7cb42be0` is clean.
- L20 proves on a disposable two-host 15M model that the complete current
  lifecycle needs three material residencies: capture/uninterrupted plus clean
  cold; post-worker-restart restore plus corrupt/mismatch cold fallbacks; and a
  feature-off cold control. All continuation hashes matched, the 1,129-token
  prompt chunked 512/512/104, and state windows carried zero legacy tensor-page
  transfers. The controller candidate was not retained because it lacked a
  real early allocation-refusal evidence case, did not own all disposable
  paths, could lose InvocationID authority after unit collection, treated some
  evidence failures as nonfatal, and lacked a production-before snapshot. All
  disposable resources were removed, production remained unchanged, and the
  docs-only terminal closeout is clean at `e2edc4b3`.
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

Accept L20's three-residency lifecycle result but reject its controller
candidate exactly as reviewed. Open L21 only to close the five concrete
execution-contract defects: complete manifest ownership, real early allocation
refusal capture, fail-closed journal authority after unit collection, fatal
evidence-command failures, and a bound production-before snapshot. Reuse the
already proven three-residency small-model lifecycle; do not expand its semantic
matrix. L21 must remain no-production and may not read/load the primary artifact.
Only a later lead decision may authorize a three-residency primary canary.
