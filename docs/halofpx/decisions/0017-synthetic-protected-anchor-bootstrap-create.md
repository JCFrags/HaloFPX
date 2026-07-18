# ADR-0017: synthetic protected-anchor bootstrap create and reconciliation

- Status: accepted for the disabled offline L05n seam after independent adversarial review
- Date: 2026-07-18

## Decision

L05n will model the final bootstrap transition without making a storage claim.
It will be an excluded, synchronous, backend-injected, permanently synthetic
seam with no I/O, path, filesystem, credential, server, provider, cache-
admission, production-target, or node API.

The create coordinator accepts only an rvalue L05m
`context_store_bootstrap_material_synthetic_proof`. Its first action, before
validation, allocation, or backend access, is to move the proof into a private
holder and invalidate the caller object. Every return path burns the proof.
Default, moved-from, incomplete, differently bound, or non-L05m state rejects.
The request adds only one fresh nonzero 256-bit anchor-attempt identity.

Every L05n public status, witness, handle, result, and proof type includes
`synthetic` in its name. They have no serialization, conversion operator,
common success base, or factory usable by a concrete seam. Every non-synthetic
anchor implementation must reject L05m and L05n types permanently.

## Root and policy binding

One noncopyable backend is constructed atomically for fixed nonzero protected-
anchor, material, and protected-registry root identities. The roots are
pairwise distinct. The backend owns all attempt histories, current synthetic
anchor state, unresolved operation state, and sticky anchor-root quarantine
for its lifetime.

Its policy fixes store UUID, namespace, checkpoint lineage, policy epoch,
manifest-key generation, writer-authority epoch, anchor-authentication key ID
and generation, a registered-ASCII anchor-durability policy identifier, and
the existing 1 KiB anchor-envelope limit. The moved L05m proof's roots,
manifest, selected digest, material policy, and exact proposed anchor must
match. The anchor must be canonical generation one with null predecessor.
L05n accepts no key and performs no new HMAC; it uses exact carrier identity
from the earlier authenticated decision.

The anchor-root policy commitment is:

```text
SHA-256(
  "halofpx.bootstrap-anchor-root-policy.v1\0" ||
  anchor root[32] || material root[32] || registry root[32] ||
  store UUID[16] || namespace ID[32] || lineage ID[32] ||
  uint64be(policy epoch) || uint64be(manifest-key generation) ||
  uint64be(writer-authority epoch) ||
  uint64be(anchor-key ID length) || exact anchor-key ID bytes ||
  uint64be(anchor-key generation) ||
  uint64be(durability-policy ID length) || exact policy ID bytes ||
  uint64be(maximum anchor-envelope bytes))
```

Registered IDs have no normalization or case folding. Any changed field is a
different authority domain and rejects before backend entry.

## Source and operation identities

The material-source commitment is derived only from the complete owned L05m
proof. Its fixed no-NUL outcome tag is `material-prepared-v1` or
`material-already-same-v1`, matching the proof's positive outcome.

```text
SHA-256(
  "halofpx.bootstrap-anchor-material-source.v1\0" ||
  uint64be(outcome-tag length) || exact outcome-tag bytes ||
  material root[32] || registry root[32] || material attempt[32] ||
  material root-policy commitment[32] ||
  authority-source commitment[32] || source-set commitment[32] ||
  material-set commitment[32] || material operation commitment[32] ||
  selected-manifest digest[32] ||
  uint64be(manifest length) || exact manifest envelope ||
  uint64be(observed-frame count) ||
  for each frame in manifest order:
    uint64be(index) || uint64be(frame length) || exact frame bytes ||
  uint64be(proposed-anchor length) || exact proposed-anchor envelope ||
  material durable-close confirmation[32])
```

Before backend entry, the coordinator revalidates all roots and policy fields,
the exact manifest digest and bytes, every L04c-equivalent frame against the
manifest-derived descriptors, the L05m close confirmation, and every proposed-
anchor byte. Digest or parsed-field equality never replaces full bytes.

The create operation commitment is:

```text
SHA-256(
  "halofpx.bootstrap-anchor-create.v1\0" ||
  anchor root[32] || material root[32] || registry root[32] ||
  anchor attempt[32] || anchor-root policy commitment[32] ||
  material-source commitment[32] || selected-manifest digest[32] ||
  uint64be(proposed-anchor length) || exact proposed-anchor envelope)
```

The coordinator computes both commitments from owned inputs before backend
entry. The base wrapper independently recomputes both before invoking the
derived primitive. No caller or response supplies either commitment.

## Atomic create operation

The only create primitive is private and coordinator-only:
`inspect_create_if_absent_synchronize_and_durable_close`. One anchor-root-
serialized call must:

1. register the exact attempt, roots, policy, source, proposed anchor, and
   operation commitment;
2. inspect authoritative current state without exporting absence;
3. on conclusive absence only, atomically create the exact anchor if absent,
   without replacement;
4. on any present state, including exact proposed bytes, perform no create and
   terminate without a proof or retry authority;
5. safely read back and byte-compare the complete current anchor;
6. perform every modeled anchor and protected-namespace synchronization; and
7. durably close the exact attempt before a positive response.

