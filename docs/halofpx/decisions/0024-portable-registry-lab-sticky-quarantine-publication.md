# ADR-0024: portable registry-lab sticky-quarantine publication

- Status: accepted for fake-only implementation; Linux and persistence remain closed
- Date: 2026-07-18

## Decision

The next L05r lane may extend only the existing portable fake registry-lab
target with target-owned sticky-quarantine diagnosis and publication. The lane
remains `STATIC EXCLUDE_FROM_ALL`, uses only fake state and injected primitive
products, and has no public header, product edge, filesystem primitive, Linux
adapter, provider, cache, restore, inference, or persistent-write authority.

A quarantine action is admitted only when operation 5 independently derives
one exact encodable diagnosis from the complete locked snapshot. The engine
then obtains a fresh fake event ID at operation 69, revalidates the complete
diagnosis and mutation budget at operation 6, and publishes one exact
authenticated QUARANTINE through operations 70-76. It always returns
`quarantined_or_unavailable`; durable fake publication is not a positive
disposition and grants no repair, retry, clear, reuse, or authority.

This decision corrects the earlier operation list by adding operation 76,
source `staging/` directory synchronization after the cross-directory
quarantine rename. Operation 75 synchronizes the destination root directory.
Both are required before the fake can classify the final namespace projection
as complete. Operation 74 remains an atomic no-replace rename.

Normal compare-and-advance operations 10 onward, initialization, concrete
Linux primitives, and every persistence gate remain closed.

## Preconditions and exact nonpublication cases

Existing `QUARANTINE` or `staging/QUARANTINE.tmp`, whether empty, partial,
malformed, unauthenticated, live-only, or durable, blocks all mutation before
operation 69. Initialization state also remains nonmutating. Neither case
creates a second quarantine or replaces an existing name.

An operation-5 sticky classification alone is insufficient to publish. The
engine must also prove all of the following from independently authenticated
state already admitted by the accepted wire layer:

1. one exact initialized root supplies the lab-root ID and path-policy
   commitment and matches the fixed preflight/credential scope;
2. one exact public reason and phase are derivable without caller input;
3. every non-null attribution field has exact authenticated evidence;
4. the existing target-owned quarantine witness can admit that evidence after
   a future event ID is supplied; and
5. the preallocated 1 KiB encoding scratch and all non-event inputs are valid
   before operation 69.

If the marker, scope, or credential cannot supply an authenticated root for a
valid quarantine envelope, if the diagnosis is ambiguous, or if the wire
witness/encoder rejects the derived material, the engine returns
`quarantined_or_unavailable` without operation 69 or mutation. It must not
invent a root ID, path-policy commitment, reason, phase, digest, key tuple,
attempt, slot, or operation commitment merely to persist an error marker.

## Closed reason taxonomy and deterministic precedence

The public reason values remain the reserved ADR-0020 values:

| code | meaning |
|---:|---|
| 0 | unknown fail-closed |
| 1 | marker invalid |
| 2 | layout or unexpected entry |
| 3 | existing quarantine |
| 4 | HEAD invalid or unavailable |
| 5 | selected envelope invalid or unavailable |
| 6 | journal record invalid |
| 7 | chain contradiction |
| 8 | multiple unresolved attempts |
| 9 | referent missing or invalid |
| 10 | staging ambiguous |
| 11 | durability unproved |
| 12 | scope or root mismatch |
| 13 | key or authentication mismatch |
| 14 | quarantine blocked by resource or I/O failure |
| 15 | internal invariant failure |

Codes 0, 14, and 15 are fail-closed terminal diagnoses, not catch-all ways to
turn an unproved record into an encodable one. Code 14 describes why a
quarantine action could not be completed; the engine never recursively starts
a second quarantine publication after operation 69. Code 3 is observed only
as the pre-operation-69 blocking case and is never republished.

When multiple snapshot defects coexist, the fake derives one stable reason by
this precedence, independent of directory order and request values:

1. key/authentication mismatch (13), scope/root mismatch (12), marker invalid
   (1), then internal invariant failure (15);
2. layout/unexpected entry (2) and staging ambiguity (10);
3. HEAD invalid/unavailable (4) and selected-envelope invalid/unavailable (5);
4. journal invalid (6), chain contradiction (7), multiple unresolved attempts
   (8), and referent missing/invalid (9); and
5. durability unproved (11), then unknown fail-closed (0).

The implementation must derive all applicable flags during the complete fixed
scan and select only after the scan; early directory enumeration cannot choose
the reason. The existing top-level operation-5 recovery precedence remains
unchanged: already-present quarantine and initialization still outrank a new
sticky diagnosis, which outranks recovered CLOSE/ABORT and request handling.

