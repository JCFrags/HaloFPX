# Project-Lead Decisions

## 2026-08-12 — merge the first bounded speed slice and retain runtime gates

Accept PR #30 at
`7a36e01a25bd5c27b684b489d9996b4de3afa299` as a default-off,
compile-qualified ROCmFPX generation optimization. Its admitted source removes
only the unused Q8_1 activation-sum reduction for Q2/Q3/Q6/Q8 ROCmFPX MMVQ;
ROCmFP4 and stock types remain on legacy behavior. Both CachyOS Strix nodes
compiled feature-on and feature-off source while the production service
identities and zero-restart counters remained unchanged. This decision makes
no GPU correctness, model-parity, or performance claim, and issue #25 remains
open until those matched target gates pass.

Accept PR #35 at
`167df62ffc8970bc408d72e97ab71a57de4b69d2` as the bounded correction for
the mixed sampled/raw logits row-count crash. Keep issue #28 open for the
coherent output snapshot, candidate/count provenance, explicit unavailable
state, synchronization counter, and matched server latency evidence.

Continue cache completion through issues #26, #32, and #33: atomic two-rank
composition, verified longest exact-token prefix selection with suffix replay,
and compatibility derived from the live inference plan. CachyLlama commit
`6be745998f568e379ea197fcf827baec73ff9940` remains the behavioral cache
reference; its weak storage trust and single-process assumptions are not
accepted implementation authority.

Record PR #31 at `0ba18151438cb0e7279c7c8ae08e152f6f70145b` as closure of issue #16
for the model-general frozen-plan/evidence core. It remains deliberately
execution-unqualified. Route the safe CachyOS fresh-process adapter through
issue #37 rather than reopening or misreporting #16.

## 2026-08-12 — accept bounded restart qualification and begin target performance slices

Decision: accept merged PR #23 at exact merge commit
`aee627bd46de21327c9082f7915818430d38f453` as closure of issue #14's
bounded restart-correctness lane. The accepted claim is a default-off,
exact-key, Linux CPU server fixture across fresh processes for cold
publication, exact restart hit, compatibility miss/cold recomputation, and
same-size corruption miss/cold recomputation with deterministic continuation.

The admitted profile is world size 1, rank 0, ordinary transformer memory,
one slot, and greedy memoryless sampling. It is not a prefix cache, the
run-local `--cache-disk` spill cache, recurrent/hybrid support, coordinated
two-rank state, a production deployment, or target performance evidence.
Issue #26 owns the two-rank restart-safe cache boundary.

Accept merged PR #27 at exact merge commit
`bf420e9f1db4ea4ba1d7c87771b6a4d662b5be67` as a bounded optional digest
provider for the separate run-local SSD prompt cache. When explicitly built
on Linux with OpenSSL 3 Crypto, EVP SHA-256 replaces only the scalar provider;
exact-length and full-file digest gates remain. Filesystem, size, and digest
failures remain authoritative. Only an internal EVP-provider failure reopens
the file and restarts scalar hashing at byte zero. Scalar and EVP hosted rows
passed, but no matched cache-request speedup or target measurement is accepted.

Keep `fewtarius/CachyLLama` commit
`6be745998f568e379ea197fcf827baec73ff9940` as the saved-cache behavior
reference. Do not turn the two merged slices into a claim that CachyLLama's
prefix/tiering/lifecycle feature set or a complete HaloKV product is present.

Begin serious performance work with issue #25 as the active P0 generation
slice. Draft PR #30 is a candidate and remains outside accepted `main` at this
decision. Issues #15, #16, #18, #26, #28, and #29 remain open follow-ons for
prefill measurement, the target A/B harness, cache attribution, two-rank cache
coordination, sampling synchronization, and FFN conversion reuse. Correctness,
feature-off behavior, and matched target evidence remain promotion gates; a
compile or microbenchmark alone is not a model-level performance claim.

The read-only service-health observation at `2026-08-12T23:06:08Z` is accepted
only as a health receipt. It does not replace the broader platform audit or
authorize a production transition.

Reason: issue #14 and the optional digest-provider implementation are now
merged and hosted checks are green. The owner directed the project to finish
the cache boundary promptly and move into real prompt/generation performance
work without overstating cache completeness or local/control-host results.

## 2026-08-12 — pin CachyLlama cache behavior and start measured acceleration

Decision: use the `fewtarius/CachyLLama` component at exact commit
`6be745998f568e379ea197fcf827baec73ff9940` as HaloFPX's cache
behavior/reference authority. This is a requirements and behavior reference
for clean-room use only, not permission to copy source. Any HaloFPX
implementation must remain provenance-reviewed unless an exact file and
license review establishes a compatible import.

Finish the cache-correctness boundary promptly and in this order: first close
issue #14 with fresh-process exact-key reuse, deterministic continuation, and
corruption/compatibility miss-and-recompute behavior; next measure and
accelerate cache verification while preserving every full-file integrity gate;
then advance the separately measured cold prompt-processing and generation
lanes. A faster digest provider is only accepted when it produces identical
digests, retains corruption parity, and improves matched end-to-end cache time.
Do not claim prompt-processing or generation gains from avoided prompt work.

Reason: the owner identified CachyLlama as the specific saved-cache reference
and directed the project to finish that work soon, then begin serious measured
performance work. The sequence preserves fail-closed cache semantics while
moving directly into speed work with bounded kill gates.

## 2026-08-12 — correct target OS, ROCmFPX scope, and active work authority

Decision: the two physical performance targets are Nimo Direct MME3L AMD
Strix Halo systems running CachyOS, not Ubuntu and not the local Windows PC. A
bounded live read-only audit on both nodes at 2026-08-12T20:33:55Z through
20:49:20Z observed kernel `7.1.3-1-cachyos`, ROCm 7.2.4-family packages, Mesa
26.1.4, and Radeon 8060S `gfx1151`. Ubuntu remains a vendor-support and
portability/control lane only.

HaloFPX is model-architecture-general within ROCmFPX-family GGUF artifacts that
the tree can convert, load, execute, and qualify. Primary optimization and
performance claims use admitted ROCmFPX/ROCmFP4 model weights. Conventional
GGUF quants are correctness, quality, rollback, or comparison controls unless
separately admitted. MiniMax Q6 Agent is the largest stress/capacity fixture,
not the product-specific optimization target.

ROCmFPX names the GGUF weight serialization and quantization family. CPU is the
correctness reference; HIP/ROCm and Vulkan are independent accelerated
backends; RPC is the dual-node transport. Runtime K/V-cache types remain a
separate control. Current source exposes Q2, Q3, Q4 dual-scale, Q4_FAST, Q6,
and Q8 family members. Q2 has a CPU path plus selected CUDA/HIP operations and
no Vulkan path; Q3/Q4/Q4_FAST/Q6/Q8 have CPU, CUDA/HIP, and Vulkan paths. Static
support is not a performance or model compatibility claim.

PR #20 completed issue #5's run-local SSD cache-integrity slice. That cache is
not restart persistent. Issue #14 is now the active P0 correctness boundary;
issues #18, #15, and #16 own cache measurement, cold prompt processing, and
generation respectively. The ordered implementation and target kill gates are
recorded in `project/PERFORMANCE_WORKPLAN.md`.

The current always-on deployment remains a healthy conventional MiniMax
`UD-Q6_K_XL` comparison service: nimo-1 is coordinator/API and nimo-2 is the
RPC worker. It is not proof that current HaloFPX source or a ROCmFPX GGUF is
deployed. Historical ROCmFP4 experiments with reversed roles remain unchanged.

Reason: the owner corrected the operating-system and product-target
understanding. The documentation audit found current routing pages that mixed
vendor Ubuntu guidance, donor Framework hardware, historical role assignments,
and MiniMax-centric priority language. The live receipt and audit are retained
under `docs/halofpx/evidence/2026-08-12-strix-halo-live-authority/` and
`project/reviews/follow-ups/`.

## 2026-08-12 — make HaloFPX model-general and cache/performance-first

Decision: HaloFPX is a model-general llama.cpp-derived inference engine for
the two AMD Strix Halo Linux nodes and their `gfx1151` accelerators. The
current MiniMax artifact is the largest available stress fixture. It is not
the product target, the default optimization target, or evidence that a
MiniMax-specific improvement benefits other supported models.

Use this default priority order:

1. persistent prompt and KV-state integrity, including corruption-as-miss;
2. restart-persistent reuse and verified prefix reuse with a correct cold
   recomputation path;
3. prompt processing and time to first token; and
4. token-generation throughput and latency.

Model-specific work is stress coverage unless a generic measured bottleneck
and a reusable correction justify it. Prompt processing, cache reuse benefit,
and generation are separate measurements. Do not credit restored prompt work
to the cold/cache-off prompt-processing engine, and do not combine prompt and
generation rates into one performance claim.

Performance authority requires matched evidence from the real dual-Strix-Halo
Linux machines. The local Windows PC is a source-control, documentation,
build-orchestration, and limited CPU-test environment; it cannot establish
`gfx1151`, dual-node, prompt-processing, cache-reuse, or generation
performance. Preserve the exact model, source, binaries, configuration,
topology, sampler, cache state, and raw measurements used for every promoted
performance result.

Testing is proportional to a personal project. During ordinary development,
run the exact impacted tests and use short paired target-machine screens when
performance is relevant. Reserve broad suites, long matrices, and repeated
statistical qualification for milestone gates or a concrete regression. This
does not weaken the rule that corrupt, incompatible, incomplete, stale, or
unauthorized cache state must miss and recompute rather than be accepted.

