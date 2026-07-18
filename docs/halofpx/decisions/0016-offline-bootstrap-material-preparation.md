# ADR-0016: offline bootstrap-material preparation seam

- Status: accepted for the disabled offline L05m seam after independent adversarial review
- Date: 2026-07-18

## Decision

L05m freezes the semantic boundary between a consumed bootstrap authorization
and any future protected-anchor operation. It is an excluded, synchronous,
backend-injected contract only. It may inspect the already authenticated anchor
carrier owned by the source proof, but must not call a protected-anchor storage
backend or inspect, create, replace, or synchronize protected-anchor state. It
must not expose a path, file, directory, server, or provider API.

The only authority accepted by the preparation coordinator is one source-
invalidating rvalue proof from an already accepted transition:

1. an L05k `advanced_unexecuted` proof;
2. an L05k `already_consumed_same_unexecuted` proof; or
3. an L05l `consumed_same_recovered_unexecuted` proof.

These are closed provenance tags. A caller cannot supply a tag, operation
commitment, selected-manifest digest, object count, object descriptor, root
binding, or positive outcome. Default, moved-from, incomplete, rejected,
definitely-unconsumed, or conflicted source state cannot enter preparation. As
the first action in either rvalue overload, before validating the proof,
request, manifest pointer, lengths, allocation, roots, or backend state, the
coordinator moves the supplied proof into a private holder and invalidates the
caller object. Every return path therefore burns the supplied authority. A
backend or allocation error must not return reusable authority to the caller.

## Separate roots and policy binding

The protected-registry root and material-publication root are distinct 256-bit
identities with distinct authority domains. Equality is invalid configuration;
neither identity may be all zero. One noncopyable preparation backend is
constructed atomically for one fixed material root, one distinct admitted
registry root, one fixed material policy, and one fixed exact synthetic object
source set. A source proof is admissible only when its registry root equals the
backend's admitted registry root exactly. The material policy fixes:

- store UUID;
- namespace ID;
- checkpoint lineage ID;
- policy epoch;
- manifest-key generation;
- writer-authority epoch; and
- one closed durability-policy identifier;
- exact maximum source-object count in `1..128`;
- exact maximum frame bytes in `1..16,777,216`; and
- exact maximum aggregate frame bytes in `1..67,108,864`.

The exact authenticated proposed anchor and exact digest-identified manifest
must agree with that scope. Generation is exactly one, the anchor predecessor
is null, and the manifest predecessor is absent. The manifest's exact numeric
durability mode must equal the backend's closed durability policy. Request data
cannot select or weaken any root, epoch, scope, writer, or durability value.

The material-root policy commitment is:

```text
SHA-256(
  "halofpx.bootstrap-material-root-policy.v1\0" ||
  material root identity[32] || registry root identity[32] ||
  store UUID[16] || namespace ID[32] || checkpoint lineage ID[32] ||
  uint64be(policy epoch) || uint64be(manifest-key generation) ||
  uint64be(writer-authority epoch) ||
  uint64be(durability-policy identifier length) ||
  exact durability-policy identifier bytes ||
  uint8(exact manifest durability mode) ||
  uint64be(maximum source-object count) ||
  uint64be(maximum frame bytes) ||
  uint64be(maximum aggregate frame bytes) ||
  source-set commitment[32])
```

The durability-policy identifier is registered ASCII with no normalization or
case folding and is bounded by the project registered-ID limit. A future
change in any listed value is a different material authority domain. Object
count must not exceed the committed maximum. Each frame length must not exceed
the committed per-frame maximum. The coordinator computes the aggregate with
checked unsigned addition, rejecting before wraparound or when the committed
aggregate maximum would be exceeded.

## Exact manifest and material-set ownership

In addition to the authority proof, a request supplies only a fresh nonzero
256-bit material-attempt identity and borrowed exact manifest-envelope bytes.
The coordinator retains neither borrowed pointer nor caller storage. Before a
backend call it:

