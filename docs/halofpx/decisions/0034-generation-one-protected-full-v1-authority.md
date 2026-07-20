# ADR-0034: generation-one protected full-v1 publication authority

Status: accepted only for the Linux-only, default-excluded L08h-b authority.
This decision does not authorize a server edge, live llama-state restore,
product admission, shared scope, generation advancement, or persistent user
data.

L08h-b composes the authenticated L08h-a attempt wire, L08f immutable snapshot
materializer, L08e exact filesystem reader, and the protected anchor carrier.
The authority accepts only one coherent generation-one/null-predecessor domain:
store UUID, private namespace, checkpoint lineage, policy epoch, manifest-key
generation, generation, and selected manifest digest must agree across the
anchor, replay policy, and admitted manifest.

Data and anchor roots must be distinct, non-nested, pre-created 0700 Linux
directories on supported filesystems. Construction duplicates and validates
both descriptors, acquires the fixed anchor-root `writer.lock` with an OFD
write lock, copies required key material, and reconciles before returning.
Root identity is revalidated at reconciliation, publication, terminalization,
pending cleanup, and lookup boundaries. One controller thread owns an instance;
in-process serialization is required before any server edge.

Publication is fixed and generation-one: authenticate and size-bound the full
source, durably publish authenticated `pending.v1`, materialize immutable data,
create the exact protected `anchor.v1` with no-replace semantics, verify the
materialized hit, durably publish authenticated `terminal.v1`, and only then
remove and directory-sync `pending.v1`. Restart reconciliation accepts only an
authenticated exact success, a conclusive pre-anchor abort, or a fail-closed
quarantine. Corrupt, missing, incompatible, incoherent, or uncertain state
never becomes a hit.

The target remains `STATIC EXCLUDE_FROM_ALL`. It has no server or live-restore
link, uses no donor code or new dependency, and leaves product registries empty.
Rollback is one revert plus removal of disposable test roots. A default-off
server canary is the next separately reviewed boundary.
