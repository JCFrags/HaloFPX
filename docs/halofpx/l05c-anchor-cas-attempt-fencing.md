# L05c anchor compare-and-swap and attempt identity

Status: accepted after independent adversarial review.

## Boundary

L05c tightens the existing default-excluded offline publication seam. It adds
no target, provider, filesystem access, server integration, runtime option, or
persistent behavior. All feature-off and inherited behavior remains unchanged.

The request now requires a nonzero 32-byte attempt identifier. The coordinator
passes that identifier, the full expected predecessor, and the full next anchor
to the final replacement primitive. The primitive is contractually an atomic
exact compare-and-swap. A typed stale result is admitted only when the backend
guarantees that the replacement did not occur; all other replacement failures
remain visibility-uncertain.

The deterministic simulator enforces the same exact predecessor comparison
against its live protected anchor. Its attempt-token check is structural only;
it does not claim cryptographic generation, authenticated authority, durable
token storage, cross-process coordination, or asynchronous cancellation.

## Focused proof

The publication unit test uses two coordinators with distinct root fences to
model processes that cannot share in-memory exclusion. Both observe generation
7. The first pauses at its final CAS, the second publishes generation 8, and
the first is then conclusively rejected as stale with no replacement or
durability acknowledgement. The test also verifies exact nonzero attempt-token
forwarding and rejects an all-zero request before any backend call.

The existing exhaustive simulator matrix continues to cover all 21 operation
boundaries, eight typed resource failures, both before/after injection phases,
and four crash projections. L05c changes only the anchor replacement contract;
its old/new/miss recovery rules and retained-garbage behavior remain intact.

## Gates still closed

This slice does not prove replay-resistant attempt registration, authenticated
writer authority, authority transfer, process locks, late completion fencing,
real filesystem atomicity or durability, anchor bytes, quota/reserve policy,
server integration, nodes, or canaries. Persistent writes remain disabled.

No CachyLLama or GPL llama-ai code or documentation entered the engine. The
direct-cherry-pick roster remains empty. Rollback is source-only: revert this
contract extension and its tests.