1. enforces the existing 1 MiB manifest bound;
2. computes the target-owned domain-separated digest over the complete exact
   envelope;
3. requires it to equal both the source proof's selected-manifest digest and
   the exact proposed anchor's selected-manifest digest;
4. performs the closed canonical v1 structural parse; and
5. requires every exact scope, generation, predecessor, key-generation, and
   durability binding described above.

The source proof's selected digest was derived from the L05h authenticated
manifest result. Exact digest equality gives digest-transitive identity with
the exact envelope covered by that earlier decision. L05m performs no fresh
HMAC verification and must not describe this comparison as new authentication.
It accepts no key or caller-constructed authentication result. The operation
and any positive synthetic proof own a heap-bounded copy of the complete exact
envelope, including the authentication tag bytes previously covered by that
digest. They derive the complete ordered object descriptor set only from those
bytes. No caller-supplied count or descriptor is trusted.

Each descriptor is its manifest index, 256-bit object ID, exact registered
stream-type bytes and length, and unsigned 64-bit frame length. The material-
set commitment is:

```text
SHA-256(
  "halofpx.bootstrap-material-set.v1\0" ||
  uint64be(manifest envelope length) || exact manifest envelope ||
  uint64be(object count) ||
  for each descriptor in manifest order:
    uint64be(index) || object ID[32] ||
    uint64be(stream-type length) || exact stream-type bytes ||
    uint64be(frame length))
```

L05k and L05l do not carry object payloads. The noncopyable synthetic backend
base wrapper constructor therefore takes ownership of one complete heap-bounded
in-memory source set and publishes the backend as valid only after atomically
validating and fixing that entire set. It cannot add, replace, remove, reorder,
or mutate a source during its lifetime. The coordinator has private friend-only
const access to these owned bytes for pre-call commitment computation. A
derived preparation primitive may read them only through protected const
index/length accessors; it cannot replace them. The set contains exactly one
complete immutable frame for every manifest object and no extra frame. Its
commitment is:

```text
SHA-256(
  "halofpx.bootstrap-material-source-set.v1\0" ||
  uint64be(source-object count) ||
  for each source in fixed manifest order:
    uint64be(index) || uint64be(frame length) || exact complete frame bytes)
```

Construction enforces source-set ownership and the committed object, per-frame,
and aggregate byte limits with checked addition, but it has no request manifest
and therefore makes no descriptor-relative claim. After parsing the exact
request manifest and before backend entry, the coordinator requires exactly one
source per manifest object in manifest order and runs the complete L04c-
equivalent structural verifier against each manifest-derived descriptor:
exact magic and `halofpx.object.v1` domain, bounded type length, byte-exact
stream type, checked payload length, complete payload with neither truncation
nor trailing bytes, and the whole-frame SHA-256 object identity. A source
cannot be selected by a caller path. A descriptor, filename, digest claim,
parsed payload, or backend-generated placeholder is not an object source.
Admitting a concrete capture, staging, file, stream, shared-memory, or network
source requires a new non-synthetic type and separate implementation and
provenance gate. The base backend wrapper repeats the exact source-count,
ordering, limit, and L04c-equivalent matching during execution before invoking
the derived primitive.

## Source and operation identities

The source commitment uses exactly one closed ASCII tag with no trailing NUL:
the 18-byte `direct-advanced-v1`, the 22-byte
`direct-already-same-v1`, or the 23-byte `reconciled-successor-v1`. The tag
completely fixes the admitted source outcome; no redundant enum ordinal or
caller-supplied outcome scalar enters the commitment. It binds the exact
registry root, original
consumption attempt and operation commitment, authenticated successor envelope,
authenticated proposed-anchor envelope, authorization sequence, command ID,
token digest, plan commitment, authority-snapshot commitment, selected-
manifest digest, and provenance tag. The reconciled form additionally
binds the reconciliation attempt, reconciliation commitment, and complete exact
observed-successor witness. Lengths precede every variable-length field.

