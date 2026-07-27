# Current Project-Lead Status

Verified: 2026-07-27 18:00 PDT

## Overall state

The project is active and materially progressing. The current worker remains
suitable as the primary implementation owner. Earlier excessive test expansion
has been corrected by steering; recent work uses bounded kill gates and removes
slower candidates.

Project-lead monitoring is event-driven only. The 30-minute heartbeat was
deleted. Because a worker final response does not itself inject an event into
the manager task, workers must now send their boundary report directly to the
Project Lead task before ending their own turn. No periodic polling remains.

L36 closed NOT PROMOTED at
`0b4c00c5f90cf245ceee769619886f295bf4e5d2`. Both attempts were invalidated by
a proven post-free `llama_n_batch(run_ctx)` result read; no model
interpretation is admitted and no further primary repeat is authorized. L37
is active as no-production work to correct result lifetime authority and
authenticate remaining mutable graph inputs between replay and logits. Two
independent read-only specialists are auditing graph-input coverage and
  RPC/ROCm determinism.
- L37 passed at `a1bee312ca79f3087cd5bfcd327e9f11b2be72d2`.
  Result emission is now lifetime-safe, authenticated, durable, and verified
  before context destruction; focused tests reject the prior invalid values.
  Closed graph-input diagnostics cover admitted mutable classes, while RPC
  internal tensor/copy authority and opaque execution state remain explicit
  gaps. Two independent audits converged on numerical input bytes/bindings,
  scheduler copies, RPC graph reconstruction, and FA read-set extent. L38 is
  active as a no-primary/no-production replay-exec-v2 contract plus one
  synthetic Q8_0 FA poison-span discriminator.
- L38 closed NOT PROMOTED at
  `169d81ad84167fcd5449b5dc99126bd861446087`. Independent review rejected and
  removed the candidate because it did not instrument the real scheduler copy
  loop or RPC client/server execution authority. Its isolated Q8_0 FA poison
  test produced equal output despite different bytes outside selected spans,
  narrowly demoting that hypothesis. L39 is active as direct default-off
  instrumentation in scheduler split/copy execution and RPC
  serialization/reconstruction, qualified by one combined synthetic graph.
  No primary run or production mutation is authorized.
- L39 closed NOT PROMOTED at
  `0658a272d25ee660055143904aa47a2d76dc2d19`. The combined fixture proved
  deterministic compute/recompute only after replaying identical mutable
  inputs, but independent review rejected the implementation because core RPC
  authentication, server-owned reconstruction authority, canonical records,
  mutable-census closure, and required negative coverage were incomplete. The
  candidate was removed, leaving accepted runtime source byte-identical to
  L38. L40 is active as a narrower RPC-only foundation: negotiated capability,
  authenticated client/server graph equality, and bound recompute lineage.
  Scheduler-copy and mutable-census work are deliberately deferred.
- L40 passed at `53f414dfc5a8f9873ad9961f541eb41cf6dc2aae`.
  It establishes the accepted RPC trust boundary: negotiated default-off
  capability, canonical client/server graph authority, authenticated
  server-owned reconstruction receipts, and bound compute/recompute lineage.
  L41 is active only for the next independent layer, the real scheduler
  split/copy execution authority. Mutable-input census, primary testing, and
  production mutation remain unauthorized.
- L41 closed NOT PROMOTED at
  `ba0cbd51634cd58496d35cf615dfdae32a367269`. It reached the real ordinary and
  expert-partial scheduler copy seams, but review rejected the evidence
  contract: no inspectable authenticated event stream, incomplete
  destination/view authority, missing focused refusal cases, and insufficient
  exact-output assertions. The candidate was removed. L42 is authorized only
  to close those four findings with exact exported evidence and targeted
  qualification.
- L42 passed at `d0d74ff55d8b063ab73911ae95516512177c824d`.
  The accepted scheduler layer now authenticates exact split, copy-map,
  ordinary-copy, and expert-partial execution authority with externally
  reconstructable transcripts and exact destination/view/range evidence.
  L43 is active only for closed structural mutable-input classification,
  SET/SET_HASH server-applied authority, and a complete execution census bound
  to the accepted L40/L42 layers.
