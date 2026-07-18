# ADR-0020: portable registry-lab read-only prologue

- Status: accepted for operations 1-5 only
- Date: 2026-07-18

## Decision

The first portable-engine submilestone implements only the read-only prologue
of ADR-0019 over an internal `final` fake-operations type:

1. guard acquire;
2. candidate writer-lock acquire;
3. under-lock preflight;
4. fixed-layout and bounded snapshot load; and
5. authenticated recovery validation.

It cannot execute recovery, quarantine, compare-and-swap, reserve-transition,
staging, synchronization, terminal publication, initialization, or any other
mutation. It owns no path, descriptor, syscall, clock, RNG, thread, callback,
environment lookup, logging sink, Linux type, or concrete authority result.

The target remains `STATIC EXCLUDE_FROM_ALL` with no product, server, provider,
material, anchor, cache, restore, or inference backlink. Its fake-only test
driver is declared in an uninstalled internal header and may be linked only by
the focused test target. No clean-state result or absence result is exposed to
another target.

## Private authenticated decoder

The existing expectation-shaped validator and write-side lifecycle witnesses
cannot decode arbitrary persisted state: constructing an expectation from the
same untrusted record would be circular. The read-only target therefore owns a
private decoder with this order:

1. Copy the complete bounded object into kind-specific fixed storage: 1-1024
   bytes, except PREPARE at 1-4096 bytes.
2. Enforce deterministic CBOR, exact key count/order, shortest forms, closed
   fields, and complete consumption.
3. Parse the key ID/generation into private scratch and compare them to the
   independently admitted credential-package tuple. A mismatch rejects before
   key derivation; record-supplied values never select an authentication key or
   derivation context.
4. Derive `K_lab` only from the independently admitted credential package,
   then verify the purpose-separated HMAC in constant time.
5. Compute the target-owned content digest.
6. Only then expose a private typed body to the semantic binder.

The decoder returns a move-only, privately constructible
`authenticated_record_v1<Kind>` containing exact retained bytes/length, content
digest, authenticated key ID/generation, and a private typed body. It has no
public constructor, header, factory, or linkable bypass symbol.

Authentication alone grants no authority. A second private binder compares the
authenticated body to separately admitted under-lock path policy, credential,
lock, registry, predecessor/successor, and chain context. Fields with external
preflight evidence, including mount identity, owner/mode policy, layout
version, and credential tuple, must match that evidence. Marker-origin identity
fields that cannot precede marker admission, including the random lab-root ID,
are accepted only from the authenticated marker and then become the exact
expected values for every subsequently decoded object in this invocation. They
are never compared to a value copied from the same untrusted bytes. HEAD is
decoded only after this root admission.
Referenced ADR-0013/0014 registry envelopes use their independent authenticated
verifiers; an outer L05o HMAC never substitutes for inner verification.

The decoder never calls an encoder to establish acceptance. Tests may compare
independent decode and encode results, but re-encoding is not admission proof.

## Closed fake state

The fake uses fixed-capacity typed storage rather than a filename map:

- one marker, lock, HEAD, QUARANTINE, and quarantine-staging entry;
- 512 slots, each with PREPARE, CLOSE, ABORT, successor-staging, and
  selector-staging entries;
- the initial registry envelope and at most 512 immutable successors, keyed by
  exact digest; and
- bounded unexpected-entry metadata sufficient to force fail-closed recovery.

Each modeled file has independent live and durable namespace presence, live and
durable bytes, exact bounded length, and completeness. Directory projections
are likewise separate. The read-only lane cannot change either projection.
Serialized restart images contain no script, trace, credential, derived key,
tag, or scratch state.

## Three-axis primitive result

ADR-0019's storage-effect and completion axes remain orthogonal. This decision
adds a latent primitive-code axis so a successful no-effect read is distinct
from a confirmed no-effect failure.

For operations 1-5, storage effect is always `none`. Primitive code is closed:

- `ok`;
- `busy`;
- `unsupported`;
- `invalid_request`;
- `capacity_exhausted`;
- `reserve_exhausted`;
- `unavailable`; or
- `io_failure`.

With `response_confirmed`, the engine observes the code and any admitted
payload. With `response_lost`, the fake retains the listed latent code only for
oracle comparison; the engine receives neither code nor payload. With
`process_death`, the fake likewise retains the listed latent code only for the
oracle, no payload reaches the engine, no ordinary engine status exists, and a
later check begins as a new invocation over the restart projection.

The fake rejects payloads on non-`ok` results, classifications outside operation
5, non-`none` effects for operations 1-5, and every unlisted
operation/effect/completion/code product before engine entry.

