# ADR-0010: offline bootstrap-authority plan

- Status: accepted for the disabled offline L05g planner
- Date: 2026-07-18

## Decision

First-store bootstrap is an administrative operation distinct from ordinary
publication. L05g may only plan it in memory. It cannot create, replace, or
synchronize a protected anchor.

One noncopyable authority instance privately owns an immutable, bounded copy of
the exact configured scope and two distinct key-purpose records:

1. an anchor-authentication signing key; and
2. a bootstrap-administration authorization key.

Both records require exact registered ASCII ID bytes, nonzero generation,
active disposition, and nonempty bounded secret material. The two purposes are
not interchangeable. Configuration fixes the store UUID, namespace, policy
epoch, checkpoint lineage, manifest-key generation, and writer-authority epoch;
request data cannot choose or weaken them. Owned secrets are overwritten on
destruction and are never exposed by a public accessor or copied into a plan.

A valid request supplies only a nonzero 256-bit attempt ID, a selected manifest
digest, and an object count in `1..128`. The planner constructs and signs the
exact v1 anchor body with generation `1` and a null predecessor. Its opaque
plan binds the full attempt, object count, owned authenticated anchor envelope,
and a domain-separated bootstrap-authority commitment derived from the admin
key and immutable authority snapshot. The only positive status is
`authorized_unexecuted`; it conveys neither hit eligibility nor permission to
use the ordinary publication coordinator.

## Replay and manifest boundary

The planner is deliberately stateless. It proves exact plan binding but cannot
claim historical attempt-ID uniqueness, one-shot token consumption, registry
high-water protection, or crash-safe replay fencing. Those require a protected
persistent attempt registry and independently reviewed reconciliation.

The selected manifest digest is a trusted synthetic input at this milestone.
A future executor must instead require an authority-admitted authenticated
manifest proof. A raw request digest must never become durable publication
authority.

## Execution boundary

Execution must later establish conclusive protected-anchor absence and use an
exact create-if-absent primitive. Present, corrupt, unreadable, or ambiguous
anchor state must reject. Any ambiguous create, synchronization, or durable
close must fence or quarantine the root until qualified reconciliation.

L05g adds no registry file, credential provider, external token wire, path,
filesystem operation, backend method, server/provider linkage, persistent
write, cache admission, node use, or durability claim.
