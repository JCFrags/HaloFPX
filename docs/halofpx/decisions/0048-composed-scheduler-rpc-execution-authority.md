# ADR-0048: composed scheduler and RPC execution authority

- Status: accepted design; implementation and qualification remain default-off
- Date: 2026-07-26
- Base: `0d655b54d77929dafc2a7efe05f25a94d6c6ca0d`
- Scope: L42 scheduler authority and L44 RPC mutable-session composition for
  one admitted execution

## Context

L46 proved that the accepted L42 and L44 public boundaries cannot be composed
honestly by the real llama replay caller. L44 requires authority for every
recursively discovered graph leaf before filtering by RPC storage, while its
registration and exclusion APIs accept only RPC-resident tensors. The
scheduler creates RPC copies during its private split. L42 is also a
single-enable/single-split contract whose final transcript exists only after
execution, while L44 needs prepared authority before RPC compute.

This decision corrects only that composition boundary. It does not change
cache state, RPC graph authority, state-object semantics, or production
enablement.

## Decision

### Mixed local/RPC census

Source call sites register closed structural roots and roles before prepare.
The scheduler owns locality and copy derivation but may not invent mutable,
weight, or state roles. Before split pass 5 rewrites any consumer source edge,
the scheduler assigns canonical IDs from ordered caller-graph nodes plus
recursive leaf postorder, retaining every null-source bit and view
edge/offset. The prepared snapshot then maps generated copies back to those
pre-mutation source IDs and records backend ordinal, storage class, allocation
epoch where materialized, and complete nested view root/edge/offset/range
authority.

Every leaf is classified exactly once:

- `RPC_MUTABLE`: structurally registered mutable material reachable by an
  admitted RPC split;
- `RPC_IMMUTABLE_WEIGHT` or `RPC_LOCAL_STATE_PAYLOAD`: structurally excluded
  RPC material;
- `LOCAL_MUTABLE`, `LOCAL_IMMUTABLE_WEIGHT`, or
  `LOCAL_STATE_PAYLOAD`: coordinator/scheduler-authenticated local material;
  or
- `UNKNOWN`, which refuses preparation.

L44 registration/exclusion is required only for `RPC_*` leaves and the exact
scheduler copies mapped into the admitted RPC split. Local leaves remain
explicit entries in the prepared scheduler census and its authenticated root;
they are never silently dropped or represented as RPC evidence. An
RPC-reachable leaf without closed authority, a false-local classification,
unknown ownership/locality, conflicting roles, or a missing copy mapping
refuses before compute.

Dynamic views inherit classification only from an explicitly classified
structural root. The snapshot authenticates every intervening view edge,
offset, logical range, type, dimensions, and strides with checked bounds and
overflow. A view that escapes its root, changes storage class, or has ambiguous
ancestry refuses. Loader call sites need classify structural roots, not every
derived view. A local mutable root copied into an RPC split retains the
source-owned role and ordinal on the exact generated RPC copy, with its derived
range and view authority; this is not a second inferred classification.

### Precompute scheduler bridge

After the actual scheduler split and allocation preparation, but before any
copy or backend compute, the caller obtains a bounded immutable prepared
snapshot. It contains:

- attempt nonce, scheduler handle generation, coordinator scheduler graph UID,
  and monotonic execution sequence;
- exact split ordinal, backend ordinal, node range, ordered inputs, and
  connection/allocation epoch;
- canonical source IDs and full view/range authority;
- every scheduler copy identity as `(source canonical ID, destination backend
  ordinal, copy slot, copy generation)` plus destination allocation-relative
  range; and
- separate local and per-RPC census roots.

The snapshot comes from the scheduler's real split and copy map. It may not be
constructed from tensor names, pointers, flags, or a parallel llama traversal.
It authenticates intended identity and ranges only; it makes no post-copy
content-equality claim.

For each endpoint, the prepared snapshot binds the coordinator scheduler graph
UID to a scheduler-derived endpoint subgraph/copy-map identity, expected
socket, and connection/allocation epoch. L44 begins from that selected RPC
portion and binds the prepared endpoint identity, root, execution sequence,
scheduler generation, socket, and epoch before compute.