The first active integrity slice is
[GitHub issue #5](https://github.com/JCFrags/HaloFPX/issues/5), which covers
same-size SSD prompt-cache corruption without claiming completion of HaloKV.
[GitHub issue #13](https://github.com/JCFrags/HaloFPX/issues/13) records this
owner clarification.

This decision supersedes model-specific or older current-priority language in
the records below. It does not rewrite their exact experiments, accepted or
rejected results, model identities, measurements, or historical milestone
scope.

Reason: the owner clarified that MiniMax is used because it is the largest
current test model, while the intended engine must run any supported model as
well as possible. Reliable saved-state reuse, prompt processing, and token
generation are the product outcomes that should govern the next work.

## 2026-08-12 — accept the bounded L111 loader foundation

Decision: accept L111 as `PASS / RETAIN` at exact implementation source
commit `620ef60aa446990335ef46c7d76738f797e62f8f`. That commit is the direct
child of accepted L110 base
`6c88472bf5f567a1064f27f4d8a90fc8e2b47a02`.

The accepted qualification is narrow. The retained Release and Debug focused
CTest each passed `1/1`. The feature-off static `llama` target compiled with
both RPC options disabled. Fresh exact-diff review returned `PASS / RETAIN`
with no P0 or P1 finding. The source-diff and focused-test digests match the
immutable L111 receipts.

This decision accepts only the loader-internal, generation-bound atomic
two-rank partition transaction, its accounting and rollback behavior, removal
of the obsolete public slice/exclusion aliases, and typed refusal of the old
MiniMax peer-half loader mode. It does not promote graph execution,
asynchronous RPC, scheduling, a model or runtime result, production behavior,
cache behavior, product readiness, or performance. Feature-off evidence is
compile-level; no real MiniMax model was loaded for L111. The focused fixture
is a tiny contiguous F32 GGUF on mock devices, and no production caller adopts
the new partition constructor. The raw build logs and binaries were not
retained, so this reconciliation accepts the recorded build receipt and its
independent review; it does not independently reproduce their binary hashes.
A later graph or RPC milestone must receive a separate decision and separate
qualification.

Keep `docs/halofpx/evidence/l111/source-receipt.txt` immutable. Its
`accepted_head=6c88472...` field identifies the accepted pre-change base, not
the terminal retained commit. Keep
`docs/halofpx/evidence/l111/project-lead-report.txt` immutable. Its rejected
direct delivery records an unreachable historical task identifier, not a
technical rejection. The additive L111 reconciliation note and this decision
supply the missing lead disposition.

Reason: the terminal source commit, focused build receipts, exact source-diff
identity, and independent review satisfy the bounded retention gate defined by
the 2026-07-29 L111 decisions. The historical report could not reach the old
lead task, so the technical result was committed but the lead status files
remained stale.

## 2026-07-29 — require user-visible worker tasks

Decision: stop internal subagent implementation work. Future implementation,
documentation, review, and research workers must run as user-visible Codex
tasks. Preserve the frozen L111 worktree exactly until the visible
implementation task takes ownership.

Create two visible tasks:

1. L111 implementation takeover, using
   `project-management/lead/worker-specs/L111_VISIBLE_IMPLEMENTATION_TASK.md`.
2. Full internal documentation and Wiki Simplified Technical English and
   organization work, using
   `project-management/lead/worker-specs/DOCUMENTATION_STE_ORGANIZATION_TASK.md`.

The documentation task must make `WORKER_START_HERE.md` the standard entry
point for all workers. It must preserve raw evidence, licenses, immutable
receipts, and exact technical meaning. It owns documentation navigation and
validation, not implementation or lead authority records.

Current task creation is blocked because all desktop task-control calls return
`No handler registered`. Do not substitute invisible internal subagents and do
not claim that a task was created. Retain the exact specifications and retry
visible dispatch when the control plane is available.

Reason: the user must be able to inspect every worker directly. Durable task
specifications prevent context loss while the desktop control plane is
unavailable.

## 2026-07-29 — resolve L111 legacy-alias scope contradiction

Decision: permit L111 to make the minimal MiniMax loader call-site and helper
declaration changes required to remove the accepted base's public
caller-forgeable source-slice and lookup-exclusion APIs. This is a mechanical
migration into the loader-owned atomic pair transaction, not authorization to
change MiniMax graph or RPC behavior.

The migration must preserve exact default-off shadow tensor source ranges,
shapes, target devices, lookup visibility, and feature-off behavior. A complete
source scan must prove no raw/public creation or exclusion alias remains.
Graph execution, routing, asynchronous RPC, scheduling, host/model/production
runtime, cache, and performance claims remain prohibited. All original L111
tiny-GGUF accounting, rollback injection, compilation, and independent-review
gates still apply.

Reason: exact source inspection proved the old helpers are already actively
called by the default-off MiniMax loader. A safe loader-internal authority
cannot replace them while call-site edits are forbidden. Allowing only the
mechanical migration resolves the contradiction without broadening product
semantics.

## 2026-07-29 — accept L110 rejection and assign fresh L111 worker

Decision: accept L110 NOT PROMOTED/evidence-only at
`6c88472bf5f567a1064f27f4d8a90fc8e2b47a02` and its complete candidate
removal. Retire the long-lived worker from active ownership and preserve its
task as history. Assign fresh worker `/root/l111_loader_transaction` to the
isolated loader transaction gate.

L111 must implement one loader-owned, generation-bound operation creating
exactly ranks `{0,1}` as two physical partitions of one logical GGUF tensor.
It validates every input before mutation, covers every throwing mutation, and
either performs a no-throw commit or restores exact contexts, registries,
offsets, lookup state, counters, progress, and accounting. Logical source
accounting is one tensor; physical buffer sizing includes both slices exactly
once. Raw/forgeable checkpoints and caller-asserted primary authority are
forbidden.

Qualification uses a focused tiny-GGUF fixture with exact target bytes,
counts, size/progress/mmap/source offsets, public lookup, deterministic
teardown, and injected failure after every mutation boundary. Closed negatives
cover coverage, dimensions, ranks/devices, overrides, hidden full allocation,
cross-loader/stale transaction, and unwind. A fresh independent exact-diff
review decides retention. L111 cannot touch MiniMax graph, asynchronous RPC,
scheduler, host/model/production runtime, cache, or performance claims.

Reason: L110's reviewed P1 defects were in the loader foundation itself, and
the previous worker had accumulated a long sequence of unrelated protocol,
cache, and performance context. A fresh bounded worker is the safest and
fastest way to obtain a trustworthy primitive before resuming architecture
work.

## 2026-07-29 — resume L110 at atomic partition-loader gate

Decision: resume the idle L110 worker after a read-only thread and repository
audit. Preserve the four L110-authored uncommitted files only as WIP while
replacing their unsafe caller-asserted single-slice interface. Do not treat the
earlier architecture feasibility review as source approval.

The first gate is one typed atomic two-rank source-partition constructor. A
single operation must create both slices of one logical GGUF tensor and prove
same source/type/shape, exact full axis-2 coverage, no gap/overlap/bounds error,
distinct admitted rank/device ownership, one logical public/accounted tensor,
secondary lookup exclusion, exact aggregate bytes/count/progress/mmap offsets,
no hidden full allocation, and complete unwind on partial failure.

A focused tiny-GGUF fixture must verify bytes in both target contexts and
refuse missing halves, gaps, overlap, reversed/out-of-bounds ranges, same
target, ambiguous primary, hidden full tensor, override/ownership mismatch,
and partial allocation failure. Feature-on/off compile and a fresh independent
review of the exact diff/evidence are required. No MiniMax graph, asynchronous
RPC, host/model/production action, or product-candidate commit is allowed
before this gate passes.

Reason: exact inspection found that L110 had stopped with an uncompiled,
uncalled four-file WIP. The proposed boolean `primary_partition` lets callers
assert accounting without proving a complete authoritative pair. Hardening
the loader boundary first is the smallest safe resumable step and prevents
false rank ownership from contaminating the later graph/protocol work.

## 2026-07-29 — accept L109 blocker and authorize L110 expert parallelism

Decision: accept L109's scheduler-only semantic blocker at
`e5b4a9a0d1e92c44785fee8cc58bf56ef29bd4d2`. Do not retain host-threaded
or event-only overlap. Open L110 as the separately authorized multi-layer
expert-parallel graph, RPC, ownership, and scheduling lane.

For each MiniMax MoE layer, routing occurs exactly once. Selected experts are
partitioned into explicit local and RPC rank ownership, rank partials execute
concurrently, and an exact deterministic fixed-order join completes before the
residual and next layer. The prototype must span multiple consecutive layers,
preserve router/top-k/scale semantics and tensor lifetimes, and retain
feature-off and single-node fallback.

L110 may add a narrowly scoped true asynchronous RPC graph
plan/submit/completion primitive and dependency-aware scheduler frontier. The
authenticated lifecycle binds plan, attempt, sequence, split/rank ownership,
completion, receipt, cancellation/failure, and join. Host enqueue or socket
wait is not accepted as device overlap. Rank failure must cancel/drain safely
and either fall back before visible mutation or fail closed.

Independent pre-runtime design review precedes focused no-model
submit/completion/cancel/replay/refusal tests and device-event proof. The
predeclared repeated multi-layer MiniMax-shaped screen then requires exact
output, real simultaneous devices, and at least 10% matched regional
end-to-end improvement. The candidate is removed if it misses that threshold,
regresses a matched case, or synchronization removes the benefit. A retained
result may recommend one later bounded primary discriminator, but L110 may not
run the primary model, mutate production, reopen cache work, revisit
single-layer kernels, use a broad matrix, or claim full-model speed from
synthetic evidence.

Reason: source proves there is no cross-layer independence to schedule. The
only available concurrency is inside each MoE layer's independent expert
branches. Reaching it requires real rank ownership and asynchronous completion,
not another scheduler surface patch.

## 2026-07-29 — accept L108 removal and pivot L109 to rank overlap

Decision: accept L108 NOT PROMOTED/evidence-only at
`56a0526cb4e9fdb1a0d597ca391f47bbcedb9bb5` and confirm removal of its
false/unreachable candidate. Pause the user-facing distributed server-cache
composition epic. Preserve L101's accepted primary rank-local cache
correctness, but stop serially adding cache authority/transaction milestones.

Open L109 as a time-boxed scheduler-wide, multi-layer rank-overlap performance
screen using the accepted P08/P13 evidence. It targets the measured serialized
remote and local decode phases with a synthetic repeated MiniMax expert-layer
workload spanning consecutive layers and both ownership domains.

Any prototype must be default-off, preserve exact dependency ordering, buffer
lifetime, deterministic output, error propagation, synchronization, and
feature-off behavior. Device events/timelines must prove real concurrent
execution across multiple layers and ranks—not host enqueue overlap or a
single projection. The candidate is removed promptly if synchronization erases
overlap, any matched case regresses, correctness differs, or the synthetic
repeated-layer end-to-end improvement is below a predeclared material screen
threshold (10% unless exact overhead evidence justifies a different threshold
before measurement).

Reuse accepted profiling and do not revisit P03, P05, P09, P10, P11, P13, or
P14 without new evidence. Independent review is required before retention.
L109 may recommend one later matched primary discriminator only if the screen
passes. It may not load the primary model, mutate production, reopen cache
composition, run a broad matrix, or claim product speed from synthetic data.

Reason: L108's review showed that a complete server cache needs coordinated
scheduler, memory, manifest, RPC, and ownership work that is not converging as
incremental seams. Meanwhile, profiling already identifies a much larger
performance opportunity than the rejected micro-optimizations. This pivot
returns effort to measurable product results without discarding the proven
cache foundation.

## 2026-07-29 — accept L107 removal; implement the complete slice in L108

Decision: accept L107 NOT PROMOTED at
`2cedd6a151d1c276530fa0b8d96d622c967ed0b5` and confirm removal of the
unreachable partial mode. Open L108 to implement the complete dedicated
shadow-context server cache slice before exposing it.

Persisted candidate identity binds only stable logical model/profile/request,
plan, topology, ownership, placement, component manifest, key generation, and
channel authority. It must never bind a prior socket, connection epoch,
allocation epoch, or candidate-supplied live identity. Each fresh attempt must
fully construct and allocate its shadow before authenticated preflight binds
current connection and allocation epochs.

The exact lifecycle is: quiesce and retain the old context; fully allocate the
shadow; independently reconcile stable candidate authority; freeze the exact
ubatch/plan; preflight; stage and commit local/remote state; transfer server
ownership; execute and terminalize that same plan; only then destroy the old
context and require fresh preflight for later work. Allocation after preflight,
identity drift, candidate/live conflation, early old-context destruction, or
partial transaction refuses and uses the defined cold-recreation path.

L108 may keep mechanical integration and review corrections in one milestone,
but may not run an intermediate partial mode. Focused gates cover stable/live
identity negatives, allocation ordering, isolation/lifetime, both failure
sides of remote commit, single-use plans, unsupported modes, feature-off and
world-1 parity, and resource refusal. Independent pre-runtime review must pass
before a real two-host no-model transaction and one Stories15M
miss/publish/fresh-shadow-hit/corruption-fallback run. No primary model,
production, tuning, broad matrix, or new wire protocol is authorized.

Reason: L107 did not disprove the product design; it proved that scaffolding
cannot safely be exposed piecemeal. Stable cached authority and fresh live
attempt authority must remain separate, and the entire transaction must become
reachable atomically.

## 2026-07-29 — accept L106 blocker; use a dedicated shadow context in L107

Decision: accept L106's confirmed in-place KV restore blocker at
`e15d6da0de55c0f1a604614db62b5d50957b40e3`. Do not build a general atomic
swap facility for arbitrary live continuous-batch memory. Open L107 for a
dedicated, quiescent, single-slot distributed cache server profile using a
fresh disposable shadow `llama_context`.

The shadow shares immutable model weights but owns its KV, scheduler, and
runtime state. Candidate authentication, complete local restore, exact request
plan construction, and authenticated remote stage occur without touching the
active context. At a quiescent boundary, the server revalidates active
generation, empty slot, topology, and candidate identity, commits remote state,
then transfers active context/runtime ownership to the already-prepared shadow
and executes the same plan.

Before remote commit, any failure discards the shadow and cold-recomputes on
the unchanged active context. After remote commit, failure may not expose the
old local state with new remote state: it must finish the prepared ownership
transfer or tear down and cold-recreate the distributed residency before
recompute. Transient duplicated KV/context/scheduler memory must be bounded and
headroom-checked; model weights remain shared.

This product profile must refuse multi-slot continuous batching, speculative
decode, recurrent, hybrid, ISWA, and unsupported memory. Feature-off, world-1,
and normal server behavior remain unchanged. Qualification covers shadow
isolation/destruction, quiescence and single-use generations, both sides of the
remote-commit failure boundary, resource refusal, unsupported modes,
independent review, a real two-host no-model transaction, then one Stories15M
miss/publish/fresh-shadow-hit/corruption-fallback run. No primary model,
production, tuning, general swap API, broad matrix, or new wire protocol is
authorized.

Reason: existing restore preparation mutates live KV metadata before the
candidate is fully validated. A disposable context uses the already-proven
fresh-residency model and makes failure recoverable without inventing rollback
for arbitrary live state.

## 2026-07-29 — accept L105 blocker; scope L106 to transactional KV only

Decision: accept L105's confirmed memory-mutation blocker at
`a35816e52f4bb2510936fa1a29e623c3b9249521`. Do not authorize a general
transactional memory rewrite. Open L106 for a transformer-KV-only
non-mutating preview and atomic commit contract, then continue the distributed
server-cache product slice.

The default-off distributed exact-key profile supports ordinary transformer KV
memory only. Recurrent, hybrid, ISWA, and any memory implementation without
the typed contract must refuse feature enablement or cache eligibility.

After the real server batch/ubatch is frozen, KV preview owns the proposed
cell/head/slot placement, post-commit graph-facing `n_kv` and metadata, and the
source KV generation without mutating live state. Atomic single-use commit
immediately before execution must validate the generation and apply exactly
that placement. Abort/drop is non-mutating. Pending shift/copy/defrag must be
completed before preview or explicitly represented and generation-bound.
Stale, conflicting, reused, double-committed, wrong-context, or wrong-batch
plans refuse and clean-recompute.

L106 must use the same handle for lookup and hit/miss execution, then finish
the live distributed topology/profile/transaction bridge already authorized.
Qualification remains focused: KV placement/generation/state-machine tests,
unsupported-memory refusal, preview-versus-commit graph identity, independent
review, a real two-host no-model transaction, and one Stories15M
miss/publish/fresh-worker-hit/corruption-fallback vertical run. Mechanical
integration corrections may remain within L106. No primary model, production,
general memory support, tuning, broad matrix, or new wire protocol is
authorized.

Reason: graph planning genuinely needs the post-placement KV geometry, but the
project does not need to solve every llama memory architecture before shipping
its transformer cache path. A strict KV-only profile closes the proven
MiniMax/product requirement while safely refusing unsupported models.

## 2026-07-29 — accept L104 blocker and authorize L105 request-plan ownership

Decision: accept L104's confirmed lookup-order blocker at
`23f088deea65833a714271d7033d9c1c5f46c733`. Authorize L105 to add a
single-use non-executing request-plan handle, then continue the already
authorized distributed authority/profile/transaction composition.

The handle must be produced from the exact request ubatch through the same
graph build/allocation, census resolution, split binding, and authenticated
non-mutating RPC preflight used for execution. It owns the graph lifetime and
immutable topology, epoch, channel/key-generation, endpoint, split/census,
and component-descriptor authority. It may not consume admission, set inputs,
mutate state, compute, publish, or derive authority from a cached candidate.

Exact-key lookup may use only this live handle. A hit must reconcile the
candidate and restore storage against it; a miss must pass the same handle into
decode and capture without rebuilding. Drop/error safely releases and aborts.
Reuse, double-consumption, wrong request/slot/context, stale epoch,
independent rebuild, or incomplete authority must refuse and clean-recompute.
Feature-off decode and world-1 cache behavior remain unchanged.

Qualification is bounded to focused lifecycle and feature-off tests,
plan-versus-execution identity equality, drift/stale/double-use negatives,
independent review, and one real two-host no-model plan-to-hit/miss
transaction. If those pass, the worker may finish the L104 distributed
profile/bridge and one Stories15M vertical run under the existing gates. No
primary model, production mutation, tuning, broad matrix, or new wire protocol
is authorized.

Reason: valid cache selection requires live request topology, but that topology
currently exists only after lookup. A single owned plan used by both lookup
and execution breaks the cycle without trusting stored candidates or building
two graphs that can drift.

## 2026-07-29 — accept L103 blocker and authorize L104 product authority seam

Decision: accept L103's confirmed semantic blocker at
`8dcdc408ac76f73097dcfe3f39edae992d9d31b6`. Authorize L104 to implement
the bounded internal shared authority/profile seam required to compose the
server exact-key cache with the retained rank-local distributed protocol.

L104 must add three product pieces:

1. Scheduler/RPC-produced immutable distributed-checkpoint topology authority
   from live allocation and session truth, with ordered ranks, distinct
   plan/ownership/placement/topology identities and epoch, endpoint/device
   identity, channel/key generation, component manifest, and authenticated
   worker-object custody binding.
2. A separately named Linux-only, default-off distributed transformer
   profile/codec that reuses manifest v1 and represents coordinator
   control/local state plus the authenticated external worker object. The
   existing world-1 codec must remain byte- and behavior-identical.
3. A typed llama/server capture and
   prepare/reconcile/stage/local-stage/remote-commit/final-apply transaction.
   Every failure must abort and recompute from a clean context.

L104 may not derive authority from result text, opaque copied configuration,
or invented rank constants, and may not introduce a second state transport
protocol. Qualification proceeds through focused feature-on/off and negative
tests, independent pre-runtime review, a real two-host no-model transaction,
then one Stories15M server miss/publish/fresh-worker-hit test if the earlier
gates pass. That run must prove deterministic equality, zero bounded legacy
GET/SET, corruption or topology mismatch causing miss/recompute, and complete
custody/cleanup. No primary model, production mutation, performance work,
broad matrix, or automatic follow-on is authorized.

Reason: this is not optional formalism. The current server describes every
cache as world 1/rank 0 while the actual worker object is governed by live
distributed ownership. Without this seam the product can only save partial
state or lie about topology. Implementing it is now the shortest safe route to
a usable HaloFPX cache.

## 2026-07-28 — promote L102 and open L103 server composition

Decision: promote L102 PASS at
`b1e21c49606f2ffd2768d0f28766b0007498a6a8`. The downstream L101 envelope
P2 is closed. Open L103 to make the proven distributed cache reachable through
the real llama-server exact-key lane.

L103 must reuse the retained server cache and worker-local distributed
protocols rather than add another layer. It must replace the composed lane's
hard-coded single-rank cache identity with actual distributed
topology/ownership authority, orchestrate worker capture and authenticated
publication, and restore through fresh-worker stage/commit. Missing, corrupt,
stale, partial, or topology-mismatched state must miss and recompute; it may
never be accepted.

Qualification is bounded to focused source tests, one real two-host no-model
fixture, and one Stories15M end-user vertical run: miss/publish, worker
restart/fresh residency, hit/restore, exact deterministic equality, zero
legacy GET_TENSOR/SET_TENSOR state-page transfer, one corruption/mismatch
miss-recompute, complete authority/custody, feature-off inertness, cleanup, and
independent review. No primary model, production transition, performance
claim, tuning, broad matrix, or new protocol foundation is authorized.

Reason: L101 established the core rank-local mechanism's primary-model
correctness. The remaining product gap is reachability: the current
llama-server exact-key transformer uses coordinator state flags and
single-rank identity, while the worker-local protocol is only called by the
canary. Closing that seam produces an actual usable feature and is now higher
value than more harness work.

## 2026-07-28 — accept L101 cache correctness; open L102 envelope-only correction

Decision: accept the retained block-aware serialization source and L101's
primary capture/fresh-restore result as conclusive cache-correctness PASS at
terminal commit `4ebc29ee1f557ffa73860465158e6a35e80540fb`. Keep L101 itself
NOT PROMOTED solely because no signed terminal controller envelope was
published. No primary correctness rerun is authorized or required.

Open L102 only to correct and qualify the composed-result signer invariant.
Retained operation 540 proves the helper refused `RPC mutable authority is
incomplete`; the five authenticated records have `set=7`,
`set_hash_hit=0`, and `set_hash_miss=0`. The signer incorrectly requires the
last two counters to sum positive even though L101 proves the valid
rank-local path used zero legacy GET_TENSOR/SET_TENSOR state-page transfers.

L102 must preserve positive set activity and every mutable, graph, receipt,
identity, and cross-binding requirement while permitting the legitimate
zero-hit/zero-miss legacy-counter state. Qualification is limited to retained
L101 composed payloads, focused real helper sign/verify, malformed and partial
negative cases, and independent review. It must not access the model or
production, forge a retrospective L101 envelope, reopen block geometry, or
claim performance or end-user cache reachability.

Reason: replay correctness is established by identical token, suffix, logits,
all 152,180,736 occupied KV bytes, all 64 RPC components, and authenticated
server custody. Repeating that expensive run would add no decision value. The
remaining defect is an exact, downstream evidence-validator assumption.

## 2026-07-27 — accept L56 root cause and bind parent/split graph identity

Decision: accept L56 diagnostic PASS commit
`8af226d675d9ae287d5d2bddd849f9920507d9ba`. Open L57 only to correct L40
reconciliation using scheduler-owned parent-to-split UID mapping for the exact
backend/execution and run one complete stories15M controller qualification. No
primary, production, cache expansion, tuning, matrix, or L58.

Reason: the exact defect is proven without another runtime. Parent UID `26` and
RPC split UID `27` are different valid identity levels; comparing them directly
caused the first armed prompt failure. The correction must bind both rather
than accept arbitrary UIDs or assume numeric adjacency.

## 2026-07-27 — accept L55 localization and discriminate L40 receipt refusal

Decision: accept L55 diagnostic PASS commit
`51e87b0c011eb3c7dc5b170bd8f64048bccd0853`. Open L56 only to instrument the
closed refusal conditions inside coordinator-side L40 graph-result
reconciliation and, if source alone is insufficient, repeat exactly the first
armed chunk once. No semantic fix, later execution, primary, production, or
L57.

Reason: L55 decisively clears server graph execution and localizes the first
non-success to one client reconciliation branch. The remaining ambiguity is
small: receipt absence versus an exact identity, digest, authentication,
consumption, or status mismatch. Broad scheduler or cache work is no longer
justified.

## 2026-07-27 — accept corrected chronology and trace first armed chunk

Decision: accept L54 NOT PROMOTED commit
`0578c9ce3e58ef832af734ab4a9c0e0ddae94f26`. Open L55 only for embedded
source/binary provenance and bounded authenticated status transitions through
common warmup and exactly the first armed 512-token prompt chunk. Stop after
that chunk; no semantic fix, later prompt, capture/restore, primary,
production, or L56.

Reason: L54 correctly avoided an incapable warmup-only run. Retained chronology
now proves warmup succeeds and the first armed prompt fails only after remote
graph execution. The next decisive boundary is the exact client receipt,
scheduler copy/finalize, L42/L44 result, or caller status branch.

## 2026-07-27 — accept L53 contradiction and run one lineage discriminator

Decision: accept L53 NOT PROMOTED commit
`20af537f0d36d9de3877af860e4f24d89d7e2641`. Open L54 only for exact
source/binary provenance and bounded pending/arm/split/RPC-client/status
transition evidence during one disposable common-warmup-only run. Do not patch
semantics, execute prompt/capture/restore, access primary, mutate production,
or open L55.

Reason: retained runtime and reconstructed source disagree about whether the
warmup execution was armed. Neither a scheduler correction nor a controller
retry is justified until the exact binary lineage and first arm/refusal/status
transition are observed together. One warmup-only discriminator is the
smallest decisive step.

## 2026-07-27 — accept L52 and localize coordinator warmup status

Decision: accept L52 NOT PROMOTED commit
`d236e74d2b2c3df96d88ef4cce5269d1baf3f24a`. Open L53 only to identify and
correct the coordinator scheduler/composition branch returning decode `-3`
during an unarmed common warmup, then run one stories15M qualification. No
primary, production, cache, tuning, matrix, or L54 is authorized.

Reason: L52 closes evidence publication and passes device, readiness, and
placement. The worker journal independently shows the L40 RPC graph prepared
and executed successfully, so revisiting those gates would be waste. The next
source boundary is coordinator scheduler completion and explicit armed/unarmed
state propagation.

## 2026-07-27 — accept L51 and correct evidence-directory ordering only

Decision: accept L51 NOT PROMOTED commit
`1746c15c9688cb068751ab40619bb0637cff1b3a`. Preserve its focused-qualified
arm/disarm warmup and unit-evidence corrections. Open L52 only to create and
verify manifest-owned evidence directories before atomic device-receipt
publication, then run the deferred stories15M qualification once. No primary,
production, architecture, tuning, matrix, or L53 is authorized.

Reason: L51's model workflow did not begin. The exact failure is a controller
state-machine ordering error—copy preceded directory creation—not a warmup,
composition, cache, model, or device failure. It is bounded and should be
corrected without reopening qualified runtime foundations.

## 2026-07-27 — accept L50 and isolate unarmed warmup lifecycle

Decision: accept L50 NOT PROMOTED commit
`8fa511036ef9fc633b00fe1148ae0b032457f495`. Preserve its proven ROCm0/gfx1151
build and device gate. Open L51 only to identify and correct the unarmed
diagnostic warmup failure, and to repair InvocationID/journal/exit authority
lost by the current `systemd-run --wait --pipe` collection and fish cursor
quoting. Permit one stories15M requalification after reviewed preflight. No
primary, production, tuning, matrix, or L52 is authorized.

Reason: the ROCm build defect is closed and readiness succeeds. The current
failure occurs before any prompt, capture, restore, or composed result, so it is
not cache evidence. Warmup must be honestly feature-off until an execution is
armed, while the armed path remains fail-closed. Complete unit evidence is also
mandatory before another controller result can be accepted.

## 2026-07-27 — recover L49 exit cause and require real ROCm device admission

Decision: accept L49 NOT PROMOTED commit
`e606f62cb19063ceb7bfdbe9dff979ea0544abf0`. Use the still-retained nimo-1
journal as exact evidence that the candidate worker exited with unknown
`ROCm0` and no accelerator devices. Open L50 only to correct the proven build
or runtime device-admission defect, add a real no-model ROCm0/HFXCAP2 gate plus
mandatory early-exit evidence, and run one stories15M qualification. No
primary, production, redesign, tuning, matrix, or L51 is authorized.

Reason: the L49 transport correction passed. Reproducing the failure would add
no information because its journal remains available. A successful compile is
not device authority; the exact candidate binary must prove ROCm0 and protocol
readiness before the controller may start a model workflow.

## 2026-07-27 — accept L48 timeout and correct only readiness transport class

Decision: accept L48 NOT PROMOTED commit
`591603ff0982fe684fd67c45f40898f4332fac88`. Open L49 only to reconstruct the
pre-runtime-accepted L48 candidate, add a closed HFXCAP2 readiness transport
class whose outer deadline exceeds its frozen 120-second application budget,
and run one stories15M controller qualification. No primary, production,
redesign, tuning, matrix, or L50 is authorized.

Reason: all L48 design, binding, key, verifier, build, and pre-runtime gates
passed. The sole execution failed before qualification because nested deadline
authority was inconsistent: generic command killed the readiness process at 30
seconds. This is one precise transport classification defect and does not
justify another architecture iteration.

## 2026-07-27 — accept L47 runtime proof and close the controller binding

Decision: accept L47 NOT PROMOTED commit
`d9aabb66822660b393cc8f14501ea5552471c6d9`. Open L48 only for a closed
argv-safe runner/controller result binding with protected key-file authority,
explicit unarmed warmup lifecycle, mandatory composed-result verification, and
reconstruction of the already-qualified ADR-0048 composition. No primary,
production, tuning, matrix, or L49 is authorized.

Reason: L47 proved the difficult runtime mechanics on the real disposable
multi-execution canary, but manual logs cannot authorize a primary transition.
The remaining blocker is operational and bounded. Building the verifier and
controller contract before reconstructing the candidate prevents another
successful manual run that the primary controller cannot accept.

## 2026-07-26 — accept L46 blocker and correct foundation composition

Decision: accept L46 NOT PROMOTED commit
`0d655b54d77929dafc2a7efe05f25a94d6c6ca0d`. Open L47 as an ADR-first,
no-primary/no-production correction for mixed local/RPC census semantics, an
actual scheduler-derived precompute RPC split/copy admission bridge, and
per-execution L42 arm/prepare/finalize/abort with adjacent L44 result binding.
No L48 or product/performance work is authorized.

Reason: L46 proves the accepted L42 and L44 layers are individually useful but
their public boundaries are mutually incompatible for the real workload.
Another runner patch cannot solve the leaf-locality contradiction or ordering
cycle. The smallest honest path is to correct their composition contract and
qualify it through the real disposable multi-execution canary before returning
to the primary model.

## 2026-07-26 — accept L45 pre-mutation blocker and wire the real caller

Decision: close L45 NOT PROMOTED without a transition, then open L46 only to
wire accepted L42/L44 authority into the real distributed-state canary and
runner using structural source-call-site registration and closed evidence. No
primary access, production mutation, cache promotion, tuning, matrix, or L47 is
authorized.

## 2026-07-28 — accept L100 root cause and open L101 block-aware serialization

Decision: accept L100 PASS at
`1597a2b4610d397b9d31411b51210527edfc9b31` and its independently reproduced
124-tensor audit. Open L101 for the narrow block-aware KV serialization
correction, targeted qualification, and one conditional primary confirmation.

Reason: current capture/prepare computes element count as byte size divided by
`ggml_element_size`. For Q8_0, the divisor is the 34-byte storage block; a new
Q8_0 tensor then reapplies 32 elements per block, shrinking each requested
1,227,264-byte occupied interval to 38,352 bytes. Only 4,755,648 of
152,180,736 provably read occupied bytes were serialized/restored. This exact
147,425,088-byte omission explains why represented hashes matched while logits
and tokens diverged.

L101 must use one checked byte-to-element/view contract across coordinator
local and RPC capture/stage/apply: require byte length divisible by type size,
compute `(bytes/type_size)*block_size` with overflow checks, validate type,
offset, stride, dimensions, allocation bounds, and require
`ggml_nbytes(view_or_copy)==requested_bytes` before capture/publication or
restore acceptance. Plain block-size-1 types follow the same helper. Partial
blocks, mismatch, truncation/oversize, misalignment, and range escape refuse.
Preserve exact occupied rows; padded-row expansion is not authorized.

Focused gates must cover Q8_0 unit and retained 1,227,264-byte geometry,
plain types, structural negatives, all 124 L98 tensors totaling 152,180,736
represented occupied bytes with zero readable gaps, capture/stage/apply/live
equality, feature-off inertness, and the smallest real two-host quantized
capture/restore fixture with zero legacy GET/SET. One adversarial review is
sufficient.

Only if those gates pass with no P1/P2 may exactly one OOM-aware primary
capture/fresh-restore/one-token confirmation run. Acceptance requires token
21549/suffix `alpha`, exact logits and represented/live state equality, all 124
occupied ranges, per-attempt custody, authenticated bounded zero legacy
GET/SET, cleanup, ordered recovery, and exact production reconciliation. No
retry, padded-row expansion, protocol/cache product/performance work, or
automatic follow-on is authorized.

## 2026-07-28 — accept L99 and open L100 KV physical-range coverage

Decision: accept L99 PASS at
`bf861840423c60c9f71afa119086b32b4e4ef5e3`, including its canonical L98
diff, ranked diagnosis, reviewed per-attempt response verifier, and explicit
zero-GET/SET evidence limitation. Open L100 only for the recommended offline/
no-model physical byte-range coverage discriminator across all 124 KV tensors.

Reason: L99 proves the first retained divergence is computed logits, after
equal represented state, replay geometry, node assignment, and graph input.
The strongest remaining source-backed candidates are an auxiliary replay input
outside the contract or kernel-readable physical KV bytes omitted by logical
serialization. A complete range audit is cheaper and more decisive than
another primary run.

L100 must derive the byte intervals actually or conservatively addressable by
the q8_0 final-replay attention/read kernels from exact tensor type/dimensions,
strides, views/ancestry, allocation offsets, K/V layout, backend ownership, and
kernel block/tile/vector behavior. Account for block rounding, padding,
alignment, alias/overlap, transposition, and noncontiguous access. Compare
these with exact captured, serialized, applied, and live component intervals,
classifying proven-read, possibly-read, allocation-only, represented, restored,
and unrepresented bytes. A gap is not causal unless source proves it can be
read.

Require machine-readable totals and tensor/range identities, focused synthetic
coverage for contiguous, padded, quantized, strided/view, alias, truncated, and
small-gap cases, retained L98 replay, and independent review. If a readable
gap exists, stop with exact ranges and the smallest serialization-correction
proposal; do not implement. If coverage is exact, close this hypothesis and
specify the minimum synchronized authenticated per-layer pre/post-attention
digest discriminator. No host/model/accelerator build/runtime/production
action, primary retry, semantic correction, or automatic follow-on is
authorized.

## 2026-07-27 — allow one L59 replacement after Windows durability admission failure

Decision: preserve the first L59 controller attempt as a pre-runtime admission
failure and authorize one mechanical cross-platform directory-durability
correction followed by one replacement stories15M first-chunk discriminator.

Reason: L59's failure harvester passed focused tests, independent pre-runtime
review, and a real injected Linux unit failure that retained an authenticated
partial worker stream while explicitly recording the missing client stream.
The controller attempt then stopped before any stories model or RPC
discriminator because its new unconditional POSIX directory-fd `fsync` raised
`PermissionError` on the local Windows evidence directory. The intended runtime
authority was therefore not consumed.

The correction must retain file fsync and atomic publication, preserve POSIX
directory fsync on POSIX, use and record the strongest supported Windows
directory/publication durability mechanism, reopen/revalidate the published
file, and fail visibly on durability errors. Only affected tests and review may
be repeated before one replacement first-chunk run. No semantic RPC/cache
change, capture/restore matrix, primary access, production mutation, or
performance work is authorized.

## 2026-07-27 — reject L62 candidate and require real lifecycle authority

Decision: accept terminal L62 NOT PROMOTED at
`76bae384139dbc083cea7a8fa4e26479a1219c2b` with its candidate removed.
Open L63 as a source-native, attempt-scoped redesign of real L44 lifecycle,
L40/L42 pre-execute admission, and honest transport staging. Stories runtime
is conditional on a real composed no-model fixture and independent review.

Reason: L62 correctly stopped before model runtime. Its no-model fixture failed
at mutable begin, while review established that the verifier relied on
synthetic records, send failures were mislabeled as sent requests, connection
epoch reused a graph/server nonce, actual L44 lifecycle refusals were absent,
and process-global event cardinality could mix attempts. That evidence contract
could not safely classify the real failure.

L63 must emit records at the actual lifecycle and graph-compute seams, bind a
real connection/allocation epoch and attempt identity, represent transport as
explicit byte-progress/EOF/error stages, and enforce bounded per-attempt
cardinality and concurrency isolation. A real two-host composed success plus
material refusal and transport-failure paths must pass using L61 harvesting;
synthetic record tests are supplementary only. One stories first chunk is
authorized only after review accepts that foundation. No cache matrix, primary
artifact, production mutation, or performance work is allowed.

## 2026-07-27 — accept ADR-0049 and implement the lifecycle foundation

Decision: accept terminal L63 NOT PROMOTED at corrected exact HEAD
`7628b7cd2ada0fc4d0262f94eaee59f3768a13b0` and accept ADR-0049 as the
frozen design authority. Open L64 to implement and qualify the complete
capability/lifecycle revision. Do not run stories15M in L64.

Reason: source review proves the requested evidence cannot be safely layered on
the accepted APIs. Connection identity currently aliases a server nonce; there
is no distinct allocation-topology epoch; L42 admission is not handle-bound;
L44 refusal recording begins after begin succeeds; scheduler authority can be
reused across unrelated executions; and global locking/cardinality spans
network and attempt boundaries. A partial patch would preserve ambiguity.

L64 must negotiate distinct connection and allocation epochs, bind immutable
prepared admission to an attempt handle, create refusal authority before L44
begin, isolate locks/cardinality per attempt without holding locks across I/O,
record honest staged AUTH transport, and clear all authority on
finalize/abort/disarm/reconnect/reallocation. Focused protocol, concurrency,
epoch, refusal, cleanup, and feature-off tests plus a real multi-execution
two-host no-model composed fixture and independent adversarial review are
mandatory. Candidate source is retained only on PASS. No stories model, primary
artifact, production mutation, cache matrix, or performance work is authorized.

## 2026-07-27 — reject L64 candidate and move L65 to a fresh implementation task

Decision: accept terminal L64 NOT PROMOTED at
`54910a78fae5de586d918cb1252e5867749513b2`; its rejected candidate remains
removed. Assign L65 to fresh task `019fa62e-2e6f-7451-846f-1d4a6c1d13d0`
from the exact clean L64 base. The prior implementation task becomes an idle
preserved record.

Reason: L64's real composed observation was promising but not reusable.
Independent review found seven material foundation defects: no explicit mutual
wire capability negotiation; consuming/unverified L42 admission; incomplete
real refusal coverage; tautological event cardinality; abort/disarm evidence
loss; incomplete transport failure/harvesting coverage; and no truly
overlapping concurrency or atomic multi-writer publication authority. The
long-running worker has accumulated extensive diagnostic context, so a fresh
implementation owner is preferable for a protocol redesign.

L65 must close every review finding with explicit negotiated versions,
independently authenticated handle-bound admission, pre-begin per-attempt
recording, non-tautological terminal grammars, honest real transport failure
injection, evidence-preserving cleanup, atomic per-attempt publication, and
genuinely overlapping attempts. The real two-host no-model composed fixture
must cover execute/recompute, epoch rollover, refusals, concurrency, exact
output, feature-off, and L61 harvesting. Candidate source is retained only
after independent PASS. No stories model, primary artifact, production
mutation, cache matrix, or performance work is authorized.

## 2026-07-27 — accept L61 harvesting and move before authenticated execute

Decision: accept terminal L61 NOT PROMOTED at
`b74170ec208f17001d12e8bb5278f67f75bb38ba` and retain its reviewed
host-bound response harvesting. Open L62 only to instrument the immediately
preceding L40/L42/L44 admission and graph-compute/recompute client decision,
then run one first-chunk discriminator.

Reason: L61 proves both owning-host harvesters and both stream paths work.
During the sole stories run, both streams were truthfully absent because
`RPC_CMD_HALOFPX_GRAPH_AUTH_EXECUTE` was never entered. Chronology establishes
that the worker's ordinary 144-node graph was common warmup, while the armed
first chunk failed locally before authenticated execution. The later RPC
malformed/crashed text came from buffer teardown and is not causal evidence.

L62 must emit a closed authenticated reason for every reachable pre-execute
decision: armed/pending/sequence, L40 prepare/compute/recompute, L44 mutable
session lifecycle/refusal, L42 prepared/final state, chosen opcode and
send/no-send result, parent/split identity, and exact local return status.
Focused disposable accepted/refusal paths and L61 harvesting must pass before
one stories first chunk. Once model runtime starts there is no retry; a proven
semantic correction requires a later decision. No cache matrix, primary
artifact, production mutation, or performance work is authorized.

Reason: preflight proved the accepted diagnostic APIs are not invoked by the
primary caller. Running with L44 enabled would refuse compute; running with it
disabled would violate the discriminator contract. This is a material but
bounded integration prerequisite, not a justification to weaken the gate or
repeat a primary attempt.

## 2026-07-26 — accept L44 and run one complete primary discriminator

Decision: accept L44 PASS commit
`5f69d5cdaf8eb51283dd750c1fd8ca869fcf4d66`. Authorize L45 as exactly one
controller-managed primary two-fresh-residency, one-token correctness run
combining the accepted L40 RPC graph, L42 scheduler execution, and L44 mutable
session/update authorities. No retry, cache promotion, production enablement,
performance tuning, model matrix, or L46 is authorized.

Reason: L44 closes the last known execution-input observability gaps with
reviewed real-handler evidence and admitted-session isolation. Another
small-model diagnostic would add delay without addressing the remaining
question. The next useful evidence is whether the exact primary restore now
matches or which authenticated boundary diverges first.

## 2026-07-26 — verify migration restoration and resume exact L44 lane

Decision: accept the restored repositories at management commit
`a22632696fd04d752224f4a99822b11cdd12c4b5` and clean HaloFPX commit
`aba0f78d07c824c3bcdbcb5ffbdc26e174cda3bf`. Resume L44 only for the two L43
blockers: admitted-session isolation and negative injection through real RPC
handlers. No primary artifact, production mutation, cache semantics, tuning,
model matrix, or L45 is authorized.

Reason: repository identities and live production authority match the migration
handoff exactly. L40 and L42 remain the accepted foundations, and the smallest
path to a decisive primary discriminator is to close L43's two concrete
reusable-layer defects without reopening broader instrumentation.

## 2026-07-25 — accept L43 rejection and pause for OS migration

Decision: accept L43 NOT PROMOTED commit
`aba0f78d07c824c3bcdbcb5ffbdc26e174cda3bf`. Keep its rejected candidate
removed and preserve L40/L42. Open no L44. Pause implementation, write the
L20–L43 report and zero-context project-manager handoff, and create verified
local/server migration backups.

Reason: the user is replacing the manager OS. L43 ended cleanly with two exact
remaining blockers: admitted-session isolation and real-handler negative
injection. A durable, verified migration boundary is now higher priority than
starting new implementation.

## 2026-07-25 — accept L42 scheduler foundation and close mutable-input authority

Decision: accept L42 PASS commit
`d0d74ff55d8b063ab73911ae95516512177c824d` as the reusable authenticated
scheduler execution-authority layer. Open L43 only for structural mutable-input
classification, real RPC SET/SET_HASH server-applied receipts, and a complete
active mutable-input census bound to L40/L42 execution identity. No primary,
production, cache-semantic, tuning, or L44 work is authorized.

Reason: L42 closes all four L41 review findings and independently verifies
ordinary and expert-partial execution with exact authenticated evidence. The
remaining pre-primary observability gap is the content and completeness of
mutable execution inputs, which should be built as one separate reusable layer.

## 2026-07-25 — accept L41 rejection and close only its four review gaps

Decision: accept L41 NOT PROMOTED commit
`ba0cbd51634cd58496d35cf615dfdae32a367269`. Open L42 only for a caller-owned
authenticated scheduler event export, complete destination/allocation/view
authority, the specifically missing refusal fixtures, and exact deterministic
ordinary/expert output qualification. No broader architecture or primary work
is authorized.

Reason: L41 proved the selected real execution seams are reachable, including
ordinary and expert-partial copies, but its internal counters and incomplete
destination evidence cannot support promotion. The four independent-review
findings are concrete and bounded, so one targeted correction is warranted
without reopening the overall design.

## 2026-07-25 — accept L40 RPC foundation and add scheduler authority separately

Decision: accept L40 PASS commit
`53f414dfc5a8f9873ad9961f541eb41cf6dc2aae` as the reusable authenticated RPC
graph-authority foundation. Open L41 only for bounded default-off authority at
the actual scheduler split/copy construction and execution seams. Defer
mutable-input census, SET_TENSOR roles, primary testing, production mutation,
and L42.

Reason: L40 passed independent review after closing negotiation, canonical
encoding, server-owned reconstruction, receipt, lineage, replay, and
feature-off requirements. Building scheduler execution authority as a separate
layer preserves that accepted boundary and avoids another oversized candidate.

## 2026-07-25 — accept L39 rejection and split the authority stack

Decision: accept L39 NOT PROMOTED commit
`0658a272d25ee660055143904aa47a2d76dc2d19`. Keep the rejected combined
candidate removed. Open L40 only for the RPC foundation: explicit diagnostic
capability negotiation, bounded authenticated canonical client/server graph
records, a server-owned reconstruction receipt, and compute/recompute lineage.
Defer scheduler-copy authority, mutable census, SET_TENSOR role coverage,
expert/Q8/FA cases, any primary run, and L41.

Reason: L38 and L39 both showed that implementing the entire scheduler/RPC
authority contract in one candidate produces partial guarantees that cannot be
accepted. L39's corrected fixture was deterministic, but review found material
protocol and authority gaps. Establishing the RPC trust boundary as a reusable
accepted layer is the smallest durable forward step and prevents a third broad
approximation.

## 2026-07-25 — accept L38 rejection and instrument scheduler/RPC directly

Decision: accept L38 NOT PROMOTED commit
`169d81ad84167fcd5449b5dc99126bd861446087`. Keep its rejected replay-exec-v2
candidate removed. Open L39 only for runtime-default-off instrumentation inside
the actual scheduler split/copy loop and RPC client serialization/server
reconstruction paths, with one combined synthetic qualification. No primary
run or L40 is authorized.

Reason: L38's synthetic poison test demotes out-of-selected-span FA reads for
that bounded graph, but independent review found the proposed contract did not
bind exact scheduler split/copy execution, RPC IDs and graph equality, mutable
SET_TENSOR ordering/content, recompute UID, non-contiguous hashing, or complete
view/source authority. These must be captured at their real execution seams,
not reconstructed afterward.

## 2026-07-25 — accept L37 and authenticate numerical replay inputs

Decision: accept reviewed L37 PASS commit
`a1bee312ca79f3087cd5bfcd327e9f11b2be72d2`. Open L38 only for a bounded
default-off replay-exec-v2 contract covering mutable graph-input content,
canonical graph/scheduler assignment, inserted cross-backend copies, RPC graph
serialization/reconstruction, selected KV/attention read inputs, and output
provenance. Include one synthetic Q8_0 flash-attention poison-span
discriminator. No primary run or L39 is authorized.

Reason: L37 closes the post-free result-authority defect and qualifies admitted
mutable graph classes on the disposable path. Independent audits agree that the
remaining gap is numerical-input and execution binding rather than replay
control flow: actual masks/indices/KV slices, scheduler copy endpoints, RPC
tensor reconstruction, and whether FA reads beyond authenticated spans. A
single synthetic one-layer poison test plus bounded instrumentation is the
highest-value step before another primary transition.

## 2026-07-25 — accept L36 closeout and fix result lifetime before graph coverage

Decision: accept L36 NOT PROMOTED commit
`0b4c00c5f90cf245ceee769619886f295bf4e5d2` only as a harness result-authority
failure. Open L37 for no-production correction of the post-free context read,
source-derived graph-input coverage, focused disposable/synthetic
qualification, and independent specialist review. No primary run or L38 is
authorized.

## 2026-07-27 — accept L59 harvesting and require transient-unit absence

Decision: accept terminal L59 NOT PROMOTED at
`d80b792747a43a9ee2f6faacad6a9b5dfe17d331` and retain its reviewed
failure-harvesting and cross-platform durability foundation. Open L60 only for
a fixed transient-unit pre/post-launch absence guard followed by the deferred
stories15M first-chunk discriminator.

Reason: L59 now proves that authenticated partial response streams survive
failure before worker/key/root cleanup. Its authorized replacement never
launched the model because systemd still had the fixed device-gate unit fragment
loaded from the prior admission failure. The controller lacked a pre-launch
proof that fixed transient names were fully unloaded. This is an operational
lifecycle defect, not inference evidence.

L60 must require correct-manager `not-found/inactive/dead/MainPID0` authority,
no cgroup/process/listener/fragment, and bounded reconciliation for safely
inactive stale units before launch; uncertainty or active ownership refuses.
Focused lifecycle tests and one no-model stale-unit exercise precede one
first-armed stories chunk. Unambiguous pre-model controller mechanics may be
corrected within L60 after focused review to avoid repetitive milestone churn.
Once model runtime starts, no retry is authorized. No cache matrix, primary
artifact, production mutation, or performance work is allowed.

Reason: both L36 attempts read `llama_n_batch(run_ctx)` after freeing the
aliased context, producing 0 and then 3386108400. Neither is valid admission
evidence. Pre-free records directionally show equal recorded graph/KV authority
but different logits, so the next proportional work is to make result emission
lifetime-safe and authenticate currently uncovered masks, indices, mutable
graph leaves, scheduler assignments/copies, RPC bindings, and derived FA/Q8
inputs before any further primary transition.

## 2026-07-25 — permit one corrected L36 after admission-only failure

Decision: preserve the first L36 run as rejected admission evidence and
authorize one corrected execution after a focused reviewed gate repair. Honest
fresh restore must require `n_batch=0` independently of the enabled compatible
diagnostic layers; capture remains frozen at `n_batch=512`. No other gate,
matrix, alternate mode, or further repeat is authorized.

Reason: both primary residencies completed, but the runner rejected the restore
before replay-authority interpretation because `require_result` coupled the
known lifecycle value to `SEMANTIC_DIAGNOSTICS_ONLY`. Source inspection confirms
that the combined diagnostics path incorrectly expected 512. This run did not
test the intended L36 discriminator, so one narrowly reviewed correction is
proportionate. Production recovered exactly before authorization.

## 2026-07-25 — accept L35 and authorize one L36 primary authority record

Decision: accept reviewed L35 PASS commit
`93c3ae313b86aa0bfddd2c5a1a8745223cb256ac`. Authorize one ordinary exact-
primary, two-residency, one-token run with the authenticated L35 replay-
authority and logits record. Do not run a canonical-reset variant, matrix,
retry, correction, promotion, or L37.

Reason: L35 disproves graph-history reuse and KV prepare/apply divergence on the
disposable lifecycle while qualifying a concrete primary discriminator for
graph, scheduler, KV physical authority, attention views, output mapping, and
synchronized logits. This one new record can localize the primary mismatch
without repeating broad correctness testing.

## 2026-07-24 — accept L34 and require a disposable graph/KV discriminator

Decision: accept reviewed L34 PASS commit
`fc8517ffc473220d74ee27b6eb111d4be7fefd82`. Open L35 only for a source-derived
mutable-state coverage census and one combined disposable test of graph history,
scheduler rebuild/reuse, KV prepare/apply physical targets, allocator authority,
attention inputs, and synchronized replay logits. No primary run or L36 is
authorized.

Reason: L34 proves exact once-only final-token replay and fresh logits behavior
on the accepted fixture. Independent source audits agree that the strongest
unmeasured differences are capture-versus-restore graph/scheduler history and
single-sequence KV physical allocation/cursor behavior. Resolving these on a
disposable path before another primary transition avoids repetitive expensive
testing and produces a concrete discriminator.

## 2026-07-24 — accept L33 and open semantic-state source diagnosis

Decision: accept reviewed L33 NOT PROMOTED commit
`83ce2b5a449fa68d7864d8e0d31bf85c8edfc0ed`. Open L34 only for source tracing,
focused disposable/synthetic discriminators, and independent audits of semantic
inputs to the first sampled token outside the proven-equal state contract. No
primary run, production mutation, broad matrix, or L35 is authorized.

Reason: original, restore-input, live-recapture, and pre-generation coordinator
state are equal, as are all worker components, but output differs. More byte
integrity testing is not useful. The next highest-value questions are whether
the final prompt token is replayed exactly once, whether logits are valid and
from the same decode, and whether positions, sampler input, or primary-specific
runtime state lies outside sequence serialization. Two independent read-only
specialists were assigned to challenge the main diagnosis.

## 2026-07-24 — resume with one L33 primary live-state discriminator

Decision: resume from reviewed L32 commit
`601479a9cf7c18f81a37187663decece47f5fb05` and authorize exactly one primary
two-residency, one-token run with authenticated original, restore-input, live
recapture, and pre-generation state comparison. No matrix, retry, tuning,
promotion, speculative correction, or L34 is authorized.

Reason: L32 qualified the missing live coordinator/worker recapture boundary on
the disposable lifecycle. One primary run is now the smallest test that can
distinguish live-state application/mutation from semantic state omitted by the
current serializer contract. The user explicitly approved continuation while
requiring a high-quality production path and avoidance of repetitive testing.

## 2026-07-23 — stop cleanly after L32 pending user approval

Decision: record reviewed L32 PASS commit
`601479a9cf7c18f81a37187663decece47f5fb05` and send no new worker task. Await
explicit user approval before any primary discriminator or L33 work.

Reason: L32 qualified authenticated live coordinator/worker recapture on the
disposable two-residency lifecycle without touching production. The remaining
discriminator requires another primary transition. The user requested a
natural stopping point so the computer can be shut down.

## 2026-07-23 — accept L31 and diagnose live coordinator state

Decision: accept reviewed L31 NOT PROMOTED commit
`60f4272c4a9f0ecb9e365e0c32e697513668d043`. Open L32 only for default-off
post-apply live-state recapture, source audit, focused tests, and disposable
qualification. No primary load or production transition is authorized.

Reason: worker capture/stage/live-apply identity and content are now exact, and
saved coordinator input receipts match, but the restored token remains wrong.
Input-blob equality does not prove that live coordinator control/local state was
applied completely or remained intact before generation. L32 must compare an
independent post-apply recapture with the original capture and inspect sequence,
KV-cell, local tensor, recurrent, and architecture-specific memory authority.

## 2026-07-23 — close L31 after corrected primary token mismatch

Decision: direct L31 to close NOT PROMOTED after immutable component evidence,
cleanup, and independent review. Do not retry, change semantics, or open L32.

Reason: the one authorized corrected primary restore still produced a different
first token. Production recovered exactly. The authenticated per-component
evidence was retained before token rejection, so the next diagnosis must be
chosen only after reviewed capture/stage/apply mismatch localization rather
than assuming whether the Q8 correction was insufficient or another boundary
is responsible.

## 2026-07-23 — accept L30 and authorize one corrected L31 primary restore

Decision: accept reviewed L30 PASS commit
`b630c4f52c849af8cd8ebd30451a8c1268979ce3`. Authorize one primary transition
with two fresh residencies and one reference/restored token under the corrected
Q8_0 apply path. No additional case, retry, tuning, promotion, or L32 is
authorized.

Reason: L30 proves that live apply treated quantized storage blocks as scalar
elements, restoring only 34 of 1,088 bytes in the representative Q8_0
component. The corrected checked block geometry produces exact
capture/stage/apply bytes on isolated Q8 RPC and view fixtures and preserves
exact disposable two-residency output. One primary confirmation is now the
smallest proportional correctness gate.

## 2026-07-23 — accept L29 and localize stage-to-live application

Decision: accept reviewed L29 NOT PROMOTED commit
`8b54091efe456c8222528ec455316afbca8c8562`. Open L30 only for bounded
component-level instrumentation, source analysis, synthetic apply tests, and
optional disposable qualification of the stage-to-live boundary. No primary
load or production transition is authorized.

Reason: capture and validated staging have identical worker aggregates, while
the live post-apply aggregate differs and coordinator receipts remain equal.
The output mismatch therefore has a retained worker-side boundary before
generation. L30 must identify exact divergent components and test alias/view
ranges, strides, offsets, Q8 geometry, apply order, RPC addressing, and backend
synchronization without assuming which mechanism is responsible.

## 2026-07-23 — resolve L29 unit alarm as wrong systemd scope

Decision: do not invoke recovery or start duplicate services. Require L29
closeout and future production probes to query the system manager explicitly
and retain cgroup ownership evidence.

Reason: the listeners, commands, and HTTP endpoint were healthy while a
`systemctl --user` query reported the system units not found/inactive. Exact
`/proc/<pid>/cgroup` paths and system-scope properties prove both named units
are loaded active/running with correct MainPIDs and zero restarts. The alarm
was a probe-scope defect, not a production outage.

## 2026-07-23 — close L29 after fresh-residency output mismatch

Decision: direct L29 to close NOT PROMOTED after immutable evidence, cleanup,
and independent review. Do not retry, implement a correction, or open L30.

Reason: the single authorized primary transition completed both valid fresh
worker/model residencies and reached exact one-token comparison, but capture
and restore token/text hashes differed. The runner failed closed and restored
production worker-first. The next lane depends on the final authenticated
capture/stage/apply worker aggregates and coordinator receipt digests, so no
causal interpretation is admitted before reviewed closeout.

## 2026-07-23 — accept L28 and authorize one L29 primary discriminator

Decision: accept reviewed L28 PASS commit
`09123048522281025afb532715f20457ac4b9918`. Authorize one primary transition
with exactly two material model residencies and one capture/restored token.
No additional case, retry, correction, tuning, promotion, or L30 is authorized.

Reason: L28 wires the proven RPC epoch rule into the actual pre-staging path.
Capture model A terminates before worker A, worker B is distinct and current,
fresh model B loads entirely against B, and authenticated capture
object/epoch-A lineage plus target epoch B are validated before staging. The
disposable result is exact. This is now the smallest valid primary experiment
that can compare capture/stage/apply worker bytes without stale RPC handles.

## 2026-07-23 — accept L27 and wire a fresh-residency restore runner

Decision: accept reviewed L27 PASS commit
`5616abb2c19c1611c3852575270ad41b43085921`. Open L28 only to wire the worker
epoch/model-allocation epoch validator into a runnable two-residency
capture/restart/restore path and qualify it on the disposable model. No
production transition or primary load is authorized.

Reason: L27 reproduced the L26 failure with a same-process coordinator after
worker restart and proved exact restore with a fresh coordinator/model
residency. RPC remote buffers and graph handles are worker-process-local; CAPS
alone cannot refresh stale model allocations. The valid lifecycle is capture
under worker/model epoch A, terminate coordinator A before worker A, start
worker B, load fresh coordinator/model residency B, then stage/apply state
only after B's allocation epoch matches B's worker authority.

## 2026-07-23 — accept L26 and diagnose RPC restart residency authority

Decision: accept reviewed L26 NOT PROMOTED commit
`9b45bb9c844ec224fbd6fc3b39bdfe23eec11ee3`. Open L27 only for source tracing
and a disposable small-model comparison of same-coordinator-residency behavior
after worker restart versus a fresh coordinator/model residency. No production
transition or primary load is authorized.

Reason: L26 retained a valid authenticated capture, then failed before state
staging while a still-resident coordinator created a context against the
restarted RPC worker. A fresh CAPS handshake does not necessarily reconstitute
server-side weight buffers or remote allocation handles. Earlier accepted
restart cases may have used a new coordinator process/model residency. L27 must
prove or reject this lifecycle distinction and, if proven, add only fail-closed
epoch authority rather than attempting transparent weight rehydration.

## 2026-07-23 — close L26 after post-restart RPC/context failure

Decision: direct L26 to close NOT PROMOTED after immutable evidence, disposable
cleanup, and independent review. Do not retry or open L27.

Reason: the one authorized run captured primary state and restarted the
disposable worker, but the coordinator aborted during post-restart context/KV
allocation after RPC reported a crashed or malformed response. This prevented
stage/apply/token discrimination and is terminal under the one-run contract.
The bounded controller recovered exact production worker-first and
coordinator-second to HTTP 200. A transient mid-load observation was reconciled
against final exact named-unit, cgroup, PID, command, listener, and restart
authority.

## 2026-07-23 — accept L25 and authorize one L26 primary discriminator

Decision: accept reviewed L25 PASS commit
`36b026d29454adc9cdd61baf387303c3e8d9f200`. Authorize one primary model load
for capture, true disposable-worker restart, restore, and one-token diagnostic
comparison under the repaired bounded transport. No other case, retry,
semantic correction, tuning, promotion, or L27 is authorized.

Reason: L25 closes the exact transport and evidence-durability defects that
invalidated L24. Every SSH process tree now has a local deadline, typed fsynced
evidence, bounded termination/reaping, and recovery continuation; capture
evidence is authenticated before restart. The unfinished primary discriminator
is again the smallest experiment capable of separating worker byte
transport/application from coordinator or model-specific state semantics.

## 2026-07-23 — accept L24 and open no-production controller reliability work

Decision: accept reviewed L24 NOT PROMOTED commit
`46461c888b79e5496c4999c38bae749377dc1966`. Open L25 only to implement and
qualify bounded local deadlines for every controller-owned SSH process group,
typed timeout evidence, orphan-free recovery continuation, and authenticated
capture-output flushing. No production transition or primary load is allowed.

Reason: L24 captured valid primary worker state but could not authoritatively
observe its own capture-ready marker because its SSH subprocess hung. The first
recovery SSH probe hung identically while production was down. Manual
termination of only that subprocess allowed registered recovery to succeed.
This is a controller-liveness defect that must be closed independently before
any further primary discrimination.

## 2026-07-23 — close L24 after recovered controller SSH hang

Decision: accept the completed worker-first/coordinator-second emergency
recovery and direct L24 to close NOT PROMOTED after immutable evidence,
cleanup reconciliation, and independent review. Do not retry or open L25.

Reason: the single authorized transition captured primary state but hung in a
controller-owned SSH readiness probe; recovery initially hung in another SSH
probe while both production services were inactive. Clearing only the stuck
subprocess allowed the same recovery trap to restore exact production
authority to HTTP 200 with zero service restarts. The SSH hang and recovery
behavior are material controller-reliability evidence and must be retained.

## 2026-07-23 — accept L23 and authorize one L24 primary discriminator

Decision: accept reviewed L23 commit
`cb1913ca233acf8661530622720b411bc0e5d5aa`. Authorize one controlled primary
model load containing only capture, disposable worker restart, restore, and
minimum deterministic token comparison with diagnostics enabled. No cold or
fault matrix, retry, tuning, correction, promotion, or L25 is authorized.

Reason: the default-off diagnostics prove equal capture/stage/live-apply worker
aggregates on the disposable fixture, but only the pinned primary tuple can
discriminate the L22 failure. L24 must retain all three worker aggregates and
authenticated coordinator receipt digests. A worker digest mismatch localizes
the defective boundary; equal worker digests with first-token divergence
narrows the remaining fault without overclaiming a root cause.

## 2026-07-23 — accept terminal L22 and open no-production L23 diagnosis

Decision: accept L22 NOT PROMOTED commit
`4d2821b3a318d2d38f93a30aa2f3a2263cc4d01d`. Open L23 only to identify the
earliest restored-state divergence using retained evidence, source tracing,
offline instrumentation, focused tests, and the accepted disposable fixture.
Do not load the primary artifact, mutate production, retry L22, or tune.

Reason: the rank-local persistence mechanism captured and restored the expected
byte/component counts without state-page GET/SET, and every cold path remained
exact, but the true restart-restored continuation differed. This is a hard
correctness failure. L23 must distinguish component completeness/order,
sequence/KV metadata, allocation/layout authority, Q8_0 behavior, flash
attention, architecture-specific state, and rank ownership through evidence,
not speculation. It may deliver a reviewed narrow default-off correction or a
precise smallest discriminating primary experiment; the latter needs a new
Project Lead decision.

## 2026-07-23 — require manifest-to-process argv binding before L22 mutation

Decision: retain the accepted fixture evidence and uncommitted L22 work, but
block all production mutation until the actual normalized command passed to
`subprocess.Popen` is bound exactly to the accepted closed manifest before
prepare or shutdown. Authorize only focused argv/evidence-root refusal tests,
hash refresh, disposable dry-run/cleanup evidence, and independent re-review.

Reason: the fallback fixture passed the required lifecycle, but independent
review found a shared controller defect: manifest `child_argv` could describe
one child while caller-controlled `maintenance_command` executed another after
production shutdown. This is a real authority gap, not a fixture issue. The
repair must eliminate interpreter, argument, ordering, separator, path, and
evidence-directory ambiguity. The original single primary-attempt authority
survives only after accepted re-review.

## 2026-07-23 — keep L22 open for one compatible-fixture qualification

Decision: preserve the uncommitted L22 controller work and authorize one narrow
no-production fixture correction. Prefer an already-local, previously accepted
small model whose head dimensions support Q8_0 KV and the normal RPC path.
Qualify one RPC graph evaluation and then only the minimum three-residency
lifecycle. If no such fixture exists, permit F16 KV with flash attention off
solely as a non-representative controller/lifecycle qualification.

Reason: both failures occurred before production mutation and are specific to
the disposable 15M fixture configuration. Its 48-wide heads cannot represent
Q8_0 KV blocks, while the F16 substitution reached a flash-attention abort.
Neither result is evidence against the pinned primary model or the persistence
contract. L22 must not expand into kernel work or broad model testing. The
primary configuration remains unchanged and the original one-attempt authority
survives only after the narrow fixture gate passes and is reviewed.

## 2026-07-23 — accept L21 and authorize one guarded L22 primary canary

Decision: accept L21 commit
`851dc6f1af55c856532a5908516ebed9a5679891`. Authorize one L22
controller-managed maintenance transition for the pinned 160 GB ROCmFPX model
and exact cache correctness canary. This is not production enablement or final
performance promotion.

Reason: L21 closes every execution/evidence defect that rejected the L20
controller. The accepted v9 archive binds the real refusal to exact process and
journal authority, makes all evidence and cleanup mandatory, proves unchanged
production, and passed independent review. The already measured three-residency
lifecycle is the minimum honest cache canary and must not be broadened.

L22 must use the closed manifest/controller, exact model hash and primary
request, explicit `RPC0,ROCm0` placement, the accepted three-residency sequence,
and one maintenance transition. It must prove exact uninterrupted/restored/cold
suffix equality, rank-local immutable objects, zero legacy state-page GET/SET
during capture/restore, bounded corruption/missing/plan-mismatch cold fallback,
actual allocation and timing evidence, and full worker-first production
restoration. Any failure closes L22 without an automatic retry. No unrelated
tuning, cache promotion, broad matrix, or L23 work is authorized.

## 2026-07-23 — replace periodic monitoring with worker-reported events

Decision: delete the 30-minute project-lead heartbeat and use native worker
completion/attention events exclusively. Keep L21 scope and acceptance criteria
unchanged.

Reason: a worker can reliably report the boundaries that require management
attention without periodic snapshots. The primary worker was instructed to end
its task turn on milestone completion, required authority, unsafe or uncertain
production state, regression, scope expansion, authoritative-context loss, two
materially different failed approaches to one blocker, or defined no-progress
windows. Ordinary healthy progress requires no management activity.

## 2026-07-20 — retain the current primary worker

Decision: retain the existing multiday HaloFPX task as primary implementation
owner.

Reason: despite its age, the worker uses current repository/node evidence,
obeys risk-proportionate testing steering, rejects slower code, restores the
known-good service, and is not repeating closed work. Context age alone does not
justify a disruptive handoff.

Trigger to revisit: contradictory state claims, repeated closed-lane work,
ignored steering, unsafe promotion, or three consecutive checks on the same
blocker without a materially new approach.

## 2026-07-20 — no P13 steering

Decision: do not intervene in P13.

Reason: the worker measured the micro-kernel gain, translated it to an estimated
whole-token contribution of only about 0.1%, and chose to close integration.
That is aligned with the project objective and prior speed steering.

Outcome: P13 committed cleanly as `ea49690a`. The default-off proof remains;
product integration is closed because the projected whole-token contribution is
only about 0.1%. No follow-up steering is required.

## 2026-07-20 — accept P14 rejection and L10 product pivot

Decision: no steering; accept the P14 row-split rejection and observe the L10
exact-key operational cache canary.

Reason: P14 used one bounded exact-output screen, found mixed/noise-scale prompt
and generation results with generation slightly worse, rejected the candidate,
restored production, and committed the evidence. L10 addresses a recorded
product gap rather than opening another marginal performance permutation: the
current cache is laboratory-only because clients must provide a manifest handle.
The proposed private authenticated exact-key lookup remains default-off,
generation-one, non-enumerating, non-prefix, and no-overwrite.

Trigger to revisit: L10 broadens into shared/prefix discovery, permits overwrite,
weakens authenticated fixed-anchor authority, mutates live state before complete
validation, or expands testing beyond the bounded canary without a defect.

## 2026-07-20 — accept L10a authenticated selection boundary

Decision: no steering; accept L10a and observe the L10b runtime canary.

Reason: the first attempt exposed a genuine positive-path failure and was held
open rather than promoted. The repaired seam authenticates and parses the fixed
anchor before revealing the selected manifest, rejects wrong scope and corrupt
anchors as misses, performs no directory scan, and has no server runtime edge.
Focused and inherited nimo-2 tests passed 4/4, independent review accepted the
milestone, the tree is clean at `975b1550`, and production recovered.

Trigger to revisit: normal-path writeback occurs before a clean prompt boundary,
restore mutates live state before full validation, misses fail to recompute cold,
or feature-off/default behavior changes.

## 2026-07-20 — accept L10b exact-session authority boundary

Decision: no steering; accept L10b and observe the separate L10c server canary.

Reason: L10b keeps request identity target-owned and opaque, derives its lineage
from authenticated private scope and exact canonical inputs, fails closed on
profile ambiguity, and remains library-only. Its 3/3 focused Linux tests and
inherited authentication/scope controls passed; independent review corrected
two defects before the clean `d7950c43` commit. Ordinary server behavior remains
unchanged because the runtime edge is deliberately deferred to L10c.

Trigger to revisit: L10c accepts caller-chosen cache identity, publishes before
a completed cold prompt boundary, mutates live state during validation, fails to
fall through cold on any cache error, changes feature-off behavior, or touches
the known-good deployment before its disposable canary passes.

## 2026-07-20 — use milestone completion events and adaptive fallback timing

Decision: major worker milestones end their task turn, providing a reliable
completion event. Retain one durable 30-minute heartbeat only as a fallback and
predict manual review timing from the active phase.

Reason: native task waits wake for completion or attention, not ordinary
commentary. Repeated ten-minute waits therefore consumed manager activity while
adding little control value. Recent work indicates source/build/focused-test
boundaries typically justify a 30–45 minute expectation, while model loading and
runtime qualification justify 60–90 minutes or longer. The worker was asked to
finish its current turn at the L10c boundary rather than immediately opening the
next milestone.

## 2026-07-20 — accept L10c and separate multi-entry admission from eviction

Decision: accept L10c at `d0694cd5`; open the next scoped lane only for bounded
authenticated multi-entry exact-key admission and selection. Keep online
deletion, eviction, generation replacement, prefix matching, and shared scope
closed.

Reason: L10c proves normal-request miss, prompt-boundary publish, restart, exact
hit, changed-key cold fallback, no-publication under reserve exhaustion, and
feature-off preservation. Production is still blocked by the one-entry limit.
Multi-entry selection is the smallest product-enabling next capability, but
combining it immediately with deletion or eviction would open separate active-
reference, reachability, privacy, crash-recovery, and administrator-authority
risks. A full catalog should reject new writes safely while leaving inference
cold when capacity is exhausted.

Coordination outcome: automatic goal continuation opened a post-L10c turn
despite the requested stop boundary. The manager issued an explicit stop while
the tree was still clean; the worker acknowledged that no post-L10c changes
were made and is now idle pending the scoped continuation.

## 2026-07-21 — accept L10d and prioritize distributed-state truth

Decision: accept L10d at `6862ffb9`. Before opening eviction or deeper local
cache administration, audit the exact current RPC/tensor-split serialization
boundary and attempt a separately gated two-rank restore canary only if the
observed ownership model can be made fail-closed.

Reason: L10d now supplies bounded multi-entry exact-key reuse, safe capacity
fallback, private authentication, immutable records, and unchanged feature-off
behavior. The real primary workload is a 160 GB model split across nimo-1 and
nimo-2, but the existing cache milestone explicitly excludes distributed
restore. Canonical Wiki section 58 records as open whether the present sequence
blob is global or rank-local. Solving or precisely blocking that dependency
advances the actual product more than adding local eviction to a cache that
cannot yet serve the target topology.

Boundary: first trace code and measure state ownership on a disposable two-rank
fixture. Do not infer completeness from a successful API call. If safe, require
exact plan/rank binding, all-rank readiness before live mutation, full cold
fallback for any mismatch/failure, exact suffix equivalence, and no state pages
over the control plane. Keep production, eviction, shared reuse, and final
primary-model claims closed.

## 2026-07-21 — accept L11 blocker and hand off the new RPC protocol phase

Decision: accept the documentation-only L11 blocker at `78a102ac`; replace the
multiday primary worker with fresh task `019f83a3-9498-76c3-9398-be80344854ae`
for the worker-local persistence protocol. Preserve the prior task idle as the
historical implementation/handoff record.

Reason: L11 measured that current capture and restore move worker KV pages via
RPC GET/SET. A compatibility wrapper cannot convert this into rank-local SSD
ownership. The next work is therefore a new wire, storage, readiness, and
commit-live protocol rather than incremental L10 catalog work. The prior worker
remained accurate, but this clean architecture boundary benefits from fresh
context after several days and multiple compactions.

The fresh worker is authorized only for the smallest Linux/default-off protocol
and small disposable two-host canary. It must keep coordinator-local/sampler
ownership explicit, bind exact plan/topology/ranks/components, transfer only
bounded authenticated identifiers/status, validate before mutation, require
all-rank readiness, and cold-recompute after every failed or partial attempt.
Primary-model qualification waits for a reviewed small canary.

## 2026-07-21 — accept L12 and authorize one bounded primary-model canary

Decision: accept L12 implementation `6444d1e1` and evidence/docs `51922809`.
Authorize one disposable canary using the pinned 160 GB Q6_0_ROCMFPX MiniMax
artifact and exact primary workload. Do not enable the feature in production.

Reason: the small two-host proof now satisfies the architectural precondition
that blocked L11: the worker writes and stages its own immutable object, control
messages contain no state pages, all-rank authority is transcript-bound, and
failed attempts destroy/recreate the disposable context before cold fallback.
The next uncertainty is scale and exact-model applicability, not another local
protocol permutation.

Current runtime authority was checked directly: nimo-1 currently coordinates
the standard 193 GiB UD-Q6 model on 8081 and nimo-2 serves RPC on 50052. The
ROCmFPX primary artifact previously qualified for matched performance resides on
nimo-2, so the disposable primary canary may reverse roles temporarily. The
rollback contract is explicit: preserve current unit/command/PID/health evidence,
stop coordinator before worker, and restore nimo-2 worker first followed by
nimo-1 coordinator until HTTP 200 and service restart counters are reconciled.

The bounded canary must measure exact output, capture/restore/local-object bytes,
state-operation GET/SET count, prompt/generation timing, and a small matched
feature-off or mode-off cold control. A non-worse point estimate is milestone
evidence only; final G9/G10 still requires the later full statistical gate.

## 2026-07-21 — accept L13 safety stop and require executable retry guards

Decision: accept L13's negative reviewed closeout at `519a4400`. Do not abandon
the primary canary, but permit only one conditional retry after two prerequisites
pass before any production service is stopped.

Reason: the first failure is a harness batching defect, not a model/protocol
failure; the 1,128-token saved prefix was submitted as one decode against
`n_batch=512`. The committed chunking correction remains unqualified. The
second failure was more serious: an operator command targeted the wrong host,
stopping the production worker while its coordinator was live. The existing
prose runbook was insufficient. Production recovered and no cache state was
published, but future transitions must be enforced mechanically.

Prerequisite A: run the exact corrected canary path with `count > n_batch` on a
disposable small model and prove successful capture/restore/cold equivalence.
Prerequisite B: add a single host-bound transition controller that validates
remote hostname and unit role, refuses to stop nimo-2's worker until nimo-1's
coordinator is inactive and its listener is closed, and restores nimo-2 worker
and port 50052 before nimo-1 coordinator and HTTP 200. Dry-run and disposable
failure cases must demonstrate refusal on swapped hosts/roles.

Only after both prerequisites pass focused review may the same task perform one
primary-model retry. No repeated blind retries, manual ad hoc stop sequence, or
expanded test matrix is authorized.

## 2026-07-21 — accept terminal L13R and isolate application readiness

Decision: accept L13R closeout `aa3c2cf6` as not promoted. Do not authorize a
new primary run yet. Open one no-production milestone to replace disposable
worker TCP-listener readiness with an application-level RPC CAPS handshake.

Reason: both prior prerequisites passed, and the controller performed the real
shutdown/recovery in the correct dependency order. The retry failed 1.356
seconds after starting the disposable worker: its socket and MainPID were
visible, but no RPC ready banner, accepted client, model load, or state command
appeared. The immediate explanation is a listener-visible-before-application-
ready race. This is new and can be resolved without another maintenance window.

Acceptance for the readiness milestone: a bounded probe must retry until the
worker answers the exact expected CAPS version/limits, fail closed on timeout,
wrong endpoint/version/capabilities or early disconnect, and be proven against
an artificial listener-first service plus a real disposable ROCm worker while
production stays HTTP 200. Only an independently reviewed readiness gate may
justify a later Project Lead decision about another primary canary.

## 2026-07-21 — accept L14 and authorize one L15 primary canary

Decision: accept L14 implementation `b688680e` and reviewed closeout
`a496492c`. Authorize one controller-managed primary-model canary using the
exact application-level readiness gate.

Reason: the defect that stopped L13R is now reproduced and closed. Admission no
longer depends on TCP/listener/systemd state; a worker must complete exact RPC
HELLO and return the expected runtime-bound CAPS tuple. The delayed-listener
fixture and real disposable ROCm worker demonstrate the distinction. Combined
with the previously accepted long-prompt proof and host-bound transition
controller, the primary workload now has all known operational prerequisites.

L15 must build from the current reviewed source and use the pinned 160 GB
ROCmFPX artifact/request with no unrelated tuning. It gets one maintenance
transition and one canary execution. The controller owns shutdown and rollback;
the worker must be CAPS-ready before the coordinator starts. Acceptance remains
exact suffix equality, rank-local objects, zero state-payload GET/SET, bounded
cold fallbacks, recorded state I/O/timing, and a small matched runtime-off cold
screen. Any failure closes L15 without an automatic repeat.

## 2026-07-21 — accept terminal L15 and gate one L16 attempt on key provisioning

Decision: accept reviewed closeout `0db5a561` only as L15 NOT PROMOTED. Authorize
one separately named L16 primary canary only after a no-production prerequisite
makes protected channel-key provisioning an executable pre-mutation gate.

Reason: L15 did not exercise the worker protocol or primary model. The local
readiness probe rejected nimo-2's mode-0644 expected-channel key before opening
a connection. Source inspection identifies an SSH `bash -c` argument-boundary
error: redirection escaped the intended `umask 077`. Recovery was correct and
production is healthy, so this is a bounded harness defect rather than model or
state evidence. Repeating the same operational shape with a trailing chmod would
leave key preparation after shutdown and is not sufficient.

The prerequisite must provision identical fresh key bytes to both isolated
hosts without ambiguous remote shell composition and prove regular-file type,
expected owner, exact mode 0600, expected size, and equal digest without logging
the key. The controller must execute or validate this gate before first mutation,
and a wrong-mode fixture must prove refusal with production unchanged. Exercise
the exact remote path while production remains HTTP 200, add focused tests, and
obtain independent review. After those conditions pass, L16 gets one guarded
transition with the unchanged pinned model, request, result gates, and rollback
contract. Any L16 failure closes the milestone without automatic repetition.

## 2026-07-21 — accept terminal L16 and isolate placement authority in L17

Decision: accept reviewed closeout `20f19a2d` only as L16 NOT PROMOTED. Open
L17 as a no-production device-discovery and layer-placement authority milestone.
Do not load the primary model, stop production, or automatically open another
primary attempt.

Reason: L16 passed secure key provisioning and exact HELLO/HFXCAP2, then its
first capture load requested a single 159,231,007,232-byte RPC0 buffer from a
worker with 133,143,986,176 bytes total. This proves the current placement is
inadmissible, not that aggregate two-host memory is insufficient. Prior P01 and
P11 runs loaded the exact model successfully with explicit device order
`RPC0,ROCm0`, layer split, and tensor split `1,1`. The L16 canary carried both
split settings but omitted the explicit `--device RPC0,ROCm0` argument. That is
the leading explanation, not yet a verified conclusion.

L17 must make device order and placement executable authority rather than a
comment or command-line assumption. Freeze the exact explicit device argument,
verify parsed and runtime-discovered names/backend types/order/count, and prove
the intended repeating-layer ownership distribution before allocation. Exercise
the exact path against an isolated disposable nimo-1 RPC worker while normal
production remains HTTP 200, then load a small disposable model with the same
two-device ordering and prove both devices receive bounded buffers and exact
output. Include focused omission/wrong-order/one-device/refusal tests and one
independent adversarial review. Commit a clean L17 result and stop. Only a later
Project Lead decision may authorize another primary maintenance transition.

## 2026-07-21 — accept L17 and require exact-primary allocation preflight

Decision: accept reviewed L17 at `730e9633`. Open L18 only as a read-only,
no-production exact-primary allocation-planning milestone. Do not authorize a
primary load or production transition.

Reason: L17 closes the executable-authority gap that caused the L16 command to
omit explicit `RPC0,ROCm0`. Its common loader/probe resolver, focused refusal
matrix, real isolated RPC exercise, and two exact-output small-model runs prove
the intended 32/31 ownership and nonzero allocations on both devices. The
independent re-review has no remaining P1/P2 issue, and production remained
unchanged and healthy. However, L17 explicitly does not inspect or allocate the
159.9 GB primary artifact; it cannot prove actual tensor grouping, maximum
buffer requests, or per-device capacity margin.

L18 must use the pinned artifact identity and exact metadata, explicit
`RPC0,ROCm0`, layer split, and `1,1` tensor split. Its planner must share or
directly exercise the real loader's placement/grouping authority, report every
material per-device weight allocation group plus the largest and total request,
state KV/compute/reserve assumptions separately, and refuse unknown devices,
unaccounted tensors, arithmetic overflow, or insufficient margin. It may read
the model and query hardware but may not allocate the primary weights, stop or
restart production, write cache state, or make a performance claim. Retain raw
evidence and obtain one independent adversarial review. Only a later Project
Lead decision may authorize a primary maintenance attempt.

## 2026-07-21 — accept L18 and authorize one guarded L19 primary canary

Decision: accept reviewed L18 at `93c61eadd167285be448ef1e99b80f429fa4299a`.
Authorize one controller-managed L19 maintenance transition for the exact
primary model and cache correctness canary. L19 is not a production enablement
or performance promotion.

Reason: L18 binds the pinned artifact hash and exact `RPC0,ROCm0` loader path,
accounts for 809/809 tensors, and obtains the real loader's three allocation
groups. The planned weight groups plus simulated context/compute, 10%
fragmentation assumption, and 16 GiB reserve leave 26.50 GB RPC0 and 28.41 GB
ROCm0 margins against reported total capacity. It also corrects L17's
resolver-only output prediction: `output.weight` belongs to RPC0. Independent
review found no P1/P2 issue, no material primary allocation occurred, production
was unchanged, and the evidence/cleanup boundary is complete.

L19 boundary: use commit `93c61ead`, the exact 159,873,097,824-byte artifact and
pinned 1,129-token/128-generation request, explicit `RPC0,ROCm0`, layer split,
and tensor split `1,1`. Before mutation, the controller must revalidate host/unit
roles, secure equal channel keys, exact HELLO/HFXCAP2, model identity, current
known-good service authority, and a recoverable rollback path. Stop the
coordinator before its worker and restore worker-first, coordinator-second to
HTTP 200. The canary gets one material load and one capture/restore/cold
sequence. Record actual per-device allocation evidence, exact uninterrupted /
restored / cold suffix equality, rank-local object bytes, zero legacy state-page
GET/SET during capture and restore, state I/O/timing, and a bounded mode-off cold
control. Any error, mismatch, insufficient margin, or partial readiness must
cold-recompute or abort and then restore production. No repeated retry,
unrelated tuning, cache promotion, L20 work, or final performance claim is
authorized. The worker must commit a reviewed terminal result and end its task
turn.

## 2026-07-21 — accept terminal L19 and isolate the execution contract in L20

Decision: accept L19 terminal NOT PROMOTED at
`7cb42be0ba3f45863c418fb9befd5d306f5ce893`. Open L20 only as a no-production
controller/runner lifecycle prerequisite. Do not authorize another primary
load or maintenance transition.

Reason: the L19 pre-mutation review found that the inherited six-mode runner
would start six processes and therefore material-load the 159.9 GB model six
times, exceeding the literal one-load authority. It also found that the
transition controller still binds L16 key paths, child identity, and disposable
cleanup names, and that allocation-failure evidence begins too late. These are
real execution-contract defects. The stop gate worked: no production unit,
listener, key, model, inference, or cache state was touched; production remained
healthy and focused tests passed 52/52.

L20 boundary: first derive and document the minimum material-load lifecycle
needed to preserve exact uninterrupted, cold, restart-restored, missing-object,
plan-mismatch, and mode-off controls. Prefer one process and multiple contexts
within each model residency; if restart semantics inherently require another
model residency, prove that dependency and freeze the minimum count rather than
silently weakening the restart test. Parameterize the controller's milestone
identity, channel-key paths, child binary/unit names, and cleanup allowlist as a
single closed manifest; reject unknown or inconsistent identities before any
mutation. Start journal/PID/allocation/disk evidence capture before the child can
allocate or fail. Prove the lifecycle and abnormal cleanup on a small disposable
two-host model with production continuously unchanged, run focused tests, obtain
one independent adversarial review, commit a terminal L20 result, and end the
task turn. No primary artifact load, production transition, performance claim,
cache promotion, unrelated tuning, or L21 work is authorized.

## 2026-07-21 — accept L20 lifecycle evidence and target the rejected contract

Decision: accept the measured three-residency lifecycle in terminal L20 commit
`e2edc4b3277f5385118e759ed9f89c1ea0a7445a`, but do not promote its removed
controller candidate. Open L21 only as a no-production correction of the five
review findings. Do not authorize a primary artifact read/load or production
transition.

Reason: the disposable two-host fixture directly proves that an honest complete
canary needs three current model residencies rather than six: capture plus clean
cold; post-worker-restart restore plus corrupt/mismatch fallbacks; and a
feature-off cold control. Exact suffix hashes matched and state windows carried
zero legacy tensor-page operations. The independent review nevertheless found
that the controller had no real early allocation-refusal evidence case, omitted
source/build/state roots from manifest-owned cleanup, could lose InvocationID
authority after `systemd-run --collect`, allowed evidence failures to remain
nonfatal, and lacked a production-before snapshot. The candidate source was
correctly removed and production remained unchanged.

L21 boundary: retain the already proven three-residency semantics and do not
repeat or broaden its correctness matrix. Build one closed milestone manifest
that owns every disposable source, build, state, key, evidence, unit, port,
process, and child identity. Exercise one real early allocation-refusal child
through the same evidence collector. Capture InvocationID before collection and
also retain a journal cursor/time lower bound that fails closed if exact unit /
PID / invocation authority cannot be reconciled. Every evidence command,
archive step, and cleanup verification must be mandatory; any failure makes the
result non-promotable. Bind a fresh production-before snapshot and prove it
matches the unchanged closeout. Qualify with focused controller tests and the
minimum small-model/disposable cases needed for those defects, obtain one
independent adversarial review, commit a terminal result, and end the task turn.
No primary artifact access, production mutation, performance work, cache
promotion, unrelated tuning, or L22 work is authorized.

## 2026-07-27 — accept L57 identity correction and isolate RPC response boundary

Decision: accept terminal L57 NOT PROMOTED at
`0026d5243c6108659fa53ce9185af9de0d6ec857` and retain its independently
reviewed parent/split graph-identity correction. Open L58 only as a
no-primary/no-production discriminator at the real RPC graph-compute
request/response boundary.

Reason: L57 closes the proven parent UID 26 versus RPC split UID 27
reconciliation defect with an authenticated two-level identity mapping and
passes focused tests, real RPC fixtures, exact ROCm builds, controller dry-run,
and pre-runtime review. Its sole stories15M run then reached a different
boundary: the worker logged an ordinary 144-node/193-tensor graph execution,
while the coordinator received no usable graph-compute response and reported
`Remote RPC server crashed or returned malformed response`. The retained
evidence does not show whether the server handler crashed or exited, omitted or
truncated its response, emitted the wrong framing/size/opcode, or whether the
client hit EOF/socket/decode validation. The later abort in RPC buffer cleanup
is secondary and cannot supply that missing classification.

L58 must instrument only the immediate real server handler and client
request/response seam with bounded authenticated metadata: exact protocol and
binary identities, connection/attempt/parent/split/sequence identity, opcode,
expected and actual byte counts, send/receive status and EOF/error, handler
entry/exit/status, and response publication. It must first pass focused
protocol/refusal tests and one small disposable RPC graph, then may consume one
stories15M first-armed-chunk run after independent pre-runtime review. It must
stop at classification: no cache capture/restore matrix, primary artifact,
production mutation, performance work, or combined semantic correction is
authorized. If a narrow fix becomes obvious, report it for a separate Lead
decision.

## 2026-07-27 — accept ambiguous L58 and repair failure evidence lifetime

Decision: accept terminal L58 NOT PROMOTED at
`e561b56ffb0edc4ffbc38b1c5426722146d32b37`. Open L59 only to make the
response-boundary evidence survive every child failure before cleanup, qualify
that contract with injected failures, and then run the deferred stories15M
first-chunk discriminator once.

Reason: L58's response-boundary instrumentation and strict verifier passed
focused qualification and independent pre-runtime review, and the runtime
again reached the target failure. The result is nevertheless ambiguous because
the runner harvested diagnostic streams only on success. The unit-exit
exception bypassed both copies and controller cleanup deleted the remote roots.
Neither client nor worker response record survived, so the evidence cannot
distinguish handler failure, missing response publication, truncation/framing,
socket EOF/error, or client validation refusal. This is a controller
evidence-lifetime defect; it is not evidence of a new cache or model defect.

L59 must place collection in a finally/registered cleanup path before worker
stop and path/key removal. It must independently validate, copy, fsync, hash,
and retain present/missing/error status for both streams, authenticating every
available closed prefix. Focused injected failures must prove partial-stream,
one-side-missing, tamper, copy-error, and cleanup-order behavior without model
runtime. Only after independent acceptance may L59 consume one first armed
stories chunk with the L58 instrumentation. It must stop at the diagnosis and
request separate authority for any semantic correction. No primary artifact,
production mutation, cache capture/restore matrix, or performance work is
authorized.

## 2026-07-27 — accept L60 unit guard and bind harvesters per host

Decision: accept terminal L60 NOT PROMOTED at
`ade0bc86a9f7659a67239865641d9a1211f8744f` and retain its reviewed
transient-unit guard. Open L61 only to bind and prove response harvester
execution and stream creation on each owning host, then run one first-chunk
discriminator.

Reason: L60 successfully closes stale fixed-unit reuse and its sole model run
again reaches the target graph failure. It still cannot classify the response
because nimo-1 was instructed to hash a harvester path staged only on nimo-2,
and the client stream was absent. This is host-path/runtime-admission evidence,
not a cache or inference conclusion.

L61 must freeze host-to-harvester path/hash/interpreter/input/output authority,
run a real two-host no-model fixture that creates and harvests both streams,
prove wrong-host and one-side-missing behavior, and verify a harmless client
prefix in the exact runtime environment before model launch. Only after
independent review may it consume one stories15M first armed chunk. Once model
runtime starts there is no retry, and any semantic correction remains a
separate decision. No cache matrix, primary artifact, production mutation, or
performance work is authorized.

## 2026-07-27 — resolve L66 admission-order circularity with two-phase L44

Decision: split L44 into a strictly non-mutating negotiated preflight/plan and
an authenticated activation. The immutable prepared admission binds the
scheduler-logical expected mutable census, not a server-physical census that
does not yet exist. The later physical server census is a distinct authenticated
observation and must reconcile exactly with the sealed logical expectation
before commit or execution.

Reason: exact source order proves the server allocation-topology epoch is first
learned during current L44 CAPS and the physical census is produced after the
current begin call. Requiring both inside an admission sealed before current
begin is circular. Preflight may authenticate capabilities, connection and
allocation epochs, server identity, and key generation, but may not consume the
admission, mutate tensor/model state, authorize SET/SET_HASH or compute, or
publish successful execution authority. After sealing, activation independently
verifies the complete admission and live identities before mutation. Atomic
single-use consumption remains immediately before authenticated execution.
Mismatch between logical expectation and physical observation aborts without
execution.

This is an in-scope L66 semantic correction, not a new milestone. Use explicit
logical-expected and physical-observed census types/names, focused lifecycle and
refusal checks, and the already-required real two-host fixture. No stories,
model, production, cache-matrix, or performance work is authorized.

## 2026-07-27 — accept L66 rejection and revise default-off retention for L67

Decision: accept terminal L66 NOT PROMOTED at
`755ba5f2ebd943e8a1204f31be4a80516dc06182`. Open L67 as the final bounded
ADR-0049 foundation correction. Separate safe default-off source retention from
milestone or product promotion.

Reason: L64 through L66 repeatedly produced exact-output, overlapping two-host
directional success and then deleted the complete candidate because recorder
and evidence gates remained incomplete. L66's independent review still proves
four real defects: self-derived/incomplete expected binding and split
consumption authority, range-based grammar accepting unterminated streams,
process-local shared-append publication, and no complete machine-validated
refusal/provenance manifest. Repeating the same broad retention rule is no
longer product-progressive.

L67 must close the first three defects with independent expectations, one
authenticated cross-side consume receipt immediately before execution, exact
enumerated terminal productions, and immutable unique per-attempt atomic
no-replace publication. It must generate a compact validated manifest for the
focused evidence and reuse accepted L65/L66 real-handler evidence rather than
repeat an exhaustive matrix. One final composed run is allowed only after local
gates pass and only if the final source materially changes that path.

Reviewed default-off source may remain when it builds, feature-off is inert,
focused correctness gates pass, and independent review finds no correctness or
security P1/P2. Documentation or manifest incompleteness alone blocks promotion
but no longer requires deleting safe reusable source. Any accepted invalid
state, unauthenticated consumption, unsafe publication, correctness/security
defect, or feature-off regression still requires correction or removal. No
stories, model, cache, performance, or production work is authorized.

## 2026-07-27 — accept retained L67 and open one Stories15M vertical slice

Decision: accept L67 PASS at
`38c7d4ad7802116daac83e3927a1e1ea42fec8c9` as the retained default-off
ADR-0049 foundation. Open L68 for one disposable Stories15M real-model
vertical-slice qualification through the composed L40/L42/L44 path.

Reason: L67 closes all four L65/L66 review blockers with independently built
complete expectations and a signed cross-side execute/consume receipt, an exact
13-production grammar, immutable atomic no-replace per-attempt publication, and
a compact validated evidence manifest. Focused build, compile-off, runtime-off,
structural refusals, separate-process collision, transport refusal, and the real
two-host no-model composed fixture passed. Independent final review reported no
remaining correctness, security, or manifest P1/P2. This establishes reusable
authority but does not prove the actual model path now completes.

L68 gets one matched feature-off control and one feature-on composed run using
the already-qualified disposable Stories15M topology and the smallest
deterministic request that exercises authenticated RPC execute and return.
Acceptance is exact output agreement, feature-off inertness, a complete
feature-on execute/consume/terminal record, exact source/binary/runtime
identity, bounded cleanup, and continuously unchanged production. Timing is
diagnostic only. Reuse accepted controller and harvester evidence; do not repeat
broad refusal matrices.

No cache capture/restore, primary model, tuning, performance claim, repeated
retry, or production mutation is authorized. A source-proven mechanical
integration correction may remain inside L68 after focused checks; a semantic
correctness/security defect or ambiguous runtime result must stop for a
separate decision.

## 2026-07-28 — accept L68 refusal and correct expected-census ordering once

Decision: accept L68 terminal NOT PROMOTED at
`91e9761b09ca39786b5d1394f308412bab45041d`. Open L69 only to install the
sealed admission's authenticated expected census counts before registration
begins, then run one deferred feature-on Stories15M replacement.

Reason: L68's feature-off control completed deterministically with token
`29916` (`x`) and empty authority. Feature-on reached real L42/L44 setup and
registered 11 roots plus 36 exclusions, but failed before authenticated
execute. Both sides then refused terminal publication because expected counts
were still `0/0`; source installs them only after prepare succeeds. This is a
proven ordering defect that erased the primary refusal, not a reason to reopen
the whole foundation.

L69 must source expected counts from the sealed admission before the first
registration/exclusion event, never infer them from observed counts, and keep
the exact grammar unchanged. One focused no-model abort after complete
registration but before prepare plus a wrong-expectation refusal and focused
source review qualify the correction. Reuse the accepted L68 feature-off
control. Exactly one identical feature-on replacement is authorized. If it
fails after retaining terminal evidence, stop at the newly classified cause.
No broad abort matrix, feature-off repeat, cache work, primary model, tuning,
performance claim, repeated retry, or production mutation is authorized.

## 2026-07-28 — L69 terminal NOT PROMOTED

Implementation reported terminal L69 NOT PROMOTED at
`09fa1f4313c81ca9e629af6772f2108fb7ab8bf7`.

The expected-census installation-order correction passed its focused compile,
exact pre-prepare abort, wrong-count refusal, preserved pre-registration abort,
and independent source review. The L68 feature-off control was reused without
rerun.

The single authorized feature-on replacement was consumed once and stopped
before authenticated execute. The sealed logical census expected 28 register
and 38 exclude events; the real preparation path emitted only 11 and 36 before
abort. Exact grammar correctly refused this partial-census terminal state
instead of inferring expectations from observation. Canary exit status was 42
and `llama_decode` returned `-3`. No retry was attempted. Production remained
byte-identical across preflight/final snapshots and all disposable resources
were removed.

## 2026-07-28 — classify L69 mismatch as an earlier registration refusal

Decision: accept L69 terminal NOT PROMOTED at
`09fa1f4313c81ca9e629af6772f2108fb7ab8bf7` and retain its reviewed
expected-census ordering correction. Do not weaken grammar, add a partial-census
terminal, retry Stories15M, or open another foundation layer. Open L70 only as
read-only/offline diagnosis of the first unadmitted ordered census entry.

Reason: the authenticated whole-graph plan expected 28 register and 38 exclude
events, but the real stream stopped at 11 and 36. Exact source order processes
scheduler roots first and scheduler copies afterward, so the completed 11/36
prefix is evidence that a later registration/exclusion call refused before
prepare. The partial terminal is a consequence; accepting it would hide the
earlier semantic mismatch.

L70 must reconstruct the ordered root-plus-copy plan for the identical graph,
map the observed prefix to the first failing entry, and identify its tensor,
view/buffer ownership, backend, role/class/ordinal, and exact refusal condition.
It must decide whether the sealed logical census wrongly includes an entry that
cannot exist on the RPC session or runtime registration resolves the wrong
tensor/connection. No code change, new instrumentation unless evidence is
literally insufficient, model run, production action, grammar change, retry, or
follow-on implementation is authorized. Project-management files remain
Lead-owned.

## 2026-07-28 — accept L70 diagnosis and unify the census authority list

Decision: accept L70 as a complete evidence-insufficiency diagnosis. Do not run
another model extraction merely to name the first tensor. Open L71 only to build
one immutable canonical per-RPC-backend census entry list that is used both to
seal the expected census and to drive runtime L44 registration/exclusion.

Reason: source proves the admission builder collects and sorts root/copy
metadata separately, while runtime independently re-walks roots in leaf order
and copies in scheduler insertion order. Scheduler copy planning can append an
entry even when a destination copy was already inserted, so repeated plan
entries may name the same runtime tensor. The sealed counts can therefore
describe a stream that `mutable_register` correctly refuses as a duplicate.
This source-level authority mismatch exists independently of the exact missing
L69 tensor identity.

The canonical list must bind backend, stable source-authoritative runtime tensor
identity, root/copy provenance, role/class/ordinal, and register/exclude
disposition. Collapse only semantically identical repeats for the same backend
and tensor; conflicting duplicates fail plan construction. Counts/root and
runtime iteration must consume this exact list. Do not weaken duplicate
refusals or use pointer-order hashing as stable authority.

L71 qualification is limited to a synthetic repeated-copy case, conflicting
duplicate refusal, sealed-list/count/root agreement, exact-once iteration,
feature-off inertness, required builds, and focused independent source review.
No Stories/model run, offline extraction, RPC transport qualification, grammar
change, cache or primary work, performance work, production action, or
automatic follow-on is authorized.

## 2026-07-28 — accept L94 and open L95 authoritative launch/cgroup parsing

Decision: accept L94 terminal NOT PROMOTED at
`4e4245f447928bd7ae8d63a1d0ef330ec2c0cc64` and retain its reviewed source.
Open L95 to replace the restore-canary InvocationID text parser and raw cgroup
comparison with existing/closed authority mechanisms, followed by one full
primary correctness attempt.

Reason: the restore canary's `systemd-run` returned success and emitted its
InvocationID on stderr, while this one path parsed stdout. Other launches
already obtain PID/InvocationID from exact systemd state through
`capture_disposable_unit_authority`. Cleanup's alternate-owner check similarly
compared raw `ps` cgroup text to systemd's normalized absolute ControlGroup
path. These are duplicated presentation parsing defects, not cache/runtime
failures.

L95 must reuse `capture_disposable_unit_authority` for restore-canary launch
and bind its cursor/PID/InvocationID to subsequent systemd state. Raw
stdout/stderr remains evidence but not authority. For cgroups, read and
strictly parse the source-proven unified cgroup-v2 identity (prefer
`/proc/<pid>/cgroup`), require one valid `0::/absolute/path` entry, and compare
the normalized path exactly to systemd ControlGroup plus manifest unit, PID,
InvocationID, host, and port. Malformed, v1, multiple, relative, stale,
same-unit, production, unknown, or ambiguous ownership refuses. Audit the same
two parser families for other reachable duplicated seams. Focused tests and
one independent review are sufficient.

After those gates, exactly one OOM-aware primary transition may run through
capture, fresh restore, and one token under existing token/state/component
equality, zero legacy GET/SET, custody, cleanup, recovery, and production
reconciliation gates. No retry, runtime semantic correction, protocol/cache
product/performance work, or automatic follow-on is authorized.

## 2026-07-28 — accept L95 and open L96 relocatable runtime package gate

Decision: accept L95 terminal NOT PROMOTED at
`7e53bd82bdf96e18ec77380a011dc44be444f5f3` and retain its reviewed parser
corrections. Open L96 to make the cross-host staged runtime relocatable and
prove it on nimo-2 before any production shutdown, followed conditionally by
one full primary correctness attempt.

Reason: L95 never executed the model. The staged canary failed provenance with
rc127 because its ELF RUNPATH embedded the absolute nimo-1 build directory.
The archive contained `libllama-common.so.0` and valid relative symlinks, but
the dynamic loader could not resolve them on nimo-2. This must be prevented at
package admission rather than discovered after production transition.

L96 should use the smallest build/packaging correction, preferably
origin-relative CMake build RUNPATH for project libraries plus only explicitly
approved fixed ROCm paths. Ambient `LD_LIBRARY_PATH` and ad-hoc runtime copying
are not authority. Before production mutation, exact extracted nimo-2 staging
must reject absolute source/build RPATH/RUNPATH, path or symlink escape,
missing/tampered libraries, and unresolved dependencies; bind ELF dependencies,
paths, hashes, modes, owners, symlinks, source/build/archive identities, and
run the exact staged provenance plus no-model probe under a sanitized
environment. Apply the same closed gate to every cross-host executable in this
transition. Focused negatives, feature-off inertness, and one independent
review are sufficient.

Only a passing retained pre-mutation package receipt authorizes exactly one
OOM-aware primary capture/fresh-restore/one-token transition under existing
equality/state/custody/zero-GET-SET/cleanup/recovery gates. No retry, runtime
semantic correction, protocol/cache product/performance work, or automatic
follow-on is authorized.

## 2026-07-28 — accept L96 and open L97 exact package probe contract

Decision: accept L96 terminal NOT PROMOTED at
`8c25e2a160655c4bdebb3fc742301e615b49ad1c` and retain its reviewed
relocatable runtime/package gate. Open L97 to correct the unsupported `--help`
exit assumption, followed conditionally by one full primary correctness
attempt.

Reason: exact canary source defines no `--help` success path; unsupported or
insufficient CLI input returns 2 from option/common parsing. The staged nimo-2
canary already executed exact canonical `--halofpx-provenance` with rc0 under a
sanitized environment after its origin-relative RUNPATH, DT_NEEDED, ldd,
symlink, hash, owner, mode, source, build, and archive authority passed. That is
the proper loader/startup proof. Requiring generic help rc0 was unsupported.

L97 must use canonical provenance execution cross-bound to the closed package
receipt as the positive no-model execution gate. Remove the help requirement,
or retain it only as an exact source-defined rc2 negative with pinned output
hashes. Arbitrary nonzero acceptance is forbidden. Audit other generic CLI
exit assumptions and focus tests on provenance success, loader rc127,
tamper/unresolved dependency, wrong provenance, and wrong exit refusal. One
independent review is sufficient.

Only a passing nimo-2 pre-mutation gate authorizes exactly one OOM-aware
primary capture/fresh-restore/one-token transition under existing equality,
state/custody, zero-GET/SET, cleanup, recovery, and production reconciliation
gates. No retry, runtime semantic correction, protocol/cache product/
performance work, or automatic follow-on is authorized.

## 2026-07-28 — classify L97 terminal and accept attributed OOM baseline

Decision: classify the consumed L97 transition terminal NOT PROMOTED with no
retry. Accept the current `NRestarts=1` production pair as the new observed
baseline. Authorize terminal evidence/review/commit and subsequent read-only/
source-only diagnosis only.

Reason: the L97 package gate passed and the primary path advanced through
residency-A capture and fresh residency-B worker/canary execution before
refusing `durable and emitted result authority differ`. This is the next
forward correctness boundary. A restore-canary InvocationID change before
evidence collection is separately material but is not assumed causal.

During recovery, journals exactly attribute the worker counter change to one
kernel OOM kill at 21:08:42 and one policy restart at 21:08:45. The coordinator
then aborted after RPC loss at 21:08:47 and policy-restarted once at 21:08:52.
Both services are now stable, unique, and healthy with unchanged installed
unit/argv/config/executable identities and coordinator HTTP 200. Accepted
authority is coordinator PID `2989515`, InvocationID
`49d23af81c5d495b80e3c9c906f72c7a`, `NRestarts=1`; worker PID `2135516`,
InvocationID `7a3c97b846854036acd33421bb45ab73`, `NRestarts=1`.

Do not restart, reset, reconfigure, or otherwise mutate production. Disposable
cleanup is complete. Preserve exact durable/emitted artifacts and chronology,
finish independent terminal review and commit, then diagnose the result-
authority mismatch and InvocationID lifecycle read-only. No correction,
host/model action, or automatic follow-on is authorized.

## 2026-07-28 — accept L97 and open L98 result/lifecycle closure

Decision: accept L97 terminal NOT PROMOTED at the corrected exact commit
`647f3d4bfd4574e6b5086c42407116cbb5ce843b`. Retain its reviewed default-off
source and accepted production baseline. Open L98 to close the empty result
field and restore-canary terminal InvocationID P2s together, followed by one
full primary correctness attempt.

Reason: durable JSON and emitted output differ only because `output_fields()`
uses `([^ ]+)`, which cannot represent the valid empty emitted field
`prompt_chunk_sizes=`. Residency-B otherwise executed through authenticated
server terminal authority. Separately, the transient restore unit unloads and
clears InvocationID before evidence collection; no evidence makes that the
forward cause, but its custody contract must be closed in the same milestone.

L98 must replace regex extraction with a total canonical space-delimited
`key=value` parser. Empty values are valid; empty tokens/multiple separators,
invalid or duplicate keys, missing delimiters, whitespace/control characters
inside values, and unconsumed text refuse. The parsed map must equal the
independently verified durable JSON exactly. Tests must replay the retained L97
line/JSON and cover empty/nonempty plus structural negatives.

Restore-canary launch must retain systemd terminal authority, preferably with
`RemainAfterExit=yes` consistent with ordinary canaries. Require exact terminal
Active/SubState, ExecMainCode/Status/Result, retained launch cursor/PID/
InvocationID, journal collection by InvocationID before explicit stop/reset/
unload, and final absence. Cleared or changed identity before collection
refuses. Audit other reachable canary collection paths for the same lifecycle.
Focused tests and one independent review are sufficient.

After those gates, exactly one OOM-aware primary capture/fresh-restore/
one-token transition may run under existing equality/state/component/custody/
zero-GET-SET/cleanup/recovery gates. No retry, runtime semantic correction,
protocol/cache-product/performance work, or automatic follow-on is authorized.

## 2026-07-28 — accept L98 correctness failure and open L99 product diagnosis

Decision: accept L98 terminal NOT PROMOTED at
`db3b034dfe6d6ef857031ff5473f8fe2286657d1`, retain its reviewed corrections,
and classify the token/logits mismatch as a P1 product blocker. Open L99 as
read-only/source-only diagnosis. No primary retry or semantic correction is
authorized.

Reason: the full path finally reached a genuine fresh restore result. Capture
returned token 21549/suffix `alpha`; restore returned token 9283/suffix `计划`.
Authenticated semantic provenance confirms different logits, while represented
coordinator control/local state and all worker component manifests agree.
Matching represented hashes therefore demonstrate the current state contract
is incomplete or execution consumes additional uncontrolled state; they do not
establish cache correctness.

L99 must build a machine-readable canonical diff of all retained capture/
restore results, semantic and replay authority, composed authority, component
capture/apply records, epoch/plan/topology, and server authority to identify the
earliest retained divergence. It must then trace exact final-token read-sets
through coordinator ROCm and RPC worker code, including KV logical bytes versus
padding/strides/views, cell/head/sequence metadata, scheduler copies and graph
allocation, quantized attention, synchronization/events, RPC reconstruction,
allocator/runtime state, and architecture-specific recurrent/expert/router
state. Reconcile accepted L31-L35 and L37-L44 evidence; eliminated hypotheses
stay eliminated absent new exact evidence. Rank source-supported candidates
and propose the smallest offline/no-model discriminator or minimum future
instrumentation.

Separately, L99 may fix the response verifier offline only by grouping each
authenticated stream into exact per-attempt 1..N productions with closed
identity bindings and replay/gap/mixing refusal. Replay retained L98 evidence
and claim zero legacy GET/SET only if exact bounded windows support it. This P2
cannot explain the P1. One independent read-only review is required. No host,
model, build, runtime, production mutation, or automatic follow-on is
authorized.

## 2026-07-28 — accept L90 and open L91 consolidated child authority

Decision: accept L90 terminal NOT PROMOTED at
`6e8d2b2b834af3a0e739efd11dbef271a362ddc6` and retain its independently
reviewed pre-mutation disposable reconciliation. Open L91 to consolidate the
child transient-unit guard onto validated manifest authority, followed by one
full primary correctness attempt.

Reason: L90 proved the pre-mutation closed set absent and completed
authenticated primary warmup with exact L89 paired response custody. It then
stopped before workload/capture/restore because the child uses a separate
hard-coded transient-unit allowlist rather than the controller's validated
manifest-owned set. This is duplicated controller authority, not a runtime,
cache, response-protocol, or model defect.

L91 must derive child launch and cleanup guard tuples from the validated
manifest, canonicalizing the service name exactly once. Wrong host/port,
near-name, missing entry, active ownership, and ambiguity remain refusals.
Every reachable L77 primary `systemd-run` branch and cleanup call must be
checked against the same canonical set, with a focused static/call-path scan,
no-host tests, and one independent review. Unambiguous mechanical pre-runtime
corrections found by that consolidated scan may remain within L91 after
focused qualification rather than opening another milestone.

After those gates, exactly one primary transition may run in the full
correctness mode, not first-chunk-only diagnostic mode. Warmup success must
continue to residency-A reference/capture, worker restart, fresh-residency-B
authenticated stage/commit, and one deterministic restored token. Acceptance
requires exact reference/restored token and state/component equality, zero
legacy state-page GET/SET, complete L44/L76/L89 client/server authority and
custody, bounded cleanup, worker-first/coordinator-second recovery, and exact
final production reconciliation. No retry, runtime semantic correction,
server-abort change, production cache enablement, server composition,
performance work, tuning, or automatic follow-on is authorized.

## 2026-07-28 — accept L91 attributed OOM recovery baseline

Decision: classify the consumed L91 transition as terminal NOT PROMOTED with no
retry. Accept the current production PIDs, InvocationIDs, and `NRestarts=1` as
the new observed baseline. Authorize only exact predicate-checked disposable
cleanup and read-only terminal diagnosis/evidence closeout.

Reason: journals attribute the worker counter change to one kernel OOM kill of
the recovery-started process and the coordinator counter change to its abort
after losing that RPC worker. Existing on-failure policies restarted each unit
exactly once. Both are now healthy and unique with unchanged installed unit,
argv/config, listener authority, and coordinator HTTP 200. This is an
explained recovery cascade, not permission to normalize or reset counters.

Accepted authority is coordinator PID `2896932`, InvocationID
`d33e57248a4e4eb98f81cc1a44cf1ff6`, `NRestarts=1`, port 8081/HTTP 200; worker
PID `2084398`, InvocationID `0137204322234e5e9ddde8a4173ef177`,
`NRestarts=1`, port 50052. Production services must not be restarted or
mutated.

Cleanup may target only exact closed-manifest L48/L89/L90/L91 disposable
units, keys, source/build/archive, and evidence-staging paths after exact
type/owner/mode/identity, non-mount, no-live-reference, and non-production
predicates pass. Only inactive/exited MainPID-0 expected disposables may unload.
Re-prove absence and the accepted production authority afterward. Preserve the
guard failure and complete independent terminal review. Diagnose why the
consolidated child authority still refused using retained source/evidence only;
no correction, host/model action, or new runtime is authorized.

## 2026-07-28 — accept L91 capture and open L92 local authority discriminator

Decision: accept L91 terminal NOT PROMOTED at
`2b0910b3b0ac80b01901838e2f26773194aa3248` and retain its reviewed
default-off source. Open L92 for a no-host closed-path unit-authority
discriminator and conditional smallest source-proven correction. A primary
transition is conditional on that discriminator passing.

Reason: L91 materially advanced through authenticated residency-A capture and
retained four server authorities, then refused before residency B. Retained
evidence localizes the refusal to tuple membership after capture-worker
cleanup, but omitted the rejected canonical tuple and installed authority-set
identity. Current source nominally admits the expected restore tuple; choosing
a field correction would therefore be guesswork and another blind full-model
transition is disproportionate.

L92 must durably retain the exact canonical installed authority entries and
hash, all planned reachable L77 primary launch/cleanup tuples in execution
order, and any refusal's requested tuple, phase, set hash, and membership
result. Its deterministic no-host rehearsal must consume the real child
initialization/environment produced by the exact validated manifest and the
real tuple constructors for device gate, capture worker/canary, capture
cleanup, restore worker/canary, and cleanup. Hand-built expected tuples are not
acceptable. Exact coverage and a stable authority hash across the simulated
capture-to-restore transition are required.

If the rehearsal reproduces a mismatch, the smallest source-proven correction
may remain within L92 after focused tests and independent review. If it does
not reproduce, stop without host/model access and report the evidence blocker.
Only a passing real closed-path rehearsal and no-P1/P2 review authorize exactly
one primary transition through residency-A capture, fresh residency-B restore,
and one deterministic token under existing equality, custody, zero-GET/SET,
cleanup, recovery, and production-reconciliation gates. No retry, broad matrix,
protocol change, performance work, production cache enablement, or automatic
follow-on is authorized.

## 2026-07-28 — accept L93 and open L94 combined restore-launch closure

Decision: accept L93 terminal NOT PROMOTED at
`62199ea96a171e0dd1f691cddf68af871b0f1e54` and retain its reviewed explicit-
port correction. Open L94 to close the restore-canary journal lower-bound and
shared-listener cleanup-attribution P2s together, followed by one full primary
correctness attempt.

Reason: L93 completed authenticated residency-A capture and launched/admitted a
genuinely fresh restore worker. Restore-canary launch stopped because this one
path manually parsed raw `journalctl --show-cursor` output instead of using the
already accepted exact cursor helper. During final cleanup, the stopped capture
worker's absence check also treated the active restore worker's shared 50248
listener as capture ownership. Both are bounded controller-observability
defects, not runtime/cache failures.

L94 must use `exact_journal_cursor()` for the restore-canary lower bound and
retain it before launch, with focused format/refusal coverage. An absent
capture unit may coexist with the shared listener only when the listener PID is
positively bound to a different currently admitted manifest unit on the same
host/port through retained PID/InvocationID plus exact current unit/cgroup/
MainPID authority. Unknown, production, same-unit, stale, multiple, or
mismatched ownership refuses. Generic nonzero-listener acceptance is forbidden.
Audit capture-to-restore and finally-cleanup ordering; one independent review
is sufficient.

After those gates, exactly one OOM-aware primary transition may run through
capture, fresh restore, and one token under the existing exact token/state/
component equality, zero legacy GET/SET, authority/custody, cleanup, recovery,
and production-reconciliation gates. No retry, runtime semantic correction,
protocol change, cache product enablement, performance work, tuning, or
automatic follow-on is authorized.

## 2026-07-28 — classify L92 terminal and require read-only production reconciliation

Decision: classify L92 terminal NOT PROMOTED with no retry or correction in
L92. Accept its exact source diagnosis. Authorize read-only production
reconciliation and, only after exact attribution, predicate-checked disposable
cleanup. Production services and counters must not be mutated.

Reason: the durable sequence-5 receipt proves post-capture cleanup requested
`(nimo-1, halofpx-l48-worker-capture, 50184)` while the validated authority and
actual launch used port 50248. Python evaluated the default argument in
`stop_worker(unit, port: int = PORT)` when module `PORT` was still 50184,
before L77 configuration changed the global to 50248. The local rehearsal used
the launch-derived tuple and therefore missed this language-level binding
seam. Residency A captured successfully; residency B never launched.

The freshly controller-started production services currently report
`NRestarts=0`, unlike the prior accepted baseline of 1. Accept the new PIDs,
InvocationIDs, and counters only if exact system-unit/argv/config/hash/cgroup/
listener/HTTP/journal evidence proves clean controller stop/start, no
intervening fault or policy restart, and stable health. Otherwise retain unsafe
classification and stop. Do not normalize or reset counters.

After acceptance, cleanup may touch only exact closed-manifest disposable
targets after type/owner/mode/non-mount/no-live-reference/non-production
predicates pass. Re-prove absence and production authority. Complete terminal
review and commit. A read-only/source-only audit for other Python default
arguments capturing mutable configuration globals is authorized; implementation
belongs to a separate Lead decision.

## 2026-07-28 — accept L92 and open L93 explicit-port correctness attempt

Decision: accept L92 terminal NOT PROMOTED at
`6735cdddedc9254328bd2ef4b44c3b0f9071b60c`, its exact diagnosis, cleanup,
independent review, and reconciled production baseline. Open L93 for the
mechanical explicit-port correction followed by one full primary correctness
attempt.

Reason: durable evidence proves Python's `stop_worker` default captured port
50184 before L77 configured 50248. No other mutable configuration default was
found. Residency-A capture and four authenticated server authorities already
passed; the correction no longer needs another diagnostic milestone.

L93 must remove implicit mutable-global port authority: `stop_worker` requires
an explicit port and every reachable call site must provide the exact
launch/manifest-derived value, including device-gate 50249 and L77 50248.
Missing/wrong ports refuse. The real no-host rehearsal must invoke the actual
cleanup path, retaining request/set receipts and stable authority hash.
Focused coverage of startup/configured ports, capture/restore cleanup, all
reachable L77 branches, and feature-off behavior plus one independent review
is sufficient.

After those gates, exactly one OOM-aware primary transition may run through
authenticated residency-A capture, worker restart/fresh residency-B
stage/commit, and one restored token. Acceptance requires exact reference/
restored token and authenticated state/component equality, zero legacy
state-page GET/SET, complete client/server custody, no accepted invalid state,
bounded cleanup, ordered recovery, and exact production reconciliation. No
retry, runtime semantic correction, protocol change, production cache
enablement, server composition, performance work, tuning, or automatic
follow-on is authorized.

## 2026-07-28 — accept L71 and consume one canonical-census replacement run

Decision: accept L71 PASS at
`3f22338f0582640496c4f7033ea1c67132f6ba8d` and retain its canonical census
correction. Open L72 for exactly one feature-on Stories15M replacement using
the identical L69 model/request/topology/runtime tuple. Reuse the accepted L68
feature-off token `29916` / output `x` without rerun.

Reason: L71 makes one immutable per-RPC-backend list the sole authority for
sealed counts/root and runtime registration/exclusion. It collapses only exact
semantic duplicates and fails conflicting stable/runtime tensor identities.
Focused repeated-copy, conflict, projection, exact-once iteration,
compile-off, and independent source review gates passed with no
correctness/security P1/P2. The pre-existing Windows feature-off final-link
symbol issue is outside the L71 diff and does not block the Linux feature-on
disposable path.

L72 must build only required Linux feature-on binaries, bind exact identities,
and consume one request. Success requires canonical-list/count/root agreement,
authenticated prepare and execute/consume, RPC return, terminal authority, and
exact output matching the retained control. Failure must retain the precise
terminal semantic boundary and stop. No feature-off repeat, foundation
expansion, broad tests, cache or primary work, tuning, performance claim,
production mutation, retry, or automatic follow-on is authorized.

## 2026-07-28 — accept L72 execution and align canonical order with grammar

Decision: accept L72 terminal NOT PROMOTED at
`11f4b50fc48cdb52570d55d4d054c5470d95f13e`. Open L73 as one combined
mechanical canonical-order correction plus one feature-on Stories15M
replacement; do not split another source-only milestone.

Reason: L72 proves the canonical census cardinalities match exactly and the
server authenticated, prepared, and executed sequence 1 / UID 27. The client
recorded prepare, decision, complete transport, commit, and end, then refused
only because the L71 comparator can interleave register/exclude dispositions
while exact grammar v1 requires a contiguous register block followed by a
contiguous exclude block. This is a deterministic comparator defect, not a
reason to change grammar or reopen authority design.

L73 makes disposition the primary canonical key within each backend, REGISTER
before EXCLUDE, followed by the existing pointer-independent keys. The same
immutable list remains sole authority for sealing and runtime iteration.
Focused interleaving, duplicate/conflict, projection, exact-once iteration,
feature-off, required build, and source-review gates precede one identical
feature-on replacement. Reuse the L68 control without rerun. Success requires
token `29916` / output `x` and accepted client/server terminal authority.
Failure retains its next exact boundary and stops. No grammar change,
foundation redesign, broad matrix, cache/primary/performance work, production
mutation, retry, or automatic follow-on is authorized.

## 2026-07-28 — accept L73 model success and correct server recorder ownership

Decision: accept L73 terminal NOT PROMOTED at
`8ee9ba84ded5f96094762108da63fcac9524ae4f` and retain its exact model-success
evidence and canonical-order correction. Do not rerun L73. Open L74 as one
narrow server-recorder ownership correction plus one final feature-on
qualification.

Reason: L73 completed the real feature-on request with exact retained-control
token `29916` / output `x`, accepted client authority and mutable receipt, and
matching authenticated server prepare, consume, execute, and return. The only
promotion blocker is contradictory server-local evidence: that recorder uses
the full client grammar but never owns or records client L42, census, or
transport events, so teardown necessarily refuses.

L74 must keep the client grammar unchanged and define an explicit exact
server-owned grammar over real server seams only: authenticated admission,
physical prepare/reconcile, atomic execute-intent consumption, backend result,
authenticated response/receipt publication, and terminal close/refusal. Every
server record binds the same admission object, expected digest, sequence, split
and backend, graph digest, epochs, and signed consume/execute receipt. Do not
fabricate/import client facts, suppress server publication, or treat conflicts
as advisory.

Focused server success/refusal and structural-negative fixtures, cross-binding,
feature-off, required builds, and independent source review precede one
identical feature-on qualification against the retained L68 control. Success
requires accepted client and server terminals plus exact output. Failure stops
at its precise boundary. No broader redesign, client grammar change, broad
matrix, cache/primary/performance work, production mutation, retry, or
automatic follow-on is authorized.

## 2026-07-28 — accept L74 exact output and close server evidence custody

Decision: accept L74 terminal NOT PROMOTED at
`2c6c39c5b9e01baa945bc81151b0a51211817ed3` and retain its reviewed
server-owned grammar and exact model-success evidence. Do not rerun L74 or
Stories15M. Open L75 only to make server terminal publication outcome explicit
and harvest the immutable server authority before cleanup.

Reason: L74 again returned exact token `29916` / output `x` with accepted client
terminal, authenticated mutable/graph receipts, and matching server
prepare/consume/execute/return. Source review found no correctness/security
P1/P2. Promotion is blocked solely because the exact model attempt's
`*-server.authority` file was not retained. The response reaches the client
before later server terminal publication; absence of a refusal cannot prove
durable publication.

L75 must journal server publication success/failure with nonsecret exact
identity/path/hash, then make the controller quiesce and authenticate/copy/fsync
the expected server authority in its evidence-finally path before any cleanup.
Missing, invalid, tampered, or failed copies are durably recorded and
non-promotable while cleanup/recovery continues. Focused no-model success,
publication-failure, copy/tamper, and cleanup-order cases plus required builds
and one independent review are sufficient. Reuse existing harvest machinery.

No model/Stories run, response acknowledgment redesign, protocol/grammar
redesign, cache/primary/performance work, production access/mutation, or
automatic follow-on is authorized.

## 2026-07-28 — retain L75 and run the missing real Linux custody proof

Decision: accept L75 terminal NOT PROMOTED at
`2902b4b6e3266c591dbaabbd7de3802866275bc4` and retain its independently
reviewed default-off source. Open L76 only for the missing real Linux no-model
server-handler publication/harvest qualification.

Reason: L75 explicitly journals post-response server publication
present/error state and makes controller harvest owner/mode/type/size/HMAC,
server-grammar, cross-binding, atomic no-replace, fsync/reopen/hash, and
cleanup-order authoritative. Focused suites and a Linux authenticated helper
harvest passed with no review P1/P2. The local host cannot build feature-on
Linux code, and L75 prohibited target-host access, so the actual handler seam
remains unqualified.

L76 may use one Linux target host with isolated disposable user units, paths,
keys, and ports while named production services remain continuously untouched.
Run exactly one real-handler publication success and one injected publication
failure. Success must quiesce, authenticate, and retain the exact server
authority before cleanup. Failure must produce explicit bound error evidence,
non-promotable harvest status, and complete cleanup. Reuse accepted L75/L61
machinery; do not repeat tamper/copy cases unless review requires one.

No model/Stories run, cache or primary work, protocol/grammar change,
performance work, production mutation, broad tests, extra retry, or automatic
follow-on is authorized.

## 2026-07-28 — correct the L76 helper key mode within the same milestone

Decision: keep L76 open and authorize the smallest mechanical correction:
change only `halofpx_server_authority_harvest.py` channel-key mode validation
from `0400` to the production-transition controller's established exact `0600`
contract. Keep the server-authority source-file requirement at `0400`, retain
all other custody and grammar semantics, rebuild the corrected feature-on
Linux source, and resume exactly the previously authorized success and injected
publication-failure cases. The stopped preflight consumed neither case.

Reason: exact source proves the controller provisions and requires the channel
key as an owner-only regular file at mode `0600`; the helper's isolated `0400`
check makes the real controller harvest path fail deterministically. Changing
the disposable key to `0400` would instead falsify controller qualification.
This is an unambiguous internal contract mismatch, not a protocol decision or a
reason to discard independently reviewed L75 source.

Focused `0600` acceptance and wrong-mode refusal are sufficient before the
rebuild. No additional runtime cases, model/Stories work, cache/primary work,
protocol or grammar changes, performance work, production mutation, or
automatic follow-on are authorized.

## 2026-07-28 — correct L76 remote POSIX path handling and replace success once

Decision: keep L76 open. Correct only the controller's interpretation of remote
authority paths by using `posixpath` or `PurePosixPath` for remote basename
validation, parent/basename staging construction, and basename extraction
before local retention. Keep local retained-path operations on host
`pathlib.Path`. After focused Windows-host path checks and one corrected Linux
rebuild, authorize exactly one replacement success request and the original
still-unconsumed injected publication-failure case. No further retry is
authorized.

Reason: the first success request is consumed and valid evidence of
authenticated handler execution and authority publication, but it cannot
qualify custody. Windows `Path` emitted a backslash-delimited staging path for a
Linux target, and the helper correctly refused `path_authority`; cleanup then
removed the unharvested remote source. Exact source also uses host `Path` for
the remote basename check and local-name derivation, so all remote parsing must
be corrected together without changing the local retention implementation.

Preserve the first success/publication and failed-custody evidence. Do not
weaken the exact remote root, traversal, identity, collision, or atomic
no-replace checks. No protocol/grammar/model/cache/primary/performance change,
production mutation, extra runtime case, or automatic follow-on is authorized.

## 2026-07-28 — align L76 helper HMAC key decoding and allow final replacement

Decision: keep L76 open. Preserve full-file authority and SHA-256 binding, but
make the harvest helper require the exact 130-byte, two-lowercase-hex-line key
format and decode only the first line to the 32-byte graph/preexecute HMAC key,
matching `hfx_graph_key`. After focused format/refusal checks, corrected
synthetic evidence, one independent review, and one rebuild, authorize exactly
one final replacement success request followed by the still-unconsumed
publication-failure case. No further success retry is authorized in L76.

Reason: exact C++ source validates owner/mode/type/size and the SHA-256 of the
complete two-line file, then copies and decodes only bytes 0 through 63 for
graph HMAC. The helper validates the same file identity but incorrectly passes
all 130 bytes to HMAC. L75's synthetic fixture repeated that helper mistake, so
it could not expose the real-server mismatch. Both L76 server requests remain
valid execution/publication evidence but neither proves authenticated custody.

The helper must reject uppercase, malformed length/newlines/hex, extra bytes,
zero first-line key, full-file HMAC, and second-line HMAC while retaining the
full-file digest requirement. Do not expose key material. No protocol, grammar,
model, cache, primary, performance, or production change; extra runtime case;
or automatic follow-on is authorized.

## 2026-07-28 — accept and promote L76 real server-authority custody

Decision: accept L76 PASS/PROMOTE at terminal evidence commit
`6c8dc28a66b0fc3fb2525713dd505cc4320a4c27` and retain exact corrected source
commit `52cda98fd3e6f871096089db623ddcc2c2f10705`. Close the L75/L76
server-authority custody qualification lane. Do not automatically open another
foundation, model, cache, production, or performance milestone.

Reason: the final real Linux handler success authenticated and retained the
immutable six-record server authority after server quiescence and before remote
cleanup. The retained 4,200-byte artifact independently hashes to
`04ea9584d338d3772fa7a031daa20b12818ad7c93c074d1197d1a942e2cd9c8f` and
cross-binds the expected admission, sequence, split/backend, and execute
receipt. The independent failure case recorded bound publication `errno=5`,
created no authority file, was classified non-promotable, and still completed
cleanup.

Focused qualification passed 58 tests plus 11 C++ subtests. Independent final
review returned PASS/PROMOTE with no P1/P2. All disposable units and paths are
absent. Production preflight and terminal evidence independently hash to the
same SHA-256
`511f05d2b638277e19ae7af44573eb9e244d829458f24cdefc46e3f88ffd6ded`;
no production or model mutation occurred.

## 2026-07-28 — open L77 exact-primary distributed-state correctness gate

Decision: open L77 at accepted base
`6c8dc28a66b0fc3fb2525713dd505cc4320a4c27` for exactly one
controller-managed primary-model correctness discriminator. Reuse the accepted
distributed-state canary, controller, lifecycle authority, and evidence
custody. Compare one deterministic reference token from residency A with one
restored token after worker-first creation of a genuinely fresh residency B.

Reason: current source retains two separate capabilities: private
generation-one exact-key llama-server canaries and worker-local RPC
capture/stage/commit used only by the distributed-state test path. They are not
composed, so no admissible end-user two-node persistent cache exists. Earlier
primary attempts failed exact first-token equality, while L40-L76 subsequently
closed the missing scheduler/RPC execution and terminal evidence authority.
One current-head primary discriminator is the shortest honest gate before
building the server composition.

Use the pinned 159,873,097,824-byte artifact with SHA-256
`96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`,
the frozen deterministic request, balanced `RPC0,ROCm0`, and tensor split
`1,1`. Acceptance requires exact token equality, represented state equality,
zero legacy GET/SET state-page transfer, and accepted client/server terminal
custody. Reuse accepted allocation, controller, transport, graph, census, and
custody evidence rather than repeating their matrices.

Only one runtime attempt is authorized. Production transition must stop the
coordinator before the worker, recover worker before coordinator, and finish
with exact system-unit/listener/restart and HTTP 200 reconciliation. No
production cache enablement, server composition, performance claim, tuning,
extra attempt, semantic correction, or automatic follow-on is authorized.

## 2026-07-28 — accept L77 refusal and open one mechanical L78 replacement

Decision: accept L77 terminal NOT PROMOTED at
`72ac37f80e5b721bfe5824f702d50ed4d7cce7a4`. Open L78 only to admit the exact
`halofpx.l77.primary-manifest.v1` schema through the existing L52
controller-owned evidence-directory preparation and then run one replacement
of the primary correctness discriminator.

Reason: L77 passed exact preflight and independent pre-review but refused
before SSH, model load, RPC handling, capture, or restore. Exact source shows
that the controller validates L48 and L77 schemas elsewhere, while
`prepare_l52_evidence_directories()` alone returns unless the schema is L48.
The child correctly refused the missing directory. The missing server journal
is a secondary consequence. L77 supports no cache or model conclusion.

The correction must accept exactly L48 and L77, reject unknown and near-match
schemas, and preserve all directory ownership, permission, collision, symlink,
nonempty, and controller/child binding rules. After focused checks and one
independent exact-diff review, L78 may consume one replacement transition with
the same L77 workload and acceptance gates. Reuse unchanged preflight/build and
accepted foundation evidence; do not repeat broad matrices.

No protocol, grammar, cache, model, or production semantic change; production
cache enablement; server composition; performance work; tuning; extra attempt;
or automatic follow-on is authorized.

## 2026-07-28 — accept L78 refusal and consolidate L79 controller integration

Decision: accept L78 terminal NOT PROMOTED at
`ee6a7039996e8cb265268f2dd6c657544669098f`. Open L79 to define and apply one
exact composed-schema family for only L48 and L77 across existing controller
selection paths, then qualify the complete L77 controller-to-child preflight
without production access before authorizing one replacement transition.

Reason: the L78 evidence-directory correction passed, but
`child_environment()` retained an independent L48-only Boolean governing
provenance, component, semantic, replay, result, composition, graph/mutable,
response, host-bound harvest, and device-receipt environment. The child
correctly refused before host or model work. Exact source shows several paired
L48/L77 checks alongside this isolated L48-only family selection. Correcting
one branch and immediately restarting production would risk another mechanical
omission.

L79 must exhaustively scan controller schema comparisons and exercise the exact
L77 manifest through validation, executable/hash binding, evidence-directory
preparation, full environment construction, and child preflight up to but not
including SSH or host mutation. It must assert all required environment values
and reject unknown/near-match schemas. One focused independent review is
sufficient. Milestone-specific behavior and exact refusals remain unchanged.

After that offline closure only, L79 may consume one replacement of the same
primary discriminator under the existing production transition and correctness
gates. No arbitrary schema admission, controller redesign, protocol/grammar/
cache/model semantic change, production cache enablement, server composition,
performance work, tuning, extra attempt, or automatic follow-on is authorized.

## 2026-07-28 — accept L79 boundary and diagnose before any further runtime

Decision: accept L79 terminal NOT PROMOTED at
`64ef3034f232be980a2ee388e39c1bc43691e5cb`. Accept its consolidated
controller-integration closure, but admit no model or cache conclusion. Open
L80 as read-only/offline diagnosis of the warmup decode failure and the
independent server-authority helper identity mismatch. No runtime replacement
is authorized.

Reason: L79 reached the pinned primary model and one authenticated composed
server prepare/execute/publication sequence, then the client returned generic
`GGML_STATUS_FAILED` before any workload token or state capture. Retained
evidence does not presently distinguish local backend failure, composed
reconciliation/finalization, or another decode sub-boundary. Separately, the
server publication journal cannot substitute for immutable authority because
the staged harvester failed exact source identity.

L80 must exhaust retained client, worker, controller, recorder, and response
evidence against every reachable failure branch in current source and compare
the sequence with accepted L73/L74 model success. It must independently trace
the local, packaged, staged, manifest-bound, and remote helper hashes. The two
findings remain separate. If exact localization is impossible, L80 may specify
one minimal future discriminator but may not implement it.

No source edit, build, host or production access, model load, retry, cache
promotion, performance work, or automatic follow-on is authorized.

## 2026-07-28 — accept L80 diagnosis and open L81 decisive postcompute gate

Decision: accept L80 read-only diagnosis. Open L81 to make the already-existing
postcompute failure result durable at its source, explicitly bind the
server-authority harvester to the worker-side path and exact hash, qualify both
offline, and then consume one primary attempt with an early warmup kill gate.

Reason: absence of the unconditional graph-compute failure log, combined with
the authenticated server prepare/execute/publication, proves graph compute
succeeded. The only reachable `GGML_STATUS_FAILED` families are L44 mutable
commit, L42 scheduler finalize, L40 result reconciliation, L44 session
finalization, and RPC execution disarm. Each already writes a bounded branch
string in memory, but context warmup fails before the canary's decode wrapper
can harvest it. The exact branch is not recoverable from L79 evidence.

The independent custody defect is exact: the controller derives the helper from
nimo-2 readiness, then invokes that path on worker host nimo-1. The correct
helper exists under the worker source root. L81 must add a closed worker-side
executable/hash binding and preserve all L76 source, key, path, authentication,
atomic-retention, and cleanup checks.

Offline qualification must force all five postcompute failure families and
prove durable output equals the in-memory result without exposing secrets. It
must prove correct worker-helper success and wrong-host, missing, stale, and
hash-mismatch refusal. One independent focused review is sufficient.

Exactly one primary attempt follows. A warmup failure terminates with its exact
branch; warmup success proceeds in the same attempt to the existing one-token
fresh-residency correctness discriminator. No production cache enablement,
server composition, performance work, tuning, semantic correction, extra
attempt, or automatic follow-on is authorized.

## 2026-07-28 — accept L81 exact refusal and open L82 allocation-epoch semantics

Decision: accept L81 terminal NOT PROMOTED at
`d41441864e1fc3ab2b32713c93be341de96cc2bb`. Retain its reviewed durable
failure discriminator and corrected server-authority custody mechanics. Open
L82 to classify and correct the exact admitted-session allocation-topology
epoch rollover. Permit one primary attempt only after focused and real
no-model lifecycle qualification.

Reason: authenticated server authority proves
`ADMISSION_ACCEPTED -> ABORT` with reason `WRONG_ALLOCATION_EPOCH`, zero
execute receipt, and no physical prepare or backend execution. Exact source
increments `allocation_topology_epoch` and aborts an admitted session on buffer
allocation/free. The real primary lifecycle performs such an operation after
admission. This is the first exact semantic boundary; L81 supports no cache
correctness conclusion.

L82 must prove which post-admission operations are scheduler execution-local
versus persistent model/state topology. Its correction may reorder sealing
after protected allocation or separate proven execution-local identity from
the protected epoch, but immutable accepted admission and exact epoch checks at
prepare/execute must remain. It may not suppress epoch increments broadly,
make refusal advisory, or tolerate unexpected persistent allocation/free.

Focused persistent/transient, free, wrong-epoch, overflow, duplicate/conflict,
and cross-session checks plus one real two-process Linux no-model composed
success and unexpected-topology refusal are sufficient. Feature-off inertness,
required builds, L81/L76 evidence behavior, and one independent review are
required. Do not repeat broad authority matrices.

Only after those gates may one primary attempt run with the existing warmup
kill gate and, on warmup success, continue to the one-token fresh-residency
correctness discriminator. No production cache enablement, server composition,
performance work, tuning, extra attempt, or automatic follow-on is authorized.

## 2026-07-28 — correct L82 causality and open L83 pre-prepare discriminator

Decision: accept L82's blocker before implementation/runtime and revoke the
allocation-epoch correction path. Open L83 only to durably identify the silent
client refusal after accepted admission and before graph compute. Permit one
warmup-only primary attempt after focused offline branch proof and review.

Reason: exact timestamps and source ordering prove the persistent model and two
scheduler/gallocr buffers were allocated before admission. The first free
occurred only after `llama_decode -3`, followed by scheduler and persistent
model teardown. `free_buffer()` then incremented the epoch and terminalized the
still-admitted server session as `WRONG_ALLOCATION_EPOCH`. Client
`mutable_abort()` closes only local state and sends no server abort. Thus the
L81 server record is a secondary teardown event, not the forward cause.
Exempting allocation/free would weaken persistent-topology protection.

L83 must add bounded nonsecret branches for every silent interval path:
scheduler census lookup, invalid disposition, register/exclude refusal with
safe role/ordinal/index identity, mutable prepare refusal with safe status, and
graph-input-authority refusal. Existing semantics and epoch behavior remain
unchanged. Offline fixtures must force each branch, match the in-memory result,
and preserve L81 postcompute diagnostics. One independent review is sufficient.

One primary attempt may run only as a warmup discriminator. Failure retains the
exact client branch plus authenticated server custody and stops before workload
or state operations; unexpected warmup success also stops and reports. The
local-only client abort/server-session terminalization gap is recorded for a
later protocol decision after the forward refusal is known.

No server-abort protocol change, semantic correction, cache correctness run,
production cache enablement, server composition, performance work, tuning,
extra attempt, or automatic follow-on is authorized.

## 2026-07-28 — stop at L83 production uncertainty and reconcile read-only

Decision: classify L83 as terminal NOT PROMOTED for model/cache purposes. Its
warmup discriminator is accepted evidence of
`l44_mutable_exclude_refused` at backend 0, census index 253, disposition 2,
role 1, ordinal 579. Stop all implementation and model work. Authorize only one
bounded read-only production reconciliation; do not restart or mutate the
currently healthy services.

Reason: the warmup kill gate held and authenticated server authority was
retained, so L83 names the exact next semantic boundary without a cache result.
During recovery the coordinator initially returned HTTP 503 and both named
production units advanced `NRestarts` from 0 to 1. The controller correctly
withheld final authority. A later snapshot shows exact services active/running,
unique expected listeners, and coordinator HTTP 200, but changed restart
counters require attribution before accepting a new baseline.

The read-only reconciliation must bind exact system-unit state, PIDs,
InvocationIDs, ExecStart/argv/cgroups, executable identities, listener
ownership, HTTP health, and journals spanning recovery. If both restarts are
fully attributable to the authorized recovery window with no continuing fault,
the Lead may accept the healthy state as a new observed baseline. Otherwise a
separate mutation decision is required.

No counter reset, service restart, configuration change, implementation work,
model access, retry, or automatic follow-on is authorized.

## 2026-07-28 — accept attributed restart baseline and remove exact L83 staging

Decision: accept the current healthy production state as the new observed
baseline: nimo-1 coordinator PID `2791438`, InvocationID
`037044282f2445d5814e44562858cec0`, `NRestarts=1`, port 8081/HTTP 200; nimo-2
worker PID `1980935`, InvocationID `fdb16161c9474e7c9fc33b43f29f45c7`,
`NRestarts=1`, port 50052. Authorize deletion only of four verified inert L83
staging artifacts, followed by exact absence and read-only health
reconciliation.

Reason: the controller issued one authorized start per unit. A kernel global
OOM later killed the recovered worker; its existing on-failure policy restarted
it once. The coordinator aborted shortly after RPC loss and its existing policy
also restarted it once. Journals exclude a second controller start, dependency
activation, or unexplained event. Both current services are exact, unique,
healthy, and stable with matching installed unit/argv/cgroup/executable
authority.

Cleanup scope is limited to nimo-1
`/var/tmp/halofpx-l83-source`,
`/var/tmp/halofpx-l83-source.tar`,
`/var/tmp/halofpx-l83-bin.tar`, and nimo-2
`/var/tmp/halofpx-l83-bin.tar`. Each literal target must be revalidated under
`/var/tmp`, not mounted or process-referenced, and match expected type/owner
before removal. No glob or parent deletion is allowed.

No service restart, counter reset, unit/configuration change, implementation
work, model access, retry, or automatic follow-on is authorized.

## 2026-07-28 — accept exact cleanup and open L84 exclusion diagnosis

Decision: accept PASS for removal of the four exact L83 staging artifacts and
final production reconciliation. Close the production safety hold. Open L84 as
read-only/offline diagnosis of the exact backend-0 census-index-253 immutable
weight exclusion refusal; no new primary attempt is authorized.

Reason: every cleanup target was re-resolved and matched expected path, type,
owner, mode, size, non-mount status, and absence of live references before
literal deletion. Exact absence and depth-one scans passed. Production remains
on the accepted PID/InvocationID/`NRestarts=1` baseline with unique listeners
and HTTP 200.

L84 must distinguish every `mutable_exclude()` refusal condition and reconstruct
the canonical census entries that can share the entry's runtime tensor or
stable identity. It must trace root/copy provenance, disposition, role/ordinal,
view/storage ancestry, backend/socket selection, and L71 deduplication
semantics using retained evidence and exact source. The local-only client
abort/server teardown gap remains separate.

If exact evidence is insufficient, L84 may specify one minimal offline
metadata/census extraction or no-model discriminator. No source edit, build,
host/model access, production mutation, runtime attempt, or automatic follow-on
is authorized.

## 2026-07-28 — accept L84 projection diagnosis and open L85 storage authority

Decision: accept L84 read-only diagnosis. Open L85 to make resolved storage and
RPC destination identity part of the immutable canonical census, add typed
register/exclude refusal evidence, qualify the design offline across two RPC
sessions, and then run one warmup-only primary discriminator. No cache
correctness run is authorized.

Reason: exact source eliminates invalid input, pointer-level register/exclude
conflict, exact duplicate, conflicting duplicate, and session bookkeeping.
The remaining causes are resolved storage absent/non-RPC or an RPC buffer on a
different admitted socket; both currently collapse to `WRONG_CONNECTION`.
Canonical construction binds stable source and runtime tensor pointer but not
the recursively resolved storage tensor/buffer/socket later required by L44.
Distinct base/view tensors sharing storage can evade its conflict check.

L85 must resolve full view/storage ancestry after graph allocation, reject
cycles/unresolved/non-RPC/wrong-destination storage, bind logical and storage
stable identities plus backend/endpoint/device/socket identity, and derive
sealing and iteration from the same projection. Exact semantic aliases alone
may deduplicate. Existing wrong-connection and duplicate refusals remain
strict; mismatches may not be dropped silently.

Typed result evidence must distinguish invalid role/input, missing session,
already registered, absent/non-RPC storage, wrong socket, exact duplicate,
conflicting duplicate, recorder failure, and success without exposing pointers
or data. Offline base/view/nested-view/cycle/two-session fixtures, root/count
equality, feature-on/off builds, inertness, and one independent review are
sufficient.

After those gates, one primary attempt may export the exact bounded canonical
entry and typed result. Any rejection/failure stops; warmup success also stops.
No workload, capture, restore, server-abort protocol change, production cache
enablement, server composition, performance work, tuning, extra attempt, or
automatic follow-on is authorized.

## 2026-07-28 — retain L85 source and open L86 typed projection export

Decision: accept L85 terminal NOT PROMOTED at
`6f1f962ae0cb5670e727d4b2bfdbbcc462649f91` and retain its independently
reviewed default-off source. Accept the recovered `NRestarts=0` production
authority recorded in current status. Open L86 only to preserve the exact typed
projection failure and bounded candidate through scheduler census rollback,
then run one warmup-only primary discriminator.

Reason: L85 correctly failed before admission, but its scheduler conflict path
cleared the census and lost the required entry/reason. The durable result
`l42_resolved_census_refused|typed_reason=0` cannot identify the primary
conflict. This is an evidence-completeness blocker, not a cache or model
correctness result.

L86 must distinguish cycle, unresolved/non-RPC storage, wrong destination
backend/endpoint/device/socket, logical/stable identity conflict, resolved
storage disposition or role conflict, overflow, and invalid input. It must
export backend, candidate index, root/copy provenance, disposition/role/
ordinal, and pointer-independent logical/storage identities without secrets or
raw pointers. Failure must survive rollback; success exports no failure.

Offline fixtures must force each reason and retain reproducible outputs,
including the L85 storage and feature-off gates. One independent focused review
is sufficient. One primary warmup-only attempt follows; any refusal or success
stops before workload/state operations. No semantic correction, server-abort
change, cache correctness run, production cache enablement, server composition,
performance work, tuning, extra attempt, or automatic follow-on is authorized.

Preserve all L83/L85 evidence directories. Exact reproducible local L85 source
archive and build directories may be removed only after workspace/reference
validation; no other deletion is authorized.

## 2026-07-28 — accept L86 exact local candidate and open L87 RPC filtering

Decision: accept L86 terminal NOT PROMOTED at
`4c77d7af13ae03b425ccf32377af7e8bc1024aa8` and retain its reviewed typed
diagnostic source. Accept the recovered production authority recorded in
current status. Open L87 to exclude legitimate non-RPC destination candidates
from the RPC admission/L44 census, then run one gated primary attempt that may
continue to correctness on warmup success.

Reason: L86 identifies backend 1 candidate 487 as a local mutable root,
selected-KV role 11/ordinal 64, rejected as
`WRONG_DESTINATION_BACKEND` before storage resolution. The canonical census is
consumed only for RPC admission and L44 iteration, but its builder currently
includes roots/copies for local backends. The resolver then correctly rejects
the local destination because it is not in the authenticated RPC backend set.

L87 must filter only candidates whose declared destination is non-RPC before
RPC census sealing. Local graph execution remains unchanged. Candidates that
claim an RPC destination may not be dropped and must retain strict resolved
storage/backend/endpoint/device/socket and conflict checks. Mixed local/RPC,
root/copy, falsely RPC-targeted storage, root/count equality, two-session,
feature-on/off, real no-model success/refusal, and one independent review are
sufficient.

After those gates, one primary attempt may run. Warmup failure stops with exact
typed evidence. Warmup success continues in the same attempt to residency-A
reference/capture and fresh-residency-B one-token restore. Acceptance requires
exact token/state equality, zero legacy state-page GET/SET, accepted client/
server custody, cleanup, and recovery. No retry, runtime correction,
server-abort change, production cache enablement, server composition,
performance work, tuning, or automatic follow-on is authorized.

## 2026-07-28 — accept L89 and open L90 pre-mutation disposable reconciliation

Decision: accept L89 terminal NOT PROMOTED at
`8c71aab90e8fdec124bbd593eb53c08ec54b2861` and retain its independently
reviewed paired response-boundary correction. Open L90 for one replacement
primary correctness attempt after a single controller-mechanics prerequisite.

Reason: L89 proved and qualified the exact response ownership contract, but
its transition stopped before model launch because a stale disposable L48 unit
was loaded `active/exited` with no process. The guard behaved safely; cleanup
subsequently proved the unit and all disposable state absent. This consumed no
model/cache evidence and does not justify reopening response semantics.

Before production shutdown or transition consumption, L90 must reconcile the
closed disposable L48/L89/L90 unit/path set. It may unload only an exact,
expected inactive/exited disposable with no PID, process, listener, live
reference, or ownership ambiguity, then must re-prove absence. Active,
unknown, near-name, referenced, or ambiguous state refuses before mutation.
Focused tests must prove stale-unit cleanup, genuine-active refusal, unknown
refusal, idempotent absence, and ordering before production shutdown; one
independent review is sufficient.

After those gates, exactly one primary attempt may run using the accepted L89
response contract. Warmup success continues to residency-A reference/capture
and fresh-residency-B one-token restore. Acceptance requires exact token/state
equality, zero legacy state-page GET/SET, complete client/server authority and
custody, bounded cleanup, worker-first/coordinator-second recovery, and exact
final production reconciliation. No retry, broad matrix, server-abort change,
production cache enablement, server composition, performance work, tuning, or
automatic follow-on is authorized.

Preserve evidence directories. Only the two exact reproducible L86 source
archives may be removed after validation.

## 2026-07-28 — accept L87 and open L88 derived recorder capacity

Decision: accept L87 terminal NOT PROMOTED at
`eb92a66da1a21ef230597f25dd5002c9890a7af3` and retain its reviewed
local-vs-RPC census correction. Accept the recovered production authority
recorded in current status. Open L88 to derive recorder capacity from admitted
census and grammar maxima, then run one gated primary correctness attempt.

Reason: L87's first EXCLUDE is otherwise admissible but returns typed
`RECORDER_FAILURE`. At that point the recorder contains BEGIN, L42, L44_BEGIN,
and 253 REGISTER events—exactly 256. `HFX_PREEXECUTE_MAX_EVENTS` is hard-coded
to 256 and emission refuses when the current sequence is at least that value.
This is a source-proven bounded-capacity defect, not a storage, socket, census,
or model semantic failure.

L88 must compute a finite maximum covering the maximum admitted register and
exclude cardinalities plus the largest fixed lifecycle, decision, transport,
and terminal expansion among all exact productions. Checked arithmetic/static
assertions and admission-time expanded-stream fit are required. Grammar,
cardinality equality, HMAC chaining, immutable publication, and transport
rules remain unchanged.

Tests must cover 255/256/257, the exact 253-register transition into EXCLUDE,
maximum register/exclude/combined cardinalities, one-over-limit refusal,
success and every abort/transport production, bounded memory, feature-on/off,
and real no-model success/refusal. One independent review is sufficient.

After those gates, one primary attempt may run. Warmup failure stops with exact
evidence; warmup success continues to the one-token fresh-residency correctness
discriminator. No retry, server-abort change, production cache enablement,
server composition, performance work, tuning, or automatic follow-on is
authorized. Preserve evidence; only exact reproducible L87 archives may be
removed after validation.

## 2026-07-28 — accept L88 and open L89 paired response authority

Decision: accept L88 terminal NOT PROMOTED at
`a546ca48c6d997f145ff42be3adcd34adf658d1d` and retain its independently
reviewed finite recorder-capacity correction. Open L89 only for a source-proven
exact response-boundary production pairing the observed late client semantic
events with complete authenticated server success.

Reason: L88 is the first primary attempt to cross the former recorder seam and
complete a 512-token composed decode with authenticated server
prepare/execute/response. The controller then rejected the client stream
because it contained only `client_decode` followed by
`client_receipt_validation`, whereas the current verifier accepts only a full
client transport production or its true prefix. This is now the narrowest
product-path boundary; capture and restore did not run.

Before changing semantics, L89 must prove from exact source and retained
L61/L73/L74/L88 evidence why graph-auth client ownership begins at decode and
where the request/response transport events are owned. If that late-attached
production is not source-proven, stop before runtime. A generic prefix, suffix,
or two-event exception is forbidden.

If proven, accept the two client events only when paired in the same verifier
decision with the complete seven-stage server success. Cross-bind all shared
authenticated identities, including attempt nonce, parent/split UID, execution
sequence, backend ordinal, opcode, connection identity, and receipt identity
where each is legitimately common. Refuse client-only input, incomplete or
refused server authority, missing/reordered/extra events, mismatches, failure
statuses, replay, and wrong opcode. Existing full client productions and
failure prefixes remain unchanged.

Qualification is bounded to focused offline positives/negatives, replay of the
retained L88 stream, feature-off inertness, a real no-model paired
success/refusal fixture, and one independent review. After those gates, exactly
one primary attempt may run. Warmup success continues in that same attempt to
residency-A reference/capture and fresh-residency-B one-token restore.
Acceptance requires exact token/state equality, zero legacy state-page GET/SET,
accepted client/server custody, cleanup, and production recovery. No retry,
server-abort protocol change, production cache enablement, server composition,
performance work, tuning, or automatic follow-on is authorized.