The following table is the complete admitted product. `C`, `L`, and `D` mean
`response_confirmed`, `response_lost`, and `process_death`. Every cell has
effect `none`; an omitted completion/code pair is forbidden. The code set in a
`C/L/D` cell applies independently to each listed completion.

| operation | admitted completion and code products | exact confirmed result |
|---|---|---|
| 1 guard acquire | C or D x {`ok`, `busy`} | `ok` -> continue; `busy` -> `busy_no_mutation` |
| 2 writer-lock acquire | C or D x {`ok`, `busy`, `unsupported`} | `ok` -> continue; `busy` -> `busy_no_mutation`; `unsupported` -> `unsupported_no_mutation` |
| 3 under-lock preflight | C, L, or D x {`ok`, `invalid_request`, `unsupported`, `capacity_exhausted`, `reserve_exhausted`, `unavailable`, `io_failure`} | `ok` -> continue; `invalid_request` -> `invalid_request_no_mutation`; `unsupported` -> `unsupported_no_mutation`; `capacity_exhausted` -> `capacity_exhausted_no_mutation`; `reserve_exhausted` -> `reserve_exhausted_no_mutation`; `unavailable` or `io_failure` -> `quarantined_or_unavailable` |
| 4 snapshot load | C, L, or D x {`ok`, `unsupported`, `unavailable`, `io_failure`} | `ok` -> continue; `unsupported` -> `unsupported_no_mutation`; `unavailable` or `io_failure` -> `quarantined_or_unavailable` |
| 5 recovery validation | C, L, or D x {`ok`, `unsupported`, `unavailable`, `io_failure`} | `ok` -> exact classification mapping below; `unsupported` -> `unsupported_no_mutation`; `unavailable` or `io_failure` -> `quarantined_or_unavailable` |

Operations 1 and 2 continue to forbid `response_lost`. For operations 3-5,
`response_lost` returns `quarantined_or_unavailable` after ordinary cleanup.
Because this lane cannot mutate storage, it never returns
`uncertain_requires_recovery` merely for a lost read response.

## Read-only recovery classification

Operation 5 examines all 512 slots, the complete fixed layout, every occupied
record, and every referenced immutable envelope under the modeled lock. Its
private classification is closed:

- `continue_to_mutation`;
- `needs_successor_close`;
- `needs_predecessor_abort`;
- `needs_sticky_quarantine`;
- `blocked_by_existing_quarantine`;
- `inadmissible_initialization_artifact`;
- `attempt_replayed`;
- `capacity_exhausted`;
- `requested_slot_occupied`;
- `invalid_transition`; or
- `preexisting_unattributed_material`.

The read-only lane stops at classification and performs no associated action.
Mappings at this boundary are:

- successor CLOSE or predecessor ABORT needed ->
  `uncertain_requires_recovery`;
- sticky quarantine needed or already present ->
  `quarantined_or_unavailable`; and
- an authenticated initializing marker ->
  `preexisting_material_no_authority`; and
- replay -> `attempt_replayed_no_mutation`;
- full capacity -> `capacity_exhausted_no_mutation`;
- occupied requested slot -> `slot_occupied_no_mutation`;
- invalid transition -> `invalid_transition_no_mutation`;
- unattributed pre-existing material ->
  `preexisting_material_no_authority`; and
- clean `continue_to_mutation` -> private test classification only, with no
  public result or reusable authority.

Classification and request admission use this single global precedence; the
first matching condition wins even when later conditions also exist:

1. Any `QUARANTINE` or `QUARANTINE.tmp` name, including malformed content,
   missing referents, or coexistence with an initializing/invalid marker ->
   `blocked_by_existing_quarantine`.
2. With no quarantine name, an authenticated marker whose authenticated phase
   is exactly `initializing` -> `inadmissible_initialization_artifact`.
3. A missing, incomplete, malformed, unauthenticated, unsupported, or
   externally incompatible marker; multiple unresolved attempts;
   contradictory records; missing/invalid referents; unexpected entries;
   ambiguous staging; invalid authenticated state; or any other unprovable
   recovery boundary -> `needs_sticky_quarantine`.
4. Exactly one unresolved PREPARE and an authenticated HEAD/resolved envelope
   byte-exactly equal to its successor -> `needs_successor_close`.
5. Exactly one unresolved PREPARE and an authenticated HEAD/resolved envelope
   byte-exactly equal to its predecessor -> `needs_predecessor_abort`.
6. Only after recovery is otherwise clean, apply the request-admission order
   below, ending in `continue_to_mutation`.

The initialization class is limited to the provable authenticated
`initializing` marker. It is never quarantined or repaired by this engine.
Malformed, incomplete, absent, or unauthenticated marker bytes do not prove
initialization provenance and therefore take precedence item 3, not this
class. ADR-0018 permits retaining evidence and discarding an exact disposable
root only when its independently governed initialization procedure proves that
provenance; ordinary reopen has no such proof.

