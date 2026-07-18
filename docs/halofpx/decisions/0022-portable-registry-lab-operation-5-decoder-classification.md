# ADR-0022: portable registry-lab operation-5 decoder and classification closure

- Status: accepted for portable fake-only operation 5 after independent adversarial review; mutation remains closed
- Date: 2026-07-18

## Decision and boundary

This decision closes the remaining operation-5 contract beneath ADR-0020. It
authorizes a later implementation only after independent adversarial review
accepts this record. That implementation remains inside the existing
`halofpx-context-store-registry-lab-read-only` `STATIC EXCLUDE_FROM_ALL` target
and its focused fake-only test. It adds no public header, option, install or
export rule, product edge, Linux primitive, runtime linkage, cache authority,
or persistent write.

Operation 5 receives only a fixture-owned immutable copy captured by operation
4 plus separately admitted preflight and request values. It never reads the
mutable fake state directly. It authenticates and binds the complete captured
layout, then returns one private classification. The clean classification
stops at test event 201, the operation-6 boundary; it is not an ordinary
result, continuation token, reusable absence proof, or mutation authority.

The normative wire authority is
[`context-store-registry-lab-v1.cddl`](../contracts/context-store-registry-lab-v1.cddl).
The accepted Python golden oracle remains independent. Donor code and donor
formats have no role in decoding, authentication, classification, or tests.

## Independently admitted input

Before operation 1 the fixture freezes two fixed, pointer-free values.

`preflight_context_v1` contains the already validated store UUID, filesystem
UUID, subvolume UUID, mount ID, device identity, owner UID, root and authority
file modes, writer-lock device/inode, path-policy commitment, registry ID and
epoch, expected authority-base scope commitment, registry policy commitment,
credential key ID/generation, and an explicit inner-key disposition. It also
contains the fixed capacity and logical-byte limit. The only admitted inner-key
disposition is `context_store_key_disposition::active`; missing, unknown,
revoked, `read_disabled`, or any other disposition fails pre-entry validation.
The random lab-root ID is deliberately
absent: it is learned only from a successfully authenticated marker and then
becomes the exact expected value for all remaining objects in that invocation.

`request_transition_v1` contains a nonzero attempt ID, slot `0..511`, caller-
declared operation commitment, exact predecessor and successor envelope bytes
and lengths, their caller-declared registry-lab envelope digests, and the exact
expected current HEAD envelope bytes, length, and digest. It contains no root
ID, secret, path, credential selector, filename, timestamp, or generation-
selection policy. Operation 5 independently verifies the embedded envelopes,
recomputes their registry-lab digests and operation commitment, and compares
the request to authenticated current state. A caller-declared digest or
commitment never substitutes for recomputation.

These values are copied into the invocation before entry and cannot be
replaced on pause or resume. Their bounded lengths, enum representations,
fixed constants, nonzero requirements, registered-ASCII fields, and
cross-field consistency are checked before operation 1. Rejection wipes the
move-owned credential and produces `invalid_request_no_mutation` with no
operation trace.

## Operation-4 immutable snapshot

Operation 4 copies both complete live and durable projections of every fixed
file, directory, slot, immutable-envelope entry, digest, and bounded unexpected
name into a fixture-preallocated snapshot owned by the invocation. No pointer
or reference to mutable state survives the copy. Snapshot capture is
allocation-free and `noexcept`; capacity or representation overflow is an
operation-4 oracle mismatch and maps to `invalid_request_no_mutation` after
ordinary cleanup. Presence, absence, incompleteness, and contradictory but
representable content are copied without interpretation for operation 5.

Tests must mutate the underlying fake state after operation 4 and prove that
operation 5 uses only the captured copy. Live and durable projections are
classified independently and must agree exactly wherever durability is part
of the admitted state. No live-only value may repair, replace, or select a
durable value, and no durable value may conceal a conflicting live value.

## Private decoder and authenticated carriers

Root, HEAD, PREPARE, CLOSE, and ABORT are decoded by five kind-specific private
routines in the read-only implementation's anonymous namespace. PREPARE accepts
`1..4096` bytes; the other kinds accept `1..1024`.
Each decoder performs this exact order:

1. copy the complete bounded bytes into kind-specific fixed scratch;
2. enforce RFC 8949 deterministic CBOR, definite lengths, shortest forms,
   exact increasing integer keys, exact field count, closed enum and constant
   values, exact registered-ASCII profile, and complete input consumption;
3. parse the outer key ID/generation into private scratch and compare them to
   the independently admitted credential tuple before any derivation;
4. derive `K_lab` only from the move-owned admitted credential under the exact
   CDDL domain and verify the kind-specific HMAC in constant time;
5. compute the kind-specific target-owned content digest over the exact
   envelope; and
6. expose only a privately constructible, move-only authenticated carrier with
   exact retained bytes/length, digest, authenticated tuple, and typed body.

The implementation must use the selected-base SHA-256 and the existing
`context_store_hmac_sha256` primitive. Its HMAC comparison is a fixed-length
aggregate comparison with no early exit, and qualification audits the compiled
operation-5 archive for the expected constant-time source shape. An optimizer
or binary audit that cannot establish this property blocks promotion; no
timing-security claim is inferred from functional tests alone.

The existing expectation-shaped registry-lab validator, encoders, admission
witnesses, or values constructed from untrusted record fields cannot establish
read admission. Encoders may be used only for supplemental fixture generation.
The independent inner predecessor and successor verifiers are reused with an
explicit `context_store_key_disposition::active` disposition. An authenticated
outer record never substitutes for inner envelope authentication.

## Semantic binder

After the marker authenticates, the binder admits only an initialized marker
whose fixed version/layout/capacity/limits, store/filesystem/mount/owner/mode,
lock identity, path policy, registry identity/epoch, and credential tuple equal
the independent preflight context. An authenticated initializing marker has
the special precedence below and deliberately short-circuits further content
decoding; no identity learned from it becomes authority outside that
classification.

For initialized state, HEAD must authenticate under the same root, path policy,
registry identity/epoch, and credential tuple. It selects exactly one occupied
immutable-envelope entry by its authenticated digest, exact declared length,
and exact bytes. That envelope must independently authenticate and bind the
HEAD high-water and the admitted registry scope/policy. Directory order,
maximum generation, filename alone, parsed-field equality, or digest-only
equality cannot select authority.

Every occupied journal record is independently decoded and bound to the same
root, path policy, key tuple, its physical fixed slot, exact attempt ID,
operation commitment, exact predecessor/successor bytes and registry-lab
digests, and exact authenticated record-chain digest. Every embedded
predecessor and successor is independently authenticated. The closed v1-to-v2
transition requires unchanged registry ID/epoch, authority-base scope,
registry policy, and key-continuity commitment; successor high-water exactly
predecessor high-water plus one without overflow; exact predecessor-envelope
digest; matching nonzero receipt fields; and a recomputed ADR-0014 operation
commitment.

A CLOSE must bind its exact PREPARE digest and successor HEAD digest. An ABORT
must bind its exact PREPARE digest and unchanged predecessor HEAD digest.
Terminal class and phase must be exact. A PREPARE cannot have both terminal
records. Terminal-without-PREPARE, multiple terminals, duplicated attempt IDs,
duplicated immutable-envelope digests, competing unresolved or CLOSE branches
claiming one successor, forks, chain contradictions, or a record whose physical
slot differs from its body is sticky invalid state.

The initialized marker's authenticated key 15 is the sole initial-chain anchor.
With zero CLOSE records, the current authenticated HEAD envelope digest must be
byte-exactly that marker initial-HEAD digest and HEAD must resolve the admitted
v1 predecessor. Every PREPARE in this frozen lane must bind that same marker
initial-HEAD digest in both key 15 (previous authenticated HEAD envelope digest)
and key 16 (exact predecessor HEAD-selector envelope digest). Every ABORT must
bind that same unchanged HEAD digest.