If independent review finds two causes indistinguishable with the accepted
decoder surface, the implementation must conservatively select the earlier
reason or remain nonpublishing. It may not add a parser oracle axis merely to
produce a more specific public reason.

The overlapping English labels are resolved as follows. Code 13 applies only
when the marker's declared credential tuple or authentication fails, so no
trusted signing/root context exists. An authenticated marker that mismatches
the independently admitted store/root/path scope is code 12. A marker that is
absent, structurally malformed, incomplete, or otherwise not a candidate for
authentication is code 1. If exact authenticated compatible marker bytes exist
but their file/namespace durability is unproved, that is code 11 instead.

After a compatible root is established, kind-local structural,
authentication, or semantic failure uses the kind-local reason: HEAD 4,
selected envelope 5, and PREPARE/CLOSE/ABORT journal record 6. Code 7 is
reserved for individually authenticated compatible records whose cross-record
chain is contradictory. Code 8 requires more than one individually valid
unresolved PREPARE. Code 9 requires an otherwise exact authenticated reference
to an absent or invalid referent. Code 10 is a per-slot successor/selector
staging ambiguity; root quarantine staging is instead the earlier existing-
quarantine blocker. Code 2 covers fixed-layout or unexpected-entry defects not
already named above. This partition is normative and must have single-cause
and overlapping-cause tests.

## Attribution and phase contract

All QUARANTINE fields are derived by the engine. Scripts and callers provide no
event ID, reason, phase, optional-presence bit, attempt, slot, previous-record
digest, HEAD digest, or operation commitment.

- `attempt`, `slot`, and `operation commitment` are either all null or all
  present. They are present only when one exact authenticated PREPARE is the
  attributable trigger and its body has passed root, slot, transition,
  predecessor, successor, and operation-commitment binding.
- `previous authenticated record digest` is present only with the same one
  uniquely attributable authenticated PREPARE as the attempt/slot/operation
  tuple. Without that attribution, it is null.
- `last readable authenticated HEAD digest` is present only for one exact
  authenticated HEAD that matches the admitted lifecycle evidence as either
  predecessor or successor. A merely parseable, present, or shape-compatible
  HEAD cannot populate it.
- phase 0 means no exact authenticated PREPARE can be attributed; phase 1 means
  the exact attributable PREPARE exists without an authenticated successor
  HEAD; phase 2 requires an exact authenticated successor HEAD for that same
  transition. Presence, generation maximum, filename order, or tensor shape
  never advances phase.

The lane admits only these four exact optional-field shapes; the implementation
must reject every other combination even if the lower-level wire encoder could
represent it:

| shape | exact admitted evidence | attributable tuple | previous PREPARE digest | HEAD digest | phase |
|---|---|---|---|---|---:|
| U0 | authenticated compatible root, but no exact lifecycle HEAD or PREPARE usable for this diagnosis | all null | null | null | 0 |
| UH | U0 plus one exact authenticated predecessor HEAD | all null | null | exact predecessor HEAD digest | 0 |
| P | one exact fully bound PREPARE with predecessor HEAD and transition evidence | exact attempt, slot, and operation commitment | exact PREPARE content digest | exact predecessor HEAD digest | 1 |
| S | P plus the exact authenticated successor HEAD for that transition | exact attempt, slot, and operation commitment | exact PREPARE content digest | exact successor HEAD digest | 2 |

An invalid or merely parseable PREPARE never supplies partial attribution. One
uniquely attributable valid PREPARE must use P or S; it is never represented
as unattributed. Multiple valid candidates select no attempt and may use U0 or
UH only when that shape truthfully represents the remaining independently
authenticated evidence. A successor phase without exact P evidence is
impossible. If no exact permitted shape exists, the diagnosis is
nonpublishing.

The target-owned quarantine witness is the final authority on whether U0, UH,
P, or S is encodable. Failure to construct it is nonpublishing and
`quarantined_or_unavailable`.

For each reason, the plan chooses the highest-evidence truthful shape in the
fixed order S, P, UH, U0, subject to this closed compatibility table. If the
highest truthful shape is not admitted for the selected reason, the engine does
not silently drop evidence to use a lower shape; it remains nonpublishing.

| reason | admitted publication shape |
|---:|---|
| 0, 1, 3, 12, 13, 14, 15 | none; classification only |
| 2 layout/unexpected entry | U0, UH, P, or S |
| 4 HEAD invalid/unavailable | U0 only |
| 5 selected envelope invalid/unavailable | UH or S |
| 6 journal record invalid | U0 or UH; the invalid record cannot attribute itself |
| 7 chain contradiction | U0, UH, P, or S, but only from other exact compatible evidence |
| 8 multiple unresolved attempts | U0 or UH; no attempt is selected |
| 9 referent missing/invalid | UH or P |
| 10 staging ambiguous | U0, UH, P, or S |
| 11 durability unproved | U0, UH, P, or S |

