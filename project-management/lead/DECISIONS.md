# Project-Lead Decisions

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