```text
SHA-256(
  "halofpx.bootstrap-material-authority-source.v1\0" ||
  uint64be(provenance-tag length) || exact provenance-tag bytes ||
  registry root identity[32] || original consumption attempt[32] ||
  original consumption operation commitment[32] ||
  uint64be(successor envelope length) || exact successor envelope ||
  uint64be(proposed-anchor envelope length) || exact proposed anchor envelope ||
  uint64be(authorization sequence) || command ID[32] || token digest[32] ||
  plan commitment[32] || authority-snapshot commitment[32] ||
  selected-manifest digest[32] ||
  reconciled-only fields in their fixed order)
```

The preparation operation commitment is:

```text
SHA-256(
  "halofpx.bootstrap-material-preparation.v1\0" ||
  material root identity[32] || registry root identity[32] ||
  material attempt identity[32] || root-policy commitment[32] ||
  source commitment[32] || source-set commitment[32] ||
  material-set commitment[32] ||
  selected-manifest digest[32] || proposed-anchor envelope digest[32])
```

Before backend entry, the coordinator computes the source-set commitment from
the base wrapper's immutable constructor-owned frames and independently
computes the root-policy, authority-source, material-set, and operation
commitments from their exact owned inputs. At execution, the base backend
wrapper independently recomputes all five commitments, including the source-
set commitment directly from the same immutable frames, before invoking the
derived primitive. After a positive response, the coordinator recomputes the
source-set commitment again from the complete readback witness. No commitment
originates from the caller, derived primitive, or response alone. No pointer,
secret, path, parsed subset, cached metadata, or caller digest may substitute
for exact owned values.

## One atomic backend operation

The only preparation primitive is a private coordinator-only
`prepare_exact_material_set_and_durable_close` operation. It is one serialized,
material-root-scoped backend operation. It does not make all storage changes
transactionally disappear on failure; it makes attempt registration, ordered
execution, terminal fencing, and the positive durable close one indivisible
authority decision.

Its semantic order is ADR-0004 steps 1 through 6 only:

1. register the exact attempt, source, roots, policy, and operation identity;
2. create unique backend-owned staging objects without following links;
3. write bounded exact source bytes and re-run the complete L04c-equivalent
   frame verification, including exact magic, domain, type, payload length,
   completeness, no trailing bytes, and whole-frame SHA-256;
4. synchronize each complete object, publish it immutably without replacement,
   and synchronize its parent namespace;
5. write, re-read, and verify digest-transitive identity with the exact
   envelope covered by the prior L05h authentication decision, then synchronize
   the complete owned manifest; and
6. publish the manifest immutably without replacement, synchronize its parent
   namespace, and durably close the exact attempt.

Destination equality is not inferred from a name or metadata. An equal existing
object is reusable only after safe exact-frame readback, length, stream-type,
and digest verification followed by the required synchronization. An equal
existing manifest requires every exact byte to match and is synchronized again.
Unequal content at an equal identity is a collision and quarantine event. The
operation never enumerates material to choose a generation.

A future concrete backend must persist the attempt journal and enforce the
single writer per material root across processes and restarts. The L05m
synthetic backend owns only lifetime-scoped state and cannot establish those
claims.

## State machine, outcomes, and uncertainty

The abstract backend state machine is:

```text
unused
  -> active_pre_material
     -> objects_published_durable
        -> manifest_published_durable
           -> durable_closed -> positive proof
  -> definitely_aborted -> terminal, no proof
  -> uncertain_terminal -> sticky material-root quarantine, no proof
```