- L43 closed NOT PROMOTED at
  `aba0f78d07c824c3bcdbcb5ffbdc26e174cda3bf`. The candidate demonstrated real
  SET/SET_HASH and mutable-census behavior, but review rejected process-global
  pointer registration without admitted-session isolation and missing
  real-handler negative injection. Candidate code was removed; L40 and L42
  remain unchanged. Per user direction, implementation is paused for OS
  migration and no L44 is open.
- OS-migration restoration verification passed on 2026-07-26. Both repository
  identities match the handoff, HaloFPX is clean, and the intentionally
  retained project evidence remains preserved. Production was reconciled
  read-only at the exact unchanged system services: nimo-2 PID `1535639`,
  port `50052`, `NRestarts=0`; nimo-1 PID `2356329`, port `8081`, HTTP `200`,
  `NRestarts=0`. L44 is now active only for admitted-session mutable authority
  and real-handler refusal injection.
- L44 passed at `5f69d5cdaf8eb51283dd750c1fd8ca869fcf4d66`.
  It replaces process-global mutable registration with admitted session handles,
  proves concurrent/foreign/stale/closed isolation, authenticates real
  SET/SET_HASH server-applied material, and exercises the required refusal
  branches through actual RPC handlers. L45 is active as the single exact
  primary two-fresh-residency one-token discriminator combining L40, L42, and
  L44 authority. No retry or performance work is authorized.
- L45 stopped before mutation because the real distributed-state canary and
  runner do not yet call the accepted L42/L44 admission APIs. Enabling L44
  without a committed session would fail compute; leaving it off would make the
  intended discriminator incomplete. No model or production state was touched.
  L46 is authorized as the no-primary integration prerequisite connecting
  structural source-owned registration, scheduler/session lifetime, and closed
  result evidence to the real replay path.
- L46 closed NOT PROMOTED at
  `0d655b54d77929dafc2a7efe05f25a94d6c6ca0d`. Source audit proved L42/L44
  cannot currently compose for the real mixed local/RPC graph: L44 requires
  authority for leaves its API rejects, the scheduler exposes no precompute RPC
  split/copy bridge, and L44 needs admission before L42's transcript exists.
  L47 is active as an ADR-first correction for mixed ownership, prepared
  scheduler admission, and bounded per-execution arm/finalize/abort lifecycle.
  No primary or production action is authorized.
- L47 closed NOT PROMOTED at
  `d9aabb66822660b393cc8f14501ea5552471c6d9`. Its ADR-0048 composition
  successfully qualified the real disposable multi-chunk capture/restore path,
  including exact token `4245`, composed per-execution evidence, and zero
  legacy state transfer. Review rejected promotion because the closed primary
  runner/controller could not securely enable, verify, retain, or require that
  result, and warmup diagnostics were not clean. Candidate source was removed.
  L48 is active only for the protected-key, argv-safe runner/controller binding,
  explicit warmup/unarmed lifecycle, verifier, and reconstruction of the
  already-qualified composition.
- L48 closed NOT PROMOTED at
  `591603ff0982fe684fd67c45f40898f4332fac88`. Its runner/controller, protected
  key, manifest, verifier, builds, 50 focused tests, and pre-runtime review all
  passed. The sole stories15M session was killed at 30 seconds because the real
  120-second HFXCAP2 readiness operation was incorrectly assigned to the
  generic command transport class. No model result or production mutation was
  admitted. L49 is active only to add a closed readiness transport class with
  correctly nested deadlines and requalify the accepted L48 shape once.
- L49 closed NOT PROMOTED at
  `e606f62cb19063ceb7bfdbe9dff979ea0544abf0`. The readiness class behaved
  correctly, but its exact disposable worker exited before opening the port.
  A Project Lead read-only journal recovery identified the retained cause
  without reproduction: `unknown device: ROCm0`, no accelerator devices, CPU
  only. L50 is active only to correct the proven ROCm build/device admission,
  make early-exit journal authority mandatory, and requalify the same controller
  shape once.
