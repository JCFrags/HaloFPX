# L05d publication attempt lifecycle

Status: accepted after independent adversarial review.

## Boundary

L05d extends only the existing `EXCLUDE_FROM_ALL` offline coordinator and
deterministic simulator. No runtime target links the seam. There is no path,
file handle, provider, server option, background work, persistent state, node
change, or donor code.

The backend now begins an exact request binding before mutation, receives the
same attempt identity at all object, manifest, anchor, sync, and close calls,
and rejects inactive identities without effect. Pre-anchor failure triggers
definite abandonment; an unconfirmed abandonment becomes
`attempt_fencing_uncertain`. Anchor or close ambiguity fences the attempt and
forbids acknowledgement. Publication succeeds only after the exact durable
close step.

The simulator records at most 128 terminal identifiers, never reuses one, and
permits only one active or unresolved attempt per root fixture. Abandonment
converts live staging entries to retained garbage so a fresh identity can begin
without trusting or reusing partial state. A simulated crash fences any active
identity before recovery.

## Tests

The exact success trace now has 23 operations for a two-object fixture: anchor
read, attempt begin, twelve object phases, six manifest phases, exact anchor
CAS, anchor sync, and durable close. The resource/crash matrix covers eight
typed faults before and after every operation under four crash projections,
for 1,472 core scenarios per process.

Focused tests also cover wrong IDs, terminal-ID replay after intervening IDs,
altered begin bindings, definite abandonment, late calls after abandonment and
durable close, fresh-ID restart, uncertain-root blocking, begin uncertainty,
abandonment failure and exception, post-linearization ambiguity, and all nine
final predecessor fields.

## Gates still closed

The in-memory registry is not a production journal and does not survive process
restart. Cross-process ownership, authenticated authority, persistent replay
history, real asynchronous callback cancellation, reconciliation, authority
transfer, filesystem atomicity/durability, capacity controls, server wiring,
nodes, and canaries remain closed. Persistent writes remain disabled.

No CachyLLama or GPL llama-ai code or documentation entered the engine. The
direct-cherry-pick roster remains empty. Rollback is source-only.
