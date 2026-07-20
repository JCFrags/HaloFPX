# ADR-0029: excluded authenticated full-v1 read composition

Status: accepted for the synthetic, memory-only L08d implementation boundary.
This does not admit a production persistent reader, codec, filesystem, writer,
or live-state restore.

## Context

ADR-0003 defines a closed authenticated full-v1 manifest and immutable object
frames. L04a through L04c parse, authenticate, and verify those units, but the
authenticated carrier previously retained only object ID, stream type, and
frame length. That was insufficient to decide whether a complete object set
matched an admitted profile, codec schema, logical boundary, or distributed
placement. L08a through L08c instead use the disposable `HFPXLD01` laboratory
format and must not become the trusted internal ABI or the basis for retention.

The accepted implementation order requires a bounded full-v1 read composition
before a filesystem reader or full-v1 writer. The owner also directed that this
step use focused tests and one representative target-node qualification rather
than an exhaustive storage/fault matrix.

## Decision

The structural parser retains every full-v1 decision fact required by a future
reader: codec ID and schema, required flag, token digest, logical position and
output boundary, rank ownership, compatibility root, topology plan schema and
execution mode, global plan digest, per-rank ownership and placement, state
profile, producer identity, and durability mode.

Only successful manifest authentication creates a private carrier containing
an owned sanitized copy of those facts. Authentication key IDs, key bytes, and
tags are absent. Failure results expose no metadata or object reference even if
their public status is later modified.

`halofpx-context-store-v1-read-only` is a `STATIC EXCLUDE_FROM_ALL` target. Its
factory receives one borrowed in-memory manifest, verification policy, trusted
closed admission record, ordered object frames, and explicit per-object and
aggregate byte limits. Construction deep-copies all inputs. The provider:

1. authenticates and replay-checks the exact manifest;
2. exact-matches scope, compatibility, lineage, and policy epoch;
3. exact-matches the complete trusted manifest metadata and object roster;
4. verifies every frame against its authenticated descriptor and explicit
   bounds; and
5. only after all objects succeed, returns one immutable candidate owning
   bounded opaque payload bytes.

Any failure returns a miss and no candidate. There is no partial object return.
The candidate cannot decode a payload, call a codec, or mutate a live context.
The provider advertises no production capabilities and always rejects publish
as disabled. Its internal synthetic `hit` exists only to prove that the L03
candidate seam can safely compose the authenticated full-v1 units.

The caller-supplied admission is a fixture allowlist, not product admission
authority; any explicit linker can construct one. The implementation has no
path, filesystem, discovery, enumeration, server, background work, logging,
metrics, donor, or live-state dependency. The owned manifest key receives a
best-effort wipe at normal provider destruction, and it is copied only after all
other allocating construction steps complete. Compiler, allocator, crash, and
process-remanence limits remain unresolved and block use of this seam for
protected live keys.

## Rejection mapping and limits

Unknown, revoked, read-disabled, or generation-mismatched keys are unauthorized
misses. Authority and replay disagreement are replay misses. Compatibility
disagreement is an incompatible miss. Malformed authentication or corrupt
frames are corrupt misses. Closed-profile, topology, codec, schema, roster, or
configured-limit disagreement is unsupported or incomplete. No rejection may
return candidate metadata or payload bytes.

Per-frame and per-payload caps remain trusted explicit inputs as required by
L04c. L08d adds a checked aggregate frame-byte cap before candidate allocation.
It does not invent production storage quotas or retention values.

## Consequences and closed gates

L08d proves all-or-nothing composition against deterministic synthetic fixtures
on Windows and one Linux Strix Halo node. It does not provide safe filesystem
open/streaming identity, real state codec admission, semantic construction of
the 16 compatibility components, generation advancement, protected full-v1
publication, retention/eviction, administration, shared scope, distributed
restore, payload zeroization, soak, or zero-regression evidence. Those remain
separate gates.

Rollback is one coherent revert. Because the target is excluded and no server
edge exists, feature-off behavior remains the selected-base control. No donor
implementation, GPL llama-ai code, CachyLLama transplant, WebUI asset, remote,
dependency, or persistent user data entered this decision.