Codes 3 and 14 are never selected for a new publication plan. Codes 0 and 15
are tested residual fallbacks but are deliberately nonpublishing. Reasons 1,
12, and 13 remain nonpublishing because they cannot establish the compatible
authenticated root/key scope required to sign a trustworthy record.

## Operation 69 event-ID authority

Operation 69 is `quarantine_event_id_acquire`. It has storage effect `none`;
completion may be confirmed, response-lost, or process-death; and the only
admitted primitive codes are `ok`, `unsupported`, `unavailable`, and
`io_failure`. Confirmed
`ok` is valid only when the engine's injected fake operation produces one
nonzero 256-bit ID.

The script selects only the primitive product, never the ID. A private
move-only `quarantine_event_id_witness` is constructed only by the fake Ops
implementation. It binds one nonzero domain-separated fake ID to the exact
invocation and diagnosis commitment, is consumable exactly once by operation
6, and is invalid after move or consumption. The engine cannot construct,
copy, edit, or replay it.

The fake Ops uses one bounded process-global monotonic issuance sequence outside
`fixed_state` and outside restart serialization. Its 256-bit fake ID format
contains a fixed test-only domain discriminator and the exact nonzero issuance
sequence, so distinct nonwrapped sequence values are provably distinct without
a hash-collision assumption. Every operation-69 call that reaches its fake
primitive consumes one sequence value, including response loss and modeled
process death. Confirmed `ok` returns the witness; lost response or death
delivers none. Sequence exhaustion or wrap fails closed before operation 6.
The sequence is thread-safe, but its concrete value is not a golden vector or
reproducibility claim; tests assert only binding, nonzero, and distinctness.

Modeled restore shares that private Ops issuance domain in the same test
process and therefore obtains a byte-distinct ID even when no quarantine
namespace survived. A real process restart may reset the fake test sequence;
cross-incarnation byte distinctness is not claimed and is not authority. An
unpersisted prior ID authorized no mutation, while any persisted quarantine
prefix or name blocks reacquisition. A future Linux adapter requires a
separately qualified OS CSPRNG and collision policy.

The public event ID is not a credential or secret and is intentionally present
inside an encoded QUARANTINE. The move-only witness, issuance sequence, and
unpublished scratch are private and wiped/excluded from restart images, traces,
results, and retained evidence. Projection must preserve, never redact or
invent, the public event ID inside retained exact/prefix record bytes.

The 32-byte fake event ID has one exact noncryptographic test-only encoding:
bytes 0-15 are ASCII `HALOFPX-L05R-EV1`; bytes 16-23 are the big-endian fixed
issuance-domain value `0x484650584c303552`; and bytes 24-31 are the big-endian
nonzero process-global issuance sequence. Zero, wrap, or reuse is rejected.
This encoding exists only to make fake uniqueness and the independent oracle
exact; it is forbidden as a Linux event-ID construction.

Operation 69 occurs after complete diagnosis and non-event encoding
preparation and before operation 6. Confirmed failure or lost response performs no mutation and
returns `quarantined_or_unavailable`; death returns no ordinary result. A later
retry may obtain a different ID. Any existing final or staging quarantine then
blocks retry before operation 69.

After confirmed operation 69, the engine combines the private witness with the
plan, encodes into preallocated private scratch, decodes and authenticates the
exact record, rebinds every field, computes the content digest, and only then
approaches operation 6. Any encoder, self-verification, scratch, or witness
contradiction is nonpublishing and wipes scratch.

The diagnosis commitment is SHA-256 over ASCII
`halofpx.registry-lab-quarantine-diagnosis.v1` plus NUL plus one exact
deterministic-CBOR map. Integers and lengths use their shortest encoding and
keys are ordered exactly:

```text
{
  0: 1, 1: invocation-id,
  2: root-id, 3: path-policy,
  4: reason, 5: phase, 6: shape,
  7: attempt / null, 8: slot / null,
  9: operation-commitment / null,
  10: prepare-digest / null,
  11: head-digest / null
}
```

The invocation ID is a nonzero uint64; root/path and every digest are bytes32;
reason is `0..15`; phase is `0..2`; and shape is `0=U0`, `1=UH`, `2=P`, or
`3=S`. The all-or-none and shape rules control every null. This commitment does
not replace the exact state check: operation 6 separately compares every
action-critical field of current `fixed_state` with the immutable operation-4
snapshot. No hash of C++ object representation, padding, pointer, or script is
trusted as state equality.

