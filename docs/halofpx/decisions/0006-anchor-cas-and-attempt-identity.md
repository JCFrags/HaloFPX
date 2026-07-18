# ADR-0006: anchor compare-and-swap and publication attempt identity

- Status: accepted for the disabled offline L05 coordinator
- Date: 2026-07-18

## Context

The L05a coordinator read and checked the protected anchor before preparing
immutable material, but its final replacement operation supplied only the next
anchor. Two coordinators outside the shared in-process fence could therefore
both read one predecessor and later ask an unqualified backend to replace it.
The accepted formal model instead records an attempt owner, generation, and
predecessor and permits anchor replacement only while that exact attempt can
still advance from the durable predecessor.

## Decision

Every publication request carries a nonzero 256-bit `attempt_id`. A qualified
authority must generate it with a cryptographically secure random generator and
must not reuse it. The identifier conveys no authority by itself and is not
part of the protected anchor identity.

The anchor linearization primitive is an exact compare-and-swap over:

1. the attempt identifier;
2. the full expected predecessor anchor, including store, namespace, lineage,
   policy epoch, key generation, authority epoch, generation, manifest digest,
   and predecessor digest; and
3. the full proposed next anchor.

The replacement may succeed only if the protected current anchor equals the
expected predecessor exactly and the attempt is authorized for that exact
transition. A backend may return the typed `stale_predecessor` step result only
when it can prove atomically that no replacement occurred. The coordinator maps
that result to a safe stale rejection. Every failed, interrupted, throwing, or
otherwise ambiguous replacement remains `anchor_visibility_uncertain`.

An ordinary publication cannot change authority epoch. Authority transfer,
cross-process ownership, attempt-token storage and authentication, cancellation,
and late asynchronous completion fencing require separate protocols and
qualification. Merely forwarding an attempt identifier does not prove them.

## Consequences

The excluded coordinator and deterministic simulator can now express and test
the final stale-writer CAS boundary. Distinct coordinators may prepare the same
immutable generation, but only one exact predecessor transition can publish;
the loser retains unreachable material and receives a conclusive stale result.

No concrete filesystem backend, protected anchor format, server hook, runtime
option, persistent write, donor code, or node behavior is authorized by this
decision.
