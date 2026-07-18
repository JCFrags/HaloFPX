# ADR-0014: bootstrap-authorization consumption transition

- Status: accepted for the disabled offline L05k coordinator
- Date: 2026-07-18

## Decision

Bootstrap authorization must be durably consumed before any future protected-
anchor create-if-absent operation. L05k models that ordering with an excluded,
synchronous, memory-only coordinator and injected compare-and-advance backend.
It performs no bootstrap execution or I/O and makes no real durability claim.

A sequence-only transition from `H` to `H + 1` is insufficient. If the atomic
operation applied but its response was lost, sequence equality cannot prove
which command advanced the registry. The authenticated successor therefore uses
a new closed wire version and carries both the exact predecessor snapshot digest
and a last-consumption receipt.

The successor body fixes format major 2/minor 0, bootstrap-authorization kind,
the unchanged registry ID and epoch, unchanged public authority-base-scope and
opaque policy commitments, the new high-water, exact predecessor envelope
digest, and a receipt containing:

- authorization sequence;
- 256-bit command identity;
- exact bootstrap-token envelope digest;
- bootstrap-plan commitment;
- selected-manifest digest; and
- proposed generation-one anchor-envelope digest.

Receipt sequence and body high-water are equal to the authorized plan sequence,
which is exactly predecessor `H + 1`. The predecessor and successor registry
key tuple and a separate stable key-continuity commitment are unchanged.
Registry identity/epoch, base scope, policy, or key change is an administrative
transition and rejects here. The exact authenticated predecessor and successor
envelopes are the compare-and-advance identities.

The frozen successor domains, each followed by exactly one NUL byte, are:

- `halofpx.registry-successor-key.v1`
- `halofpx.registry-successor-auth.v1`
- `halofpx.registry-successor.v1`
- `halofpx.registry-key-continuity.v1`
- `halofpx.bootstrap-consumption-operation.v1`

The existing registry master key may authenticate the successor only through
the new purpose-separated successor KDF and authentication domains. Both
carriers additionally own the same stable cross-version continuity commitment:

```text
HMAC-SHA-256(
  registry master key,
  "halofpx.registry-key-continuity.v1\0" ||
  deterministic-CBOR tstr(key ID) || deterministic-CBOR uint(key generation))
```

The successor carrier owns its exact bounded envelope, body, digest, key tuple,
stable key-continuity commitment, and its version-specific private authority
binding. L05j's predecessor carrier is extended to own its exact envelope and
stable continuity commitment without changing its v1 wire. Envelope-specific
authority bindings are not compared across versions.

## Operation identity and root ownership

Each protected-registry backend is constructed for one nonzero 256-bit root
identity derived by the operator from the exact protected-registry authority
domain. The backend, not a coordinator instance, owns the single attempt/fence
registry for that root for its entire lifetime. All coordinators for the root
must share that backend. Attempt IDs are terminal after first registration and
are never reusable. A fresh attempt ID may reconcile the same stable command.

The coordinator constructs one public operation commitment as:

```text
SHA-256(
  "halofpx.bootstrap-consumption-operation.v1\0" ||
  root identity[32] || consumption attempt ID[32] ||
  uint64be(predecessor envelope length) || predecessor exact envelope ||
  uint64be(successor envelope length) || successor exact envelope ||
  uint64be(authorization sequence) || command ID[32] ||
  token envelope digest[32] || plan commitment[32] ||
  authority snapshot commitment[32] || selected manifest digest[32] ||
  proposed anchor envelope digest[32])
```

The coordinator owns this commitment in its value operation and passes the
complete immutable operation synchronously to the backend. The backend binds
its attempt registry and exact-envelope compare-and-advance to every listed
input and independently recomputes the commitment before a positive result.
No pointer value, secret, or caller-supplied digest substitutes for these exact
owned values.

## Linearization and ambiguity

Every invocation carries a fresh nonzero 256-bit consumption-attempt identity.
It is fencing identity, not authorization. The stable idempotency identity is
the authenticated command receipt.

Only a backend result proving the exact successor durably current returns an
opaque move-only `advanced_unexecuted` proof. Both positive backend outcomes
must return an owned authenticated current-state witness; the coordinator-owned
wrapper compares its digest and every exact envelope byte with the proposed
successor before admitting the result. A qualified same-command retry may return
`already_consumed_same_unexecuted` only when that witness is the entire exact
successor envelope and receipt; high-water equality alone never suffices. The
proof owns no key and authorizes no I/O by itself.

A definite stale, conflict, replayed attempt, or definitely-not-applied result
returns no proof. An uncertain begin, compare-and-advance, durable close,
timeout, exception, malformed response, or late-completion risk makes the
backend-owned root registry sticky-quarantined for the rest of that backend's
lifetime and returns no proof. No later result may downgrade uncertainty to a
definite result, and constructing another coordinator cannot bypass the fence.
L05k cannot clear quarantine. A later separately qualified reconciler must
prove exact predecessor-current, exact same-command successor-current, a
conflicting successor, or continued ambiguity.

Only the coordinator can invoke the backend wrapper or construct the opaque
move-only proof. It owns the complete authenticated predecessor and successor
carriers, receipt, consumption-attempt ID, root identity, classified confirmed
backend outcome, operation commitment, command and sequence, token digest, plan
commitment, authority snapshot commitment, manifest digest, and exact proposed
anchor carrier. Those fields are readable only through const accessors so a
future executor can independently recompute and verify the operation identity.
Moving the proof invalidates its source. A future bootstrap executor must accept
only this proof and require its exact plan/anchor bindings before
create-if-absent; L05k itself authorizes no I/O.

Consumption before create-if-absent can safely burn a command if no later
bootstrap occurs. That availability cost is accepted; reversing the order would
permit replay after a crash.

## Execution boundary

L05k proves only an offline transition contract. It adds no concrete protected
registry, durable journal, cross-process CAS, filesystem atomicity, restart
reconciler, credential provider, operator identity, policy evaluation,
conclusive anchor absence, create-if-absent, bootstrap execution, server/provider
linkage, persistent cache write, restore, hit, deployment, or node behavior. It
does not resist rollback of both the protected head and its keys.

The current L05j authority accepts only the frozen v1 predecessor wire. L05k
therefore cannot plan or consume a different `H + 2` command from the v2
successor. It models only the original command's confirmed advancement and
exact same-command retry/reconciliation. Successor-aware replanning is a later
separately reviewed format/admission milestone.