The only positive backend claim is `created_backend_claim`. Exact proposed
bytes observed before this attempt's create linearization return
`already_present_no_create`; they do not prove that this consumed attempt
created or synchronized the anchor and expose no proof. Different, malformed,
corrupt, unsupported, differently scoped, non-generation-one, or non-null-
predecessor present bytes are `anchor_conflict`. Unreadable, incomplete, or
non-authoritative observation is uncertainty, never absence.

The state machine is:

```text
unused -> active_pre_inspection
  -> conclusive_absence -> create_linearized
  -> exact_same_current -> definitely_rejected_no_create
  -> other_present -> definitely_rejected
  -> unreadable_or_ambiguous -> uncertain_terminal
create_linearized
  -> exact_readback -> synchronized -> durable_closed -> proof
  -> uncertain_terminal
active_pre_inspection -> definitely_aborted
```

Original attempt IDs are terminal after registration. The backend retains
exactly 512 IDs without eviction, reuse, wraparound, or replacement. Capacity
plus one returns `history_exhausted` before backend mutation, but after the
coordinator has burned the source proof. Replay remains rejected before and
after capacity. One root admits one create operation at a time.

A rejection is definite only after the exact operation is joined or fenced and
the backend proves that no create or late mutation can occur. Exception,
timeout, unconfirmed fence, ambiguous create, failed post-create readback or
synchronization, unconfirmed close, malformed response, allocation failure in
or after backend entry, or late completion is `visibility_uncertain`. It
sticky-quarantines the root and returns one opaque move-only synthetic
uncertain handle, but no success proof. Pre-backend allocation failure is
synthetic resource exhaustion, makes no backend call, and still burns the
source proof.

Before invoking the derived primitive, the base wrapper moves the consumed
L05m proof and exact operation into a private pending slot. It retains them
through callback return, complete witness validation, and allocation-free
positive finalization. Only a final private `claim_positive` transition may
move the pending proof into the returned proof. Uncertainty, exception,
malformed positive evidence, or any post-positive validation failure retains
the pending proof and exact operation for reconciliation. A conclusive
non-application or present-state rejection destroys the pending proof and
terminalizes the attempt.

The uncertain handle identifies the exact backend-owned pending operation but
does not own or expose its proof. It exposes only const nonsecret
reconciliation inputs and cannot be copied, serialized, converted, or used
with another backend. The handle alone cannot manufacture reconciliation
authority. Its representation is fixed-size and allocation-free so uncertainty
after backend entry can always return the single reconciliation authority.

The base wrapper also retains one monotonic original phase: `1` proves this
attempt did not reach create linearization, `2` proves this attempt reached
create linearization, and `3` is unknown. The phase is advanced only by a
private base-wrapper transition inside the sole atomic operation; a derived
response cannot assert it independently. Once phase `2` is recorded it cannot
return to `1`. Any contradictory or missing phase evidence becomes unknown and
remains uncertainty.

## Positive witness and proof

A positive move-only response echoes all roots, attempt, root-policy, material-
source, create-operation, selected-manifest digest, and outcome. It owns the
complete exact anchor readback and a 32-byte close confirmation.

```text
anchor_witness_digest = SHA-256(
  "halofpx.protected-anchor-head-witness.v1\0" ||
  uint64be(anchor length) || exact anchor bytes)

create_close = SHA-256(
  "halofpx.bootstrap-anchor-durable-close.v1\0" ||
  anchor root[32] || material root[32] || registry root[32] ||
  anchor attempt[32] || create operation commitment[32] ||
  material-source commitment[32] || proposed-anchor digest[32] ||
  anchor witness digest[32] || uint8(1))
```

The wrapper independently recomputes both, requires `1..1024` readback bytes,
and compares the proposed digest, length, and every byte. Malformed positive
evidence is uncertainty.

The positive synthetic proof is opaque, move-only, and source-invalidating. It
owns the consumed L05m proof, fixed policy, all roots and attempts, all L05n
commitments, exact manifest/object/proposed-anchor/readback witnesses, both
milestones' close confirmations, and classified outcome. It owns no key, path,
file handle, credential, reusable absence, backend authority, cache authority,
or runtime permission.

## Reconciliation

Only the same backend that returned an uncertain handle may reconcile it. The
coordinator moves and invalidates the handle before validation or allocation.
The backend must retain the exact unresolved operation. Never-invoked,
definitely rejected, proven, replayed, differently bound, or reconciled state
cannot enter. The reconciliation request adds only a fresh nonzero 256-bit ID.

```text
reconciliation_commitment = SHA-256(
  "halofpx.bootstrap-anchor-reconciliation.v1\0" ||
  anchor root[32] || material root[32] || registry root[32] ||
  reconciliation attempt[32] || original anchor attempt[32] ||
  original create operation commitment[32] ||
  uint8(original phase) ||
  anchor-root policy commitment[32] || material-source commitment[32] ||
  uint64be(proposed-anchor length) || exact proposed-anchor envelope)
```