After recovery classification is clean, global precedence item 6 uses this
fixed request-admission order:

1. attempt ID in any slot -> `attempt_replayed`;
2. all 512 slots occupied -> `capacity_exhausted`;
3. requested slot occupied -> `requested_slot_occupied`;
4. request transition does not bind the authenticated current state ->
   `invalid_transition`;
5. requested successor or staging material exists without one attributable
   PREPARE -> `preexisting_unattributed_material`; and
6. otherwise -> `continue_to_mutation`.

This precedence distinguishes a full journal from an isolated slot conflict
without wrapping, eviction, reuse, directory-order selection, or attribution
from equal bytes alone.

No generation maximum, timestamp, directory order, partial decode, digest-only
equality, tensor shape, token prefix, or pre-existing material attributes work
to an attempt or grant authority.

## Credential custody and cleanup

The request move-owns the credential. The engine session keeps credential,
derived keys, tags, authenticated-record scratch, and temporary witnesses in
bounded non-relocating owners. Ordinary response, response-loss, exception,
busy, unsupported, invalid, and unavailable paths that invoked operation 1
record exactly:

90. credential, derived state, and scratch wipe;
91. writer-lock release; and
92. guard release.

The calls are ordered, unconditional, non-faultable, no-throw, and release is a
no-op if ownership was never acquired. No result is exposed before they finish.
A request rejected before operation 1 uses scoped-owner destruction and has no
operation trace.

Modeled process death is different: the engine trace stops at the death point
because a killed process executes no cleanup code. The fake harness applies the
normative restart projection, clears modeled guard/lock ownership, discards the
dead invocation's in-memory credential owner, and records a separate
`restart_teardown_audit` proving no secret enters serialized state. It must not
misreport operations 90-92 as calls made by the dead process. Future Linux
process-death qualification must independently prove kernel lock release and
credential-memory behavior.

## First implementation slice

The smallest buildable slice contains:

- closed operation/effect/completion/code algebra and constexpr admission
  table for operations 1-5;
- fixed fake state and restart serialization;
- internal final fake Ops and deterministic contention/fault script;
- move-owned cleanup session;
- operations 1-4; then
- the private decoder/binder and operation 5 classification.

The engine creates no thread. Two-engine exactly-one behavior is tested by
deterministic interleaving against shared fake ownership state. Actual threaded
and cross-process contention remain later Linux gates.

Promotion requires Release and Debug builds, Linux ASan/UBSan, every admitted
and forbidden product for operations 1-5, all 512 slots plus capacity/replay
cases, malformed layout/record/referent cases, response loss/death, exception
and allocation paths, restart round trips, wipe/release ordering, complete
source/dependency/archive/import isolation, feature-off/full/HaloFPX
regressions, and independent adversarial review.

## Reserved mutation prerequisites

The following corrections are recorded now but remain compile-time unavailable
and unimplemented. ADR-0018 and ADR-0019 must be amended before any mutation
lane opens:

- operation 6: atomic action-mutation admission and dynamic 16 MiB
  reserve/uncertainty transition immediately before each action's first write;
- operation 36: source `staging/` directory sync after successor rename;
- operation 46: source `staging/` directory sync after selector-to-HEAD rename;
- operation 69: fresh quarantine event-ID acquisition from injected Ops before
  operation 6; fake values are deterministic and distinct, while future Linux
  requires a separately qualified OS CSPRNG; and
- exact successor recovery sequence:
  `6,33,35,36,43,45,46,50,51,60,61,62,63,64`.

Operation 43 is generalized for that future sequence to synchronize the
authenticated selector file currently named HEAD. Any retained successor or
selector staging name remains ambiguous and selects quarantine instead.

Future quarantine reason codes are reserved as:

0 unknown fail-closed; 1 marker invalid; 2 layout/unexpected entry;
3 existing quarantine; 4 HEAD invalid/unavailable; 5 selected envelope
invalid/unavailable; 6 journal record invalid; 7 chain contradiction;
8 multiple unresolved attempts; 9 referent missing/invalid; 10 staging
ambiguous; 11 durability unproved; 12 scope/root mismatch; 13 key/auth mismatch;
14 quarantine blocked by resource/I/O; 15 internal invariant failure.

The read-only milestone may classify a reason internally but cannot create a
quarantine event ID or record.

## Explicit non-claims

This decision does not implement or authorize operation 6, 10 or later, 36,
46, or 69; initialization; publication; CLOSE/ABORT/QUARANTINE creation;
reserve enforcement at mutation; Linux syscalls; real locks; filesystems;
process death; persistent cleanup; cache reuse; runtime linkage; a positive
authority observation; or performance claims.