- L50 closed NOT PROMOTED at
  `8fa511036ef9fc633b00fe1148ae0b032457f495`. It proved and corrected the
  exact `GGML_HIP=OFF` cause, froze an ROCm/gfx1151 build, and passed the real
  device/HFXCAP2 gate. The sole stories run then failed during diagnostic
  warmup before prompt execution, and its unit collection exposed InvocationID
  and journal-cursor evidence defects. L51 is active only to source-localize and
  correct unarmed warmup lifecycle plus exact user-unit exit/journal evidence,
  then requalify once.
- L51 closed NOT PROMOTED at
  `1746c15c9688cb068751ab40619bb0637cff1b3a`. Source audit and focused
  qualification closed the global-env warmup defect with explicit
  per-execution arm/disarm, and repaired InvocationID/journal/exit evidence.
  The sole controller session stopped before model runtime because it copied
  the device receipt before creating the remote evidence directory. L52 is
  active only to correct that closed evidence-path ordering and perform the
  deferred stories15M qualification once.
- L52 closed NOT PROMOTED at
  `d236e74d2b2c3df96d88ef4cce5269d1baf3f24a`. Evidence directory admission
  and atomic receipt publication passed, followed by device, readiness, and
  placement gates. Warmup still returned decode `-3`. The retained worker
  journal proves L40 server graph preparation/execution succeeded, narrowing
  the failure to the coordinator scheduler/composition status path. L53 is
  active only to localize and correct that explicit unarmed scheduler lifecycle
  and perform the deferred stories run once.
- L53 closed NOT PROMOTED at
  `20af537f0d36d9de3877af860e4f24d89d7e2641`. Source audit found a genuine
  contradiction: source says common warmup is unarmed and L42/L44-gated, while
  retained runtime shows an authenticated RPC execution before the canary can
  arm. Existing evidence cannot distinguish binary/source lineage mismatch
  from an uncovered arm transition. L54 is active only as one warmup-only
  discriminator binding exact source/binary provenance and recording the
  pending/arm/scheduler/RPC refusal transitions. No semantic fix is authorized.
- L54 closed NOT PROMOTED at
  `0578c9ce3e58ef832af734ab4a9c0e0ddae94f26` without consuming runtime. It
  corrected the retained chronology: common warmup succeeded on the ordinary
  RPC graph; authenticated sequence `1`/UID `27` was the first armed 512-token
  prompt chunk, whose remote graph executed before the coordinator returned
  `-3`. A warmup-only run could not reach that failure. L55 is active only to
  record exact provenance and the client/scheduler/L42/L44 status path through
  that first chunk, then stop.
- L55 passed as a bounded diagnostic at
  `51e87b0c011eb3c7dc5b170bd8f64048bccd0853`. Exact source/binary provenance
  and one first-chunk run prove the server prepared and executed sequence `1`,
  UID `27`, digest `0717...`, then the coordinator first failed in
  `l40_graph_result_reconcile`. No later execution occurred. L56 is active only
  to distinguish the exact L40 receipt condition—absence, identity/digest/tag,
  replay/consumption, or backend-status mismatch—without changing behavior.
- L56 passed as a bounded diagnostic at
  `8af226d675d9ae287d5d2bddd849f9920507d9ba`. Source and retained evidence
  decisively identify `graph_uid_mismatch`: reconciliation compared parent
  scheduler graph UID `26` to the valid RPC split UID `27` for backend ordinal
  `0`, sequence `1`. L57 is active only to bind explicit parent/split UID
  mapping, retain fail-closed lineage, and perform one complete stories15M
  controller qualification.
- L57 closed NOT PROMOTED at
  `0026d5243c6108659fa53ce9185af9de0d6ec857`. Its independently reviewed
  parent/split identity correction is retained and focused qualification
  passed. In the sole stories15M run, the first armed graph reached the RPC
  worker, which logged ordinary graph execution, but the coordinator received
  no usable response and failed with `Remote RPC server crashed or returned
  malformed response`; cleanup then aborted while freeing the RPC buffer.
  Existing evidence cannot distinguish worker failure, missing/truncated
  response, framing/size mismatch, socket EOF, or client validation refusal.
  L58 is active only as a no-primary/no-production authenticated discriminator
  at that exact request/response boundary. It may run one small RPC fixture and
  one first armed stories chunk, but no cache/restore matrix or semantic fix.