The private primitive
`fence_original_observe_current_synchronize_and_durable_close` atomically joins
or fences the original before observing current state. A read before fencing
is forbidden. It performs no create, retry, replace, delete, or fallback.

Every definite response echoes all roots, original/reconciliation attempts and
commitments, root policy, and material source. It owns exact current bytes:
empty for conclusive absence, `1..1024` bytes for present state. Closed
classification codes are `1` exact proposed, `2` absent, and `3` other present.
The response also echoes the base-wrapper-owned original phase; the wrapper
requires it to equal the retained value and never accepts a response-supplied
upgrade from phase `1` or `3` to phase `2`.

```text
fence_confirmation = SHA-256(
  "halofpx.bootstrap-anchor-reconciliation-fence.v1\0" ||
  anchor root[32] || reconciliation attempt[32] ||
  reconciliation commitment[32] || original anchor attempt[32] ||
  original create operation commitment[32] || uint8(original phase) ||
  anchor witness digest[32] ||
  uint8(classification) || uint8(1))

reconciliation_close = SHA-256(
  "halofpx.bootstrap-anchor-reconciliation-durable-close.v1\0" ||
  anchor root[32] || reconciliation attempt[32] ||
  reconciliation commitment[32] || original anchor attempt[32] ||
  original create operation commitment[32] || uint8(original phase) ||
  material-source commitment[32] || proposed-anchor digest[32] ||
  anchor witness digest[32] || fence confirmation[32] || uint8(1))
```

Exact proposed bytes with retained phase `2` require full readback, every
synchronization, and the reconciliation close, then return
`created_same_recovered_backend_claim` with a move-only recovered synthetic
proof. Exact proposed bytes with phase `1` return
`already_present_fenced_no_retry` and no proof. Phase `3` never yields a proof.
Conclusive absence with phase `1` returns
`definitely_not_created_fenced_no_retry`; absence with phase `2` or `3` is
contradictory uncertainty. Other authoritative bytes return `anchor_conflict`
with no proof or retry authority. Unreadable,
malformed, contradictory, unconfirmed-fence, exception, timeout,
synchronization, close, or late state remains uncertainty and quarantine.

Reconciliation is exactly one-shot for one unresolved create. The first fresh
nonzero reconciliation ID is retained terminally; no replacement handle or
retry authority is issued if reconciliation remains uncertain. Concurrent
fresh IDs produce at most one primitive call, and every loser is stale. L05n
exposes no general quarantine-clear API. Reconciliation is the sole operation
admitted while quarantined, only for the exact backend-retained uncertain
operation, and never clears the sticky quarantine bit. The recovered proof adds the original uncertain phase, reconciliation
identity and commitment, exact observed anchor, fence and close confirmations,
and recovered outcome to the direct proof-equivalent material.

## Required qualification

Implementation requires:

- independent standard-library and separate C++ recomputation of every defined
  commitment, with every domain, NUL, tag, length, root, policy, epoch, key,
  material binding, manifest/frame/anchor byte, attempt, outcome, classification,
  and close code mutated;
- prepared/already-same L05m sources over direct and reconciled authority, plus
  default, moved, replayed, incomplete, and differently bound sources;
- source/handle invalidation on every result, including allocation, capacity,
  exception, malformed-positive, and post-positive failures;
- absence, exact pre-existing current with no proof, other valid current, wrong generation/predecessor/
  scope/key, unsupported, empty, oversized, truncated, noncanonical, corrupt,
  unreadable, contradictory, and late observations;
- before/after faults at registration, inspect, create linearization, readback,
  anchor sync, namespace sync, close, and terminal recording;
- lost-response reconciliation before/after create, including phase-attributed
  exact-present recovery, exact pre-existing no-proof, absent, other,
  malformed, unreadable, unconfirmed-fence, sync, close, exception, timeout,
  and late cases; presence alone never proves durability;
- root serialization, replay, exact original capacity, capacity plus one,
  last-slot races, one-shot two-fresh reconciliation attempts with at most one
  backend call and no replacement handle, re-entrant status observation without held locks, and
  sticky quarantine; and
- static exclusion of filesystem and concrete storage APIs, server/provider/
  hit/product linkage, synthetic serialization/conversion, donor code, and node
  access, followed by feature-off and inherited regression controls and
  independent adversarial review.

## Limits and next prerequisites

All authoritative, absence, synchronization, and close outcomes are synthetic
backend claims. L05n proves no filesystem, service, TPM, key custody, process,
restart, cross-process, rollback, controller, device, or power-loss behavior.
It cannot satisfy M63-01..03 or enable persistent writes.

After L05n, concrete work requires separate nonconvertible types in this order:

1. accept a concrete protected-state substrate decision covering root
   authority, key custody, persistent attempt/quarantine state, cross-process
   single writer, exact-envelope CAS, synchronization, recovery, and rollback;
2. qualify a disposable default-off protected-registry backend;
3. qualify a concrete bounded source and bootstrap-material writer;
4. qualify protected-anchor inspect/create/reconcile; and
5. only after the remaining gates, consider server/provider or canary linkage.

This preserves consume authorization, durable material, then anchor creation.
No L05m or L05n value is ever upgraded into concrete evidence.