The event ID, complete diagnosis, every optional-field presence bit/value,
authenticated root/path scope, and exact encoded content digest are included
in a private quarantine-action commitment. It is SHA-256 over ASCII
`halofpx.registry-lab-quarantine-action.v1` plus NUL plus this exact
deterministic-CBOR map:

```text
{
  0: 1,
  1: diagnosis-commitment,
  2: event-id,
  3: root-id, 4: path-policy,
  5: reason, 6: phase, 7: shape,
  8: attempt / null, 9: slot / null,
  10: operation-commitment / null,
  11: prepare-digest / null,
  12: head-digest / null,
  13: quarantine-content-digest,
  14: quarantine-encoded-length
}
```

All commitments/digests/event/root/path values are bytes32; encoded length is
`1..1024`; other types and null rules match the diagnosis map. Operation 6
consumes the witness, recomputes both exact commitments, and
rejects a moved-from, replayed, wrong-invocation, wrong-diagnosis, changed-state,
changed-attribution, changed-encoding, or changed-ID witness before storage
mutation.

## Mutation admission and publication order

Operation 6 remains the atomic dynamic admission boundary. For quarantine it
recomputes the complete action commitment, the 16 MiB worst-case logical-byte
bound including one 1 KiB quarantine object, and the exact 256 MiB reserve. A
confirmed clean operation-6 `unavailable` can arise only from independently
detected state or commitment disagreement. Capacity, reserve, unsupported,
unavailable, I/O failure, response loss, or death never becomes a definite
success.

On confirmed admission, operation 6 latches the quarantine action and invokes
operation 70 in the same engine step. There is no injectable gap in which an
allocation, exception, script edit, or definite no-mutation result can appear.
Execution after fixture construction is allocation-free and `noexcept`.

The only admitted successful trace is:

`1,2,3,4,5,69,[encode and self-verify],6,70,71,72,73,74,75,76,90,91,92`.

The bracketed work is internal allocation-free computation, not an injectable
operation or trace event.

The mutation operations are:

- 70 create fixed `staging/QUARANTINE.tmp` with no replacement;
- 71 write the exact pre-encoded authenticated bytes;
- 72 read back and authenticate exact bytes, semantics, and content digest;
- 73 synchronize the complete staging-file bytes;
- 74 atomically rename staging to root `QUARANTINE` with no replacement;
- 75 synchronize destination root-directory publication; and
- 76 synchronize source `staging/`-directory removal.

No success status is added. After confirmed complete operation 76, cleanup
90-92 runs in order and the ordinary result remains
`quarantined_or_unavailable`. The fake may expose bounded test-private audit
facts, but no object, path, bytes, digest, event ID, or disposition escapes.

## Primitive-product algebra

| operation | admitted codes | admitted effects | completion and confirmed-success rule |
|---:|---|---|---|
| 69 | `ok`, `unsupported`, `unavailable`, `io_failure` | `none` | C, L, or D; confirmed `ok` requires a nonzero Ops-derived witness |
| 6 | existing ADR-0023 set | `none` | C, L, or D; exact revalidation controls the derived result |
| 70 | `ok`, `unavailable`, `io_failure` | `none` or atomic empty `complete_live` name | C, L, or D; confirmed `ok` requires `complete_live` |
| 71 | `ok`, `unavailable`, `io_failure` | `none`, bounded partial bytes, or exact `complete_live` bytes | C, L, or D; confirmed `ok` requires exact `complete_live` |
| 72 | `ok`, `unavailable`, `io_failure` | `none` | C, L, or D; confirmed `ok` requires exact authenticated readback |
| 73 | `ok`, `unavailable`, `io_failure` | `none`, bounded partial durability, or complete file durability | C, L, or D; confirmed `ok` requires complete file durability |
| 74 | `ok`, `unavailable`, `io_failure` | `none` or atomic `complete_live` cross-directory rename | C, L, or D; confirmed `ok` requires the complete rename |
| 75-76 | `ok`, `unavailable`, `io_failure` | `none`, bounded partial durability, or complete directory durability | C, L, or D; confirmed `ok` requires the corresponding complete projection |

Every unlisted product rejects before engine entry. For lost response or death,
the code is latent oracle-only and the engine observes no returned code.
Confirmed failures and lost responses may retain any effect admitted by the
row. No sync invents bytes or names, and no create/rename produces a partial
name. Cleanup 90-92 remains nonfaultable and is not script input.