- L58 closed NOT PROMOTED at
  `e561b56ffb0edc4ffbc38b1c5426722146d32b37`. Its default-off RPC
  response-boundary instrumentation, verifier, focused tests, and pre-runtime
  review passed, but the sole stories run remained ambiguous because the
  success-only harvester skipped both diagnostic streams after the canary
  failed and controller cleanup then deleted the remote roots. The worker again
  logged graph execution and the coordinator again reported a malformed or
  crashed response, but no causal classification is admitted. L59 is active
  only to make failure-path stream harvesting durable before cleanup, qualify
  that ordering with injected failures, and then consume one deferred
  first-chunk discriminator. No cache matrix or semantic correction is
  authorized.
- L59 failure-path harvesting passed focused qualification and a real injected
  unit failure, including retention of an authenticated partial worker stream
  with explicit missing-client status. Its first controller attempt stopped
  before stories/model runtime because POSIX directory-fd `fsync` was applied
  to a Windows evidence directory and raised `PermissionError`. One narrow
  cross-platform durability correction and one replacement discriminator
  attempt are authorized; the admission failure remains immutable evidence.
- L59 closed NOT PROMOTED at
  `d80b792747a43a9ee2f6faacad6a9b5dfe17d331`. The accepted foundation now
  durably harvests authenticated client/worker response prefixes on failure
  before cleanup and records truthful Windows/POSIX durability. The replacement
  attempt still stopped before model launch because systemd retained the fixed
  transient device-gate unit name from the earlier failure. L60 is active only
  to enforce pre/post-launch not-found unit authority and then execute the
  deferred first-chunk response discriminator once.
- L60 closed NOT PROMOTED at
  `ade0bc86a9f7659a67239865641d9a1211f8744f`. Its transient-unit
  absence/reconciliation guard passed and is retained. The sole model run
  reached the first armed graph, but response evidence remained unavailable:
  the worker harvester was invoked through a nimo-2-only source path from
  nimo-1, while the client stream was absent. L61 is active only to bind and
  exercise exact host-local harvesters and both stream-creation paths before
  one deferred first-chunk discriminator.
- L61 closed NOT PROMOTED at
  `b74170ec208f17001d12e8bb5278f67f75bb38ba`. Its host-bound two-stream
  harvesting and runtime client prelaunch probe passed and are retained. The
  sole stories run proves the armed first-chunk failure occurs before
  `RPC_CMD_HALOFPX_GRAPH_AUTH_EXECUTE`; the earlier worker graph was ordinary
  warmup and the later malformed-response text came from teardown. L62 is
  active only to authenticate the preceding L40/L42/L44 admission and
  graph-compute/recompute client decision, then run one first-chunk
  discriminator.

## Repository

- Implementation: `C:\Users\britt\Documents\HaloFPX`
- Branch: `codex/integration-base-61f2f2d`
- Locked ROCmFPX base: `61f2f2d7bc4955e9bca821095ef69125837133b5`
- Latest verified commit: `b74170ec208f17001d12e8bb5278f67f75bb38ba`
- Remote count: zero
- Worktree state: clean at the reviewed terminal L61 closeout
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
- L22 is terminal NOT PROMOTED. The single exact 160 GB attempt proved exact
  capture, cold, missing-object fallback, plan-mismatch fallback, and mode-off
  continuations, with rank-local worker state and zero legacy state-page
  GET/SET. True worker-restart restore produced a different continuation and
  failed closed. Production recovered worker-first and is healthy. L23 is open
  only for offline/source/disposable diagnosis of the earliest restored-state
  divergence; it authorizes no primary load or production mutation.
- L23 added default-off authenticated capture/stage/live-apply worker digests.
  The disposable lifecycle remained exact with equal worker aggregates, but
  retained L22 evidence cannot distinguish worker application from
  coordinator/model-specific incompleteness. L24 is authorized for one
  primary-model load and only capture plus true worker-restart restore with
  minimum deterministic output. It is diagnostic, not a cache retry or
  promotion.