Material-attempt identities are terminal after registration. The synthetic
backend retains exactly 512 terminal identities without eviction. A fresh
attempt at capacity returns `history_exhausted` before mutation; no identity is
overwritten, wrapped, evicted, or made reusable, and replays remain rejected
before and after capacity. The backend also records the exact stable source
commitment so a different material set or policy cannot reuse consumed
authority. A definite rejection after registration is permitted only after the
backend has joined or fenced the exact attempt and proved that no late operation
can mutate the root. Unreachable exact material may remain after a pre-anchor
failure; it confers no eligibility and is not deleted by this flow.

The only positive outcomes are `prepared_backend_claim` and
`already_same_backend_claim`. The latter must repeat exact safe readback and all
required synchronization; prior presence or prior success is insufficient.
Definite nonpositive outcomes include invalid or replayed attempt, source or
policy conflict, writer busy, object collision, manifest collision, no space,
quota or reserve exhaustion, read-only storage, storage error, synchronization
error, and definitely aborted. They expose no proof.

Exception, interruption without a confirmed fence, timeout, malformed or
contradictory response, incomplete operation, unconfirmed close, uncertainty,
or late-completion risk is `visibility_uncertain`. It sticky-quarantines the
material root for the backend's lifetime and exposes no proof. L05m has no
quarantine-clear or reconciliation API. Registry-root quarantine and material-
root quarantine are separate and neither can clear the other.

## Exact positive witness and proof boundary

Every positive backend response echoes the material and registry roots,
material attempt, root-policy commitment, source commitment, material-set
commitment, source-set commitment, and operation commitment. The response is
move-only. Its heap-owned buffers are bounded by the committed count,
per-frame, aggregate-frame, and manifest limits. It owns:

- the complete exact frame read back for every object in manifest order, not a
  digest-only or metadata-only observation;
- the complete exact published-current manifest envelope after safe readback;
  and
- one fixed 32-byte durable-close confirmation.

The wrapper re-runs the complete L04c-equivalent verifier over each readback
frame, requiring exact magic and domain, bounded and byte-exact descriptor
stream type, checked payload length, complete payload without truncation or
trailing bytes, and whole-frame SHA-256 identity. It recomputes the source-set
commitment over all observed frames and requires exact manifest length, digest,
and every byte. Aggregate readback length uses checked addition and cannot
exceed the committed maximum.
The durable-close confirmation is exactly:

```text
SHA-256(
  "halofpx.bootstrap-material-durable-close.v1\0" ||
  material root identity[32] || registry root identity[32] ||
  material attempt identity[32] || operation commitment[32] ||
  source-set commitment[32] || material-set commitment[32] ||
  selected-manifest digest[32] || uint8(1))
```

The terminal code is fixed to unsigned value `1`; no other close scalar is
admitted. This bounded confirmation states only that the synthetic backend
reached its contract terminal after every required synchronization.

The wrapper rejects a missing, duplicate, reordered, truncated, or unexpected
frame; any changed confirmation; any object length or digest mismatch; any
manifest length, digest, or byte mismatch; aggregate overflow; or a positive
outcome without the complete witness. Digest equality never replaces complete
frame readback or the final exact manifest-byte comparison. Malformed positive
evidence is uncertainty, not a definite miss.

Success creates one opaque, move-only, source-invalidating **synthetic**
material proof. It
owns the consumed L05k or L05l provenance, both roots, fixed policy and epochs,
material attempt, every defined commitment, exact digest-identified manifest envelope,
complete ordered descriptors, complete exact frame readbacks, exact proposed
anchor, classified positive outcome, and durable-close confirmation. Const
accessors expose every nonsecret value needed for independent recomputation.
Moving the proof invalidates its source. It owns no key, registry key, source
handle, path, anchor absence, or reusable backend authority.

The proof may enter only a future excluded synthetic protected-anchor model
that explicitly accepts this exact synthetic type. It does not authorize
another material write,
anchor inspection or creation, cache admission, restore, hit, server/provider
integration, persistent write enablement, or node use.