After operation 69, every ordinary failure maps to
`quarantined_or_unavailable`; process death has no ordinary result. A
post-latch invariant disagreement cannot be relabeled definite no-mutation.
There is no recursive attempt to publish reason 14.

The eight new operation IDs admit exactly 155 products: 12 for operation 69;
17 for create; 25 for write; 9 for readback; 25 for file sync; 17 for rename;
and 25 for each directory sync. They reject 805 of the 960 code/effect/
completion combinations. Including the previously accepted 342 products, the
portable algebra becomes exactly 497 admitted products. Operation 6 is already
counted in the accepted algebra and is not double-counted here.

## Restart closure for the cross-directory rename

The fake retains file bytes, file-data durability, root-directory durability,
and staging-directory durability independently.

- before operation 70, no quarantine name is added;
- create/write faults may restart with no staging name, an empty staging name,
  an exact issued prefix, or exact complete bytes, never invented bytes;
- after file sync, complete staging bytes may survive only with namespace
  outcomes admitted by the unsynchronized staging directory;
- operation 74 is atomic in the live namespace and never overwrites final
  quarantine;
- before both directory synchronizations, the closed namespace projection is
  exactly one of neither name, staging only, final only, or both names, subject
  to the exact preceding byte and source/destination directory projections;
  no fifth state or partial name is admitted;
- after confirmed operation 75, final `QUARANTINE` is durable; until operation
  76, the old staging name may also survive because source removal is not yet
  proved durable; and
- after confirmed operation 76, restart contains the exact durable final
  QUARANTINE and no staging name.

Any retained final or staging name, including both after a partial
cross-directory durability outcome, blocks every future mutation. An exact
authenticated final quarantine does not grant success or clear authority.
Malformed, partial, conflicting, missing-after-unproved-publication, or
live/durable-disagreeing state remains `quarantined_or_unavailable`.

Restart serialization excludes scripts, credentials, derived keys, event-ID
scratch, action commitments, encoded scratch, traces, and results. Projection
never invents bytes, names, authentication, a reason, event ID, durability, or
cleanup.

## Required qualification before promotion

At minimum, the focused and exhaustive suites must prove:

- exact reason precedence for every single and pairwise defect class, with
  nonpublishing marker/scope/key cases and unchanged operation-5 precedence;
- every admitted optional-attribution shape and rejection of unsupported
  null/presence combinations, phase escalation, cross-attempt fields, stale
  digest, wrong reason, wrong event ID, and recomputed-tag semantic attacks;
- deterministic nonzero/distinct fake event IDs; move-only/wrong-binding/replay
  rejection; issuance consumption on confirmation/loss/death; script inability
  to choose IDs; operation-69 failure/loss/death; and mutation between 69/6;
- exact logical-budget/reserve boundaries and late reserve loss;
- all 155 admitted products for operations 69-76, all 805 forbidden products,
  and the exact 497 cumulative algebra count;
- exact trace and rejection of missing, extra, reordered, repeated, normal-CAS,
  terminal, or wrong directory-sync operations;
- every mutation boundary, all retained/discarded restart projections, and
  restore/reclassification, including final only, staging only, neither, and
  both where the modeled durability state admits them;
- existing final/staging quarantine blocking with zero event-ID acquisition;
- 1 KiB exact/over-limit and 16 MiB plus reserve arithmetic boundaries;
- allocation-free `noexcept` execution, wipe-before-release, modeled-death
  teardown, private freshness-witness/issuance-state exclusion, public event-ID
  exactness inside retained record bytes, and secret exclusion;
- Windows Release/Debug, Linux optimized and ASan/UBSan, feature-off, full
  HaloFPX, and focused inherited regressions;
- source/dependency/link/archive/import/product-backlink isolation with
  synthetic negative controls; and
- repeated exact-candidate matrices, retained raw hashes/commands/results, and
  an independent milestone review.

## Provenance, rollback, and explicit non-claims

This target-native design derives from ADR-0018 through ADR-0023, the accepted
target-owned wire contract, and canonical Wiki Section 63. No donor
implementation or format is imported. No GPL llama-ai code or separately
licensed documentation enters the MIT engine; no CachyLlama code is copied;
the direct-cherry-pick roster remains empty and no P3 record is required.

Rollback is removal of this excluded fake-only slice and its tests. It creates
no node, filesystem, service, model, cache, or deployment state.

This decision does not authorize normal CAS, initialization, Linux credentials
or syscalls, a concrete observation, filesystem/process-crash/power-loss
evidence, M63-01 completion, production key custody, persistent writes, a
durability mode, provider/server linkage, cache hit or restore, inference
behavior, performance claims, or the optional L14Q lane.