- L24 capture completed, but a controller-owned SSH readiness probe hung.
  Terminating the disposable child triggered emergency recovery, whose first
  SSH probe also hung. Both production services were temporarily inactive.
  The bounded stuck subprocess was cleared and the existing controller restored
  nimo-2 worker first (PID 1415055/50052), then nimo-1 coordinator (PID
  2236922/8081/HTTP 200), both `NRestarts=0`. No retry is authorized. L24 is
  closing NOT PROMOTED with the diagnostic and recovery evidence preserved.
- L24 is now reviewed and committed NOT PROMOTED. Capture retained the primary
  worker aggregate, but no authenticated reference token, stage, apply, or
  restore result exists. L25 is open only to give every controller-owned SSH
  subprocess a local process-group deadline, preserve typed timeout evidence,
  keep recovery live without orphan processes, and flush/authenticate capture
  evidence before handoff. Production must remain continuously unchanged.
- L25 passed 79 focused tests and independent review. All controller and child
  SSH now use locally enforced process-group deadlines with typed fsynced
  evidence, bounded escalation/reaping, no ambiguous mutation retry, and
  recovery continuation. Capture output is flushed, authenticated, and durable
  before restart. L26 is authorized for one primary load and only the
  capture/restart/restore one-token discriminator that L24 could not complete.
- L26 is terminal after the single authorized run. Capture and the disposable
  worker restart occurred, but the coordinator aborted while creating its
  post-restart context after RPC reported a crashed or malformed remote
  response. No retry occurred. Recovery restored nimo-2 worker first (PID
  1422619/50052), then nimo-1 coordinator (PID 2248156/8081/HTTP 200), with
  exact unit/cgroup/command ownership and `NRestarts=0`. The worker is closing
  L26 NOT PROMOTED with evidence and independent review.
- L26 is reviewed and committed NOT PROMOTED. Its capture evidence is valid,
  but no stage/apply result exists. L27 is open only to test whether restarting
  an RPC worker invalidates the server-side model/buffer allocations held by a
  still-resident coordinator model. It will compare same-residency failure
  against a fresh coordinator/model residency using a disposable small model,
  with production continuously unchanged.
- L27 passed and proved the lifecycle boundary: a worker restart destroys
  process-local RPC buffer/tensor/graph authority, so an old coordinator model
  residency cannot continue even after fresh CAPS. A new coordinator/model
  residency against the new worker restored the disposable state exactly. L28
  is open only to wire worker epoch and model-allocation epoch authority into
  the real fresh-residency runner and qualify that two-residency lifecycle on
  the disposable model.
- L28 passed 78 focused tests and independent review. The executable runner now
  enforces two fresh model residencies, authenticated capture-object/epoch-A
  lineage, coordinator-A-before-worker-A shutdown, distinct worker B, complete
  model-B readiness and current epoch revalidation before restore authorization.
  The disposable capture/restore was exact. L29 is authorized for one primary
  transition using only this two-residency, one-token discriminator.
- L29 completed the single two-residency primary transition and reached output
  comparison. Capture and restored token/text hashes differ, so the run failed
  closed and will not be repeated. Production recovered worker-first and is
  healthy at nimo-2 PID 1454894/50052 and nimo-1 PID 2283857/8081/HTTP 200,
  both `NRestarts=0`. Final boundary localization awaits the committed
  capture/stage/apply worker and coordinator digest closeout.
- A closeout alarm that both units were inactive was resolved without mutation:
  the probe queried the user systemd manager for system units. `/proc` cgroups
  and explicit system-manager queries prove both exact production units are
  loaded active/running with the PIDs above, correct commands/listeners,
  HTTP 200, and `NRestarts=0`. L29 closeout must retain this wrong-scope
  evidence and bind future production probes explicitly to system scope.