Every public L05m status, result, witness, and proof type includes `synthetic`
in its type name. These types are permanently distinct
from any future concrete backend types. They provide no conversion operator,
common success enum, inheritance path, serialization, or factory usable by a
production anchor flow. Concrete qualification must introduce a separately
reviewed non-synthetic proof rather than relabel or reuse an L05m proof.

All heap allocation follows the consume-first rule. Failure while constructing
the operation before backend entry returns a synthetic resource-exhausted
status, makes no backend call, and still leaves the caller proof invalid. An
allocation exception in or after the backend primitive is visibility
uncertainty because mutation may have begun; it sticky-quarantines the material
root. Successful buffers move into the synthetic proof without copying.

## Required adversarial qualification

Implementation cannot be promoted without:

- an independent standard-library golden serializer and a separately written
  C++ recomputation, with mutations covering every domain, trailing NUL,
  provenance tag, length, order, root, epoch, policy, source, descriptor,
  manifest, and operation binding;
- direct-advanced, direct-already-same, and reconciled-successor cases plus
  default, moved, replayed, rejected, and differently bound source proofs;
- proof invalidation on every admitted success and failure path;
- empty, oversized, truncated, noncanonical, bit-corrupt, wrong-scope, wrong-
  generation, wrong-predecessor, wrong-key-generation, wrong-durability, wrong-
  digest, wrong-count, reordered, duplicate, missing, and unexpected manifest
  or descriptor evidence;
- source and readback frames with wrong magic, wrong object domain, malformed or
  oversized type, descriptor-type substitution, wrong or overflowing payload
  length, truncated payload, trailing bytes, or wrong whole-frame digest;
- changed confirmation and positive-witness mutations for every echoed field,
  complete frame, order, length, digest, close code, and manifest byte;
- before/after fault injection at every object and manifest stage, write,
  verification, synchronization, no-replace publication, and durable-close
  boundary;
- equal and unequal destination tests; no-space, quota, reserve, read-only,
  I/O, synchronization, interruption, exception, timeout, uncertainty, and
  late-completion tests;
- concurrent fresh attempts proving one root-serialized operation at a time,
  terminal replay behavior, exact 512-attempt capacity, capacity-plus-one
  fail-closed behavior, replay before and after capacity, a concurrent last-slot
  race, no lock held across re-entrant status observation, and sticky
  quarantine; and
- static exclusion checks forbidding filesystem, path, file, protected-anchor
  storage read/inspect/create/replace/synchronize calls, server, provider, and
  production-target linkage. Authenticated anchor-carrier types and const
  accessors required to verify source bindings remain allowed. Feature-off and
  inherited regression controls follow.

## Limits

L05m is a permanently synthetic, backend-qualified semantic claim. A positive fixture
response proves conformance of the wrapper and state machine to the injected
backend contract; it is not evidence that Windows, Linux, a device, filesystem,
controller, or power-loss boundary made bytes durable. The durable-close term
means only that the synthetic backend returned the exact terminal confirmation
required by this contract; the public success statuses remain backend claims.

L05m adds no concrete object capture or payload source, path policy, no-follow
primitive, filesystem implementation, file or directory synchronization,
same-filesystem proof, atomic no-replace primitive, persistent attempt journal,
cross-process writer lock, restart recovery, key custody, rollback resistance,
protected-anchor access, create-if-absent, anchor reconciliation, bootstrap
execution, cache admission, server/provider linkage, persistent feature,
deployment, node behavior, power-loss qualification, or performance claim.

Every non-synthetic anchor seam must reject this proof permanently. Even after
a concrete material backend passes source-binding review, persistent fencing,
cross-process coordination, platform-specific crash and power-loss
qualification, exact synchronization, rollback, recovery, and the remaining
ADR-0004 gates, it must issue a new non-convertible concrete proof type under a
separate accepted decision. L05m status or proof values are never upgraded,
reinterpreted, deserialized, or relabeled as production evidence.
