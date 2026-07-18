# ADR-0015: bootstrap-consumption ambiguity reconciliation

- Status: accepted for the disabled offline L05l seam
- Date: 2026-07-18

## Decision

L05l stops at protected-registry reconciliation after an ambiguous L05k
consumption attempt. It must not inspect or create the protected publication
anchor. ADR-0004 requires every exact object and the authenticated manifest to
be published and synchronized before the anchor may select generation one.
The L05k proof contains receipt identities, not a durable-material proof for
those preceding steps. Opening create-if-absent here would freeze an unsafe
publication order.

The excluded, synchronous, memory-only reconciler reconstructs the exact L05k
operation from the original authorized plan, authenticated v1 predecessor,
transient registry key, original nonzero consumption-attempt identity, and the
backend's fixed nonzero root identity. Callers cannot supply a parsed head,
successor, receipt, operation commitment, or trusted dynamic registry scalar.
Each reconciliation also has a fresh nonzero 256-bit attempt identity.

Reconciliation extends the same root backend instance that classified the L05k
operation as uncertain. L05k therefore retains the exact uncertain operation
and an internal `consumption_uncertain` phase. Reconciliation is admitted only
when root, original attempt, original operation commitment, predecessor,
successor, and every proof-equivalent binding match that recorded operation.
Never-invoked, definitely rejected, replayed, already proven, differently
bound, or already reconciled operations cannot enter reconciliation.

The only reconciliation backend primitive is an atomic, root-scoped
`fence_original_and_read_current` operation. Before it returns an authoritative
head, it conclusively joins or fences the exact original attempt so that the
old compare-and-advance can never complete later. A separate read followed by
fencing is forbidden: observing the predecessor before a late original CAS
would falsely authorize a retry.

The reconciliation operation commitment is:

```text
SHA-256(
  "halofpx.bootstrap-consumption-reconciliation.v1\0" ||
  root identity[32] || reconciliation attempt ID[32] ||
  original consumption attempt ID[32] ||
  original consumption operation commitment[32] ||
  uint64be(predecessor envelope length) || exact predecessor envelope ||
  uint64be(successor envelope length) || exact successor envelope ||
  uint64be(proposed anchor envelope length) || exact proposed anchor envelope)
```

The backend independently recomputes that commitment. It owns root-scoped
terminal histories for original and reconciliation attempts, the exact
uncertain operation, phase state, and sticky quarantine for its lifetime.
Execution is private and coordinator-only; constructing another coordinator
cannot bypass those records. L05l provides no public quarantine-clear
operation. A successful reconciliation resolves only the exact recorded
consumption phase: exact successor becomes `successor_recovered_terminal`, and
exact predecessor becomes `predecessor_confirmed_terminal_no_retry`. It does not
clear unrelated/new uncertainty or make ordinary consume/create eligible.

## Exact-head classification

The backend returns an owned exact-head byte witness of at most 1024 bytes from
the same atomic fence-and-observe operation. Every present outcome requires
`1..1024` bytes. Absence requires an empty witness. Every definite response must
echo the exact root identity, reconciliation attempt ID, reconciliation
operation commitment, original consumption attempt ID, and original operation
commitment. The wrapper checks all five before classifying bytes.
Contradictory outcome/witness shape, wrong confirmation, exception, timeout,
malformed response, or late-completion risk is visibility uncertainty,
sticky-quarantines the root, and exposes no proof.

The wrapper compares length, domain-separated digest, and every byte. Parsed
fields, high-water equality, receipt equality, or digest equality alone never
substitute for exact bytes.

The exact-head comparison digest domain is
`halofpx.registry-head-witness.v1\0`, with exactly one trailing NUL, followed by
the unsigned 64-bit big-endian witness length and exact witness bytes. This
digest is an additional comparison check; it never replaces the final exact
byte comparison.

- Exact successor current after the confirmed terminal fence returns
  `consumed_same_recovered_unexecuted` and a move-only recovered proof.
- Exact predecessor current after the confirmed terminal fence returns
  `definitely_unconsumed_fenced_no_retry`. It returns no proof or retry
  authority. A future milestone must define a move-only, exactly bound retry
  permit and require it in a fresh consumption path before retry is possible.
- Any other authoritative present bytes return `conflict`, including an older,
  newer, different-command, corrupt, or unsupported-format head. They cannot
  authorize retry, bootstrap, or fallback.
- An absent established registry, unreadable state, incomplete observation, or
  unconfirmed terminal fence returns `visibility_uncertain` and quarantines.

Original and reconciliation attempt identities are terminal after first
registration. A reconciliation replay returns no witness or proof. Because the
backend phase machine admits at most one reconciliation attempt over its
lifetime, it retains that one exact identity without eviction or an artificial
multi-entry exhaustion surface.

## Proof and state boundary

An ambiguous L05k call produced no proof. The reconciler therefore reconstructs
proof-equivalent material from the exact authorized inputs and admits it only
after the backend confirms that the same recorded operation was in the
`consumption_uncertain` phase. The recovered proof is opaque, move-only, and
source-invalidating. It owns the exact predecessor and successor carriers,
proposed anchor carrier, root identity, original consumption attempt and
operation commitment, command and authorization sequence, token digest, plan
commitment, authority snapshot commitment, selected-manifest digest, the
confirmed original uncertain phase, fresh reconciliation attempt and
reconciliation commitment, exact observed successor bytes, and classified
reconciliation outcome. Const accessors expose every nonsecret value needed for
independent recomputation. It retains no registry key.

The state machine is:

```text
consumption_uncertain
  -> reconciliation_in_flight
     -> successor_current_fenced -> recovered proof
     -> predecessor_current_fenced -> terminal, no retry/proof
     -> other_present -> conflict, no proof
     -> absent/unreadable/malformed/late -> sticky uncertainty, no proof
```

The recovered proof authorizes only entry to a future bootstrap-material
preparation flow. It does not authorize object writes, manifest publication,
anchor access, cache admission, inference restore, or another consumption
attempt. Exact-predecessor classification authorizes nothing beyond reporting
that the recorded ambiguous operation is terminal and the predecessor was
authoritatively current at the reconciliation linearization point.

## Deferred anchor gate

The required future ordering remains:

```text
recovered consumption proof
  -> exact durable bootstrap-material proof for ADR-0004 steps 1..6
  -> one atomic protected-anchor inspect/create-if-absent-and-durable-close
```

That later material proof must own the exact authenticated manifest envelope
and complete object descriptor set and prove no-replace publication plus all
required file and directory synchronization under the same root and attempt.
Anchor absence is never an exported reusable proof. Under one serialized atomic
operation, exact proposed anchor already current may be idempotent success, any
other presence blocks, and only conclusive absence may reach create-if-absent.
Every positive result must return the exact observed-current anchor bytes. Any
ambiguous create, sync, close, exception, or late completion remains fenced
until separately qualified reconciliation.

## Limits

L05l adds no concrete protected store, key custody, persistent or cross-process
journal, real restart-surviving fence, rollback resistance if head and keys both
roll back, successor-aware `H + 2` planning, credential/principal policy,
object/manifest publication, durable-material proof, anchor absence or creation,
bootstrap execution, cache hit/admission/write, filesystem, server/provider,
node, distributed, or durability claim. An in-memory backend does not qualify a
real restart and constructing a new backend loses its synthetic state.