- L29 is reviewed and committed NOT PROMOTED. Capture and validated stage agree
  exactly on all 64 components and 2,454,528 bytes, while the live post-apply
  aggregate differs; coordinator control/local/manifest digests are identical.
  This localizes the first retained defect to stage-to-live worker application
  or restart layout. L30 is open only for component-level digest/range,
  alias/view/stride/order, RPC-offset, and synchronization diagnosis using no
  primary model and no production mutation.
- L30 passed and identified the source-backed defect: live apply converted Q8_0
  storage bytes to scalar elements without multiplying by the block size, so a
  1,088-byte component restored only 34 bytes. The guarded correction restores
  full block geometry and passed exact Q8 RPC, view, and disposable
  two-residency tests plus independent review. L31 is authorized for one
  corrected primary two-residency, one-token confirmation.
- L31 consumed the single corrected primary transition but the first restored
  token still differed from capture, so it is terminal NOT PROMOTED with no
  retry. Production recovered worker-first and is healthy at nimo-2 PID
  1468887/50052 and nimo-1 PID 2304428/8081/HTTP 200, both `NRestarts=0`.
  The next decision is intentionally deferred until independent review freezes
  the authenticated component-level capture/stage/apply comparison.
- L31 is reviewed and committed NOT PROMOTED. All 64 worker components now have
  identical authenticated identity/content after live apply and coordinator
  input receipts match, yet the first token differs. L32 is open only to
  recapture the restored live coordinator/context state before generation and
  compare it canonically with the original capture, including control metadata,
  local components, sequence/KV cell state, and architecture-exposed memory.
- L32 passed 96 focused tests and independent review. It added authenticated
  live post-apply coordinator and worker recapture immediately before
  generation. The disposable two-residency lifecycle is exact across all live
  phases. No source-backed coordinator defect is proven without the primary
  workload. Work is intentionally stopped at this clean boundary pending
  explicit user approval; no L33 task has been sent.
- User approval to resume was received. L33 is now active as one exact-primary,
  two-residency, one-token live-state discriminator using the already qualified
  L32 instrumentation. No broad smoke-test matrix, automatic retry, tuning,
  speculative fix, or production-cache promotion is authorized.
- L33 is terminal NOT PROMOTED at
  `83ce2b5a449fa68d7864d8e0d31bf85c8edfc0ed`. All authenticated serialized and
  live-recaptured coordinator/worker state is equal through the adjacent
  pre-generation boundary, but the primary first token still differs. L34 is
  active as a source-only semantic completeness diagnosis focused on final
  prompt-token replay, logits provenance, decode positions, sampler inputs,
  and architecture-specific runtime state. Two independent read-only
  specialist audits are running in parallel; no primary load or production
  mutation is authorized.
- L34 passed at `fc8517ffc473220d74ee27b6eb111d4be7fefd82`.
  Source and focused diagnostics prove both paths replay the final prompt token
  exactly once and freshly generate synchronized logits; generic replay-count,
  stale-logit, sampler-history, recurrent-memory, and state-write synchronization
  explanations are unsupported. Two independent source audits identify
  graph/scheduler history and KV physical-cell/allocator authority as the
  strongest remaining uncovered inputs. L35 is active as one combined
  disposable discriminator and mutable-state coverage census; no primary run
  is authorized.
- L35 passed at `93c3ae313b86aa0bfddd2c5a1a8745223cb256ac`.
  Authenticated disposable evidence shows capture and restore both rebuilt the
  graph, selected identical KV prepare/apply cells and heads, used identical
  attention views/backends/output mapping, and produced identical synchronized
  logits and token. L36 is active as exactly one ordinary primary replay-
  authority discriminator. No canonical-reset variant, matrix, retry, tuning,
  or speculative correction is authorized.
- The first L36 transition was rejected by a narrow harness admission defect
  after both residencies: honest fresh restore reports `n_batch=0`, but the
  combined diagnostics path incorrectly expected 512 because the invariant was
  coupled to `SEMANTIC_DIAGNOSTICS_ONLY`. No replay-authority interpretation
  was admitted. Production recovered exactly to HTTP 200. One corrected L36
  execution is authorized only after focused lifecycle-gate tests, hash refresh,
  and independent review; the failed run must remain separately preserved.

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
