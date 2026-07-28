# Project-Lead Decisions

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