This first frozen lane admits at most one authenticated CLOSE in the complete
512-slot history. Its PREPARE must start at the marker initial-HEAD anchor under
the exact rule above. Its CLOSE successor-HEAD digest must equal the one current
authenticated HEAD envelope digest, and that HEAD must resolve the exact
authenticated successor bound by the PREPARE. Other occupied slots may contain
only fully authenticated PREPARE+ABORT pairs bound to the same initial anchor.
A one-CLOSE history is clean for recovery scanning, but a new v1-to-v2 request
against its already-v2 current state reaches `invalid_transition`; this lane
does not imply v2-to-v3 authority. Any different initial digest, predecessor-
selector digest, current selector, CLOSE anchor, or history requiring a general
multi-generation chain walk is sticky. Broadening these rules requires a new
ADR and tests before code changes.

A later unique attempt may request the same exact transition after one or more
fully authenticated terminal ABORT pairs, provided no immutable successor or
staging material already exists and every other request-admission rule passes.
Terminal ABORT proves only that its own attempt did not advance HEAD; it does
not globally reserve successor bytes. If the later attempt becomes unresolved,
it is the sole unresolved branch and may use the exact recovery rules above;
the older ABORT pairs remain non-authoritative history. A competing unresolved
attempt, any CLOSE already claiming that successor, duplicate attempt ID, or
pre-existing successor object remains fail-closed under the global precedence.

Unreachable but otherwise structurally valid immutable envelopes may remain
for later administrative accounting. They are still completely decoded,
authenticated, digest-checked, and duplicate-checked. They confer no authority.
Only exact request-successor material is considered by the request-level
pre-existing-material rule.

## Full-scan and recovery invariants

Except for the two deliberate early precedence cases below, operation 5 scans
all 512 slots, every occupied immutable-envelope entry, every unexpected-entry
slot, and all live and durable projections before classifying. It accumulates
fault flags rather than returning on the first malformed late entry. This
prevents a valid early slot from concealing corruption later in the fixed
layout.

Any successor or selector staging name is always ambiguous sticky state; it is
never downgraded to request-level pre-existing material. Any unexpected name
or object is sticky. A PREPARE, CLOSE, or ABORT namespace presence occupies its
physical slot even if its content is incomplete or malformed; the malformed-
state rule has higher precedence than request slot occupancy.

Exactly one unresolved PREPARE may be recoverable only after the rest of the
complete scan is valid. If current authenticated HEAD and its resolved envelope
are byte-exactly the prepared successor and no CLOSE already records that
successor, classify `needs_successor_close`. If they are byte-exactly the
prepared predecessor, classify `needs_predecessor_abort`. An existing CLOSE
plus an unresolved PREPARE for that same successor, any other HEAD position,
or more than one unresolved PREPARE is sticky.

## Single global classification precedence

The first matching condition wins:

1. Any live or durable `QUARANTINE` or `QUARANTINE.tmp` namespace presence,
   regardless of content -> `blocked_by_existing_quarantine`. Operation 5 does
   not decode, authenticate, or interpret quarantine content; presence is the
   complete read-only rule.
2. With no quarantine name, an authenticated and externally compatible marker
   in exact `initializing` phase -> `inadmissible_initialization_artifact`.
3. Any remaining layout, projection, authentication, compatibility, history,
   referent, staging, unexpected-entry, duplicate, or chain fault ->
   `needs_sticky_quarantine`.
4. Exactly one valid unresolved PREPARE at its successor ->
   `needs_successor_close`.
5. Exactly one valid unresolved PREPARE at its predecessor ->
   `needs_predecessor_abort`.
6. Request attempt ID already appears in any slot -> `attempt_replayed`.
7. All 512 physical slots are occupied -> `capacity_exhausted`.
8. The requested physical slot is occupied -> `requested_slot_occupied`.
9. The request's exact predecessor/current HEAD, successor, inner transition,
   digests, or recomputed operation commitment do not bind ->
   `invalid_transition`.
10. The exact requested successor immutable object already exists without the
    request's attributable PREPARE -> `preexisting_unattributed_material`.
11. Otherwise -> `continue_to_mutation`.

The deliberate initializing short-circuit avoids interpreting an incomplete
initialization layout. It is allowed only after marker authentication and full
external compatibility; malformed, incompatible, or unauthenticated marker
content falls to item 3. Existing quarantine presence is the only condition
that precedes it.