L40 serialization and its endpoint RPC graph UID/digest/receipt occur inside
the real RPC compute path, so they are not falsely claimed at prepare time.
During compute L40 independently creates that authority. Final composition
then authenticates the mapping between the prepared endpoint identity and the
actual accepted L40 graph UID/digest/receipt. Recompute binds the prior accepted
endpoint-specific L40 lineage at compute and finalization.

### Per-execution lifecycle

Replace global one-shot enablement with explicit value handles:

1. `arm`: admit a fresh attempt nonce and monotonic execution sequence;
2. `prepare`: perform the actual scheduler split/allocation preparation and
   freeze the immutable prepared snapshot;
3. `compute`: allow exactly one bound scheduler execution and its L44 mutable
   session(s);
4. `final transcript`: after full scheduler, backend, and RPC synchronization,
   finalize L42 execution records and collect authenticated L44 server-applied
   receipts; return from asynchronous compute is not completion;
5. `finalize`: authenticate an adjacent composition receipt binding prepared
   root, final L42 root/result tag, every L44 census/mutation/semantic root and
   receipt tag, graph UID, execution sequence, and epochs; or
6. `abort`: close and wipe every scheduler/RPC handle and refuse later use.

Every prompt chunk, replay decode, graph reuse, and recompute execution receives
its own monotonically increasing identity. A reused graph still performs a
fresh prepared admission over the scheduler's retained real split/copy
authority. Recompute additionally binds the accepted L40 graph lineage.

Only one state transition is valid from each lifecycle phase. Stale, closed,
reused, foreign, cross-attempt, cross-graph, cross-execution,
cross-connection, or cross-allocation handles refuse. Missing finalization
makes the result non-admissible. Failure after any partial prepare or RPC
session creation mandates abort of both layers and makes every prepared/final
artifact from that execution inadmissible.

The current scheduler and L44 socket authority are serialized. A second
overlapping arm on the same scheduler or active L44 socket refuses. Isolation
is required across distinct scheduler/socket instances and across serialized
executions; no simultaneous same-socket session is promised. Each admitted
execution still uses separate value handles, buffers, and transcript state,
with no process-global pointer authority.

## Compatibility and evidence

The old one-shot L42 entry point is removed or made an explicit refusing legacy
shim when composed authority is compiled. Misuse must not silently produce
partial evidence. Accepted L40 client/server graph receipts and L44
server-applied SET/SET_HASH receipts remain authoritative and are not
redesigned.

Compile and runtime feature-off paths preserve existing traversal,
synchronization, allocation, logging, and wire behavior. No session or
snapshot exists when off. A non-RPC execution uses the same scheduler lifecycle
with an authenticated local-only census and zero RPC receipts; it remains an
honest recomputation path and never claims distributed authority.

All records are bounded canonical little-endian encodings in a new HMAC domain
`halofpx.scheduler-rpc-composition.v1`. Limits are frozen at 65,536 graph
entries, 4,096 splits, 16,384 copy mappings, 4,096 census entries per RPC
endpoint, 16 RPC endpoints, and 8 MiB total exported evidence per execution.
Reserved fields must be zero. Unknown versions, records, roles, storage
classes, duplicate IDs, noncanonical order, trailing data, arithmetic
overflow, or limit excess refuse. Records retain hashes and metadata only:
never raw pointers, keys, tensor/model/state/weight bytes, or secrets.

## Qualification boundary

Focused tests must cover lifecycle order, locality, dynamic views, bounds,
concurrency, feature-off behavior, and the specified stale/missing/tampered
refusals. Qualification then uses one disposable two-fresh-residency
stories15M canary/runner path with multiple prompt chunks and replay on a mixed
RPC0/ROCm0 graph. It must bind prepared and final receipts, exercise real SET
and available SET_HASH behavior, prove exact output, zero legacy state
GET/SET, and clean teardown. No primary artifact or production mutation is
authorized by this ADR.

## Rejected alternatives

- Registering scheduler inputs by tensor name, size, or `INPUT` flag: this is
  not structural authority and cannot bind generated copies.
- Treating local leaves as absent from L44: omission is not authenticated
  locality.
- Committing the unsplit llama graph as if it were the RPC graph: it confuses
  coordinator and worker ownership.
- Claiming final copy equality from the prepared snapshot: content equality is
  established only by the postcompute L42 transcript.
- Retaining global one-shot scheduler authority: it cannot distinguish prompt,
  replay, reuse, or concurrent execution lifetimes.