## Primitive/result mapping and trace

The immutable script grows to five entries in exact operation order. Operation
5 retains ADR-0020's 12 admitted primitive products. A confirmed `ok` must
carry exactly one oracle classification; all other operation-5 products carry
none. The engine independently derives the classification and compares it to
the oracle only after completing the scan. A confirmed-`ok` mismatch becomes
`invalid_request_no_mutation` after cleanup. Lost responses and process death
never expose or compare a classification.

Confirmed classifications map exactly:

- successor or predecessor recovery needed ->
  `uncertain_requires_recovery`;
- sticky or existing quarantine -> `quarantined_or_unavailable`;
- initializing or unattributed material ->
  `preexisting_material_no_authority`;
- replay -> `attempt_replayed_no_mutation`;
- capacity -> `capacity_exhausted_no_mutation`;
- requested slot occupied -> `slot_occupied_no_mutation`;
- invalid transition -> `invalid_transition_no_mutation`; and
- continue -> private event 201 only.

Confirmed `unsupported` maps to `unsupported_no_mutation`; confirmed
`unavailable` or `io_failure`, and every response-lost operation-5 read, map to
`quarantined_or_unavailable`. Process death has no ordinary result. Ordinary
traces are `1,2,3,4,5,90,91,92`; the clean boundary trace is
`1,2,3,4,5,201,90,91,92`. Trace capacity is nine. Cleanup, wipe ordering,
ownership, death, restart, no-throw, and allocation rules remain ADR-0021's.

## Required qualification before promotion

The focused suite must add, at minimum:

- every operation-5 admitted product and payload combination, including all
  eleven classifications, confirmed non-`ok`, response loss, and death;
- every truncation and every one-bit mutation of the operation-5 ROOT, HEAD,
  PREPARE, CLOSE, and ABORT golden objects, plus
  wrong domains, versions, key tuples, tags, field order/count, lengths,
  constants, phases, terminal classes, and trailing data;
- wrong modelled root, path policy, mount, owner/mode, lock, registry,
  scope/policy, credential disposition, predecessor, successor, continuity,
  high-water, receipt, slot, attempt, operation commitment, and referent;
- all 512 indices individually, a full valid 512-slot image, capacity plus one,
  every replay position, duplicate digests/attempts, and the 55 pairwise
  precedence combinations;
- same-transition retry after one and many terminal ABORT pairs, followed by
  clean classification, unresolved-predecessor recovery, unresolved-successor
  recovery, and CLOSE; plus rejection when a competing unresolved branch,
  CLOSE, or pre-existing successor object claims the retry;
- live/durable disagreement, snapshot immutability, missing/incomplete files,
  every staging name, every unexpected-entry position, zero/multiple
  unresolved attempts, both exact recoverable states, and every ambiguous
  recovery boundary;
- authenticated marker initial-HEAD digest mismatch; zero-CLOSE current HEAD
  mismatch; wrong PREPARE keys 15 or 16; wrong ABORT unchanged-HEAD digest; and
  one-CLOSE predecessor-anchor, successor-HEAD, or resolved-successor mismatch;
- no dynamic allocation after fixture construction, `noexcept` closure,
  credential/scratch wipe, secret-exclusion from snapshots/restarts/results,
  deterministic restart, and constant-time comparison source/binary audit;
- Windows Release and Debug, Linux ASan/UBSan, the independent archive/link
  graph audit, feature-off contract, full HaloFPX suite, and inherited focused
  regressions; and
- repeated large matrices with exact executable/source hashes, commands,
  environment, raw logs, counts, duration, and an independently reviewed
  machine-readable receipt.

The golden vector is required test input, not a runtime dependency. The
implementation must not add a new target or dependency edge. Promotion requires
an independent milestone review after tests in addition to the independent ADR
review before implementation.

## Explicit non-claims

This decision does not authorize initialization, operation 6 or later,
recovery execution, quarantine creation, compare-and-swap, reserve transition,
Linux I/O or locks, persistent writes, concrete authority, runtime or cache
linkage, production credential custody, multi-generation journal history,
durability, cache hits, inference behavior, or performance claims.
