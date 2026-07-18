# ADR-0021: portable registry-lab operations 1-4 execution closure

- Status: accepted for operations 1-4 only
- Date: 2026-07-18

## Decision

This decision closes the implementation details required to build the first
ADR-0020 code slice. It authorizes only the internal portable fake path through
operations 1-4. Operation 5, authenticated decoding, recovery classification,
mutation, Linux primitives, runtime linkage, and positive authority remain
unimplemented and unavailable.

The target is named `halofpx-context-store-registry-lab-read-only`. It is
`STATIC EXCLUDE_FROM_ALL`, has no public header, option, install/export rule, or
product edge, and links privately only to the existing registry-lab wire
target. Its one internal header is included only by its implementation and the
focused test. The engine and final fake Ops remain implementation-private; no
generic Ops template, virtual interface, callback, factory, or entry point is
available to another target.

## Complete product and payload rules

The constexpr algebra covers all 600 combinations of five operation IDs,
five storage effects, three completions, and eight primitive codes. Exactly 55
are admitted by ADR-0020: 4 for operation 1, 6 for operation 2, 21 for
operation 3, 12 for operation 4, and 12 for operation 5. Every admitted effect
is `none`. The first executable slice exhaustively tests the 43 admitted and
437 forbidden operation 1-4 products; it also statically verifies all 55
operation 1-5 products even though operation 5 cannot be invoked.

Operations 1-4 carry no payload on any code, including `ok`. Any payload or
recovery classification attached to operations 1-4 is rejected before engine
entry. Operation 5 reserves exactly one recovery-classification payload for a
confirmed `ok`; its payload rules are tested in the algebra but no operation-5
call exists in this slice. Cleanup IDs 90-92 are never scriptable.

Every admitted operations 1-4 invocation supplies exactly four immutable
script entries in numeric order, one each for operations 1, 2, 3, and 4. The
complete four-entry algebra/payload shape is validated before entry even when
an earlier entry will terminate execution. Unused suffix entries are never
executed but cannot hide a forbidden product. A paused invocation retains this
same script and a monotonic cursor; resume consumes the next entry exactly once
and cannot replace, rewind, skip, or reinterpret an entry. Terminal completion
invalidates the remaining cursor permanently.

The all-`ok` path ends only in the internal test event
`reached_operation_5_boundary_test_only`. This is not an ordinary status,
clean-state result, absence result, continuation token, observation, or
authority. It exists only in the fixed internal event trace and cannot be
returned or linked outside the focused test target.

## Request admission and internal faults

Before operation 1, the driver validates closed enum representations, exact
script length/order, every product and payload rule, bounded trace capacity,
nonzero invocation/process identities, and a structurally valid move-owned
credential. Rejection maps to `invalid_request_no_mutation`, wipes the scoped
owner, records no operation, acquires no ownership, and changes no fake state.
The rejection result becomes test-visible only after that wipe audit completes.

The operations 1-4 engine, cleanup, trace, restart projection, and fixed
containers are `noexcept` and dynamically allocation-free after fixture
construction. There is no exception callback or exception outcome axis.
Qualification installs a failing allocation oracle after fixture construction
and proves every admitted invocation and restart projection still completes.
It also audits the engine archive for allocator imports. Allocation failure
during external test-fixture construction is outside engine entry, yields no
engine result or trace, and is not a storage outcome.

An unexpected language or internal exception cannot be injected through Ops,
and this slice makes no exception-to-status claim. Every function reachable
after engine entry is statically `noexcept` and uses only fixed operations that
cannot throw. Throwing across that contract would terminate the test process
and fail qualification rather than fabricate a recoverable engine status. No
test-only seam may manufacture exceptions. A future slice that introduces a
potentially throwing operation must first amend this decision with a feasible
containment boundary and exact cleanup/status semantics.

## Ownership, primitive codes, and deterministic interleaving

The fake models one root. Its non-reentrant guard is process-local and
root-specific; its writer lock is root-wide across modeled processes. Each
invocation has a nonzero invocation ID and a fixed modeled-process slot.
Invocations in the same process contend at operation 1. Invocations in
different modeled processes may both own their local guards but contend for
the one writer lock at operation 2.

For operations 1 and 2, ownership is authoritative and the script's primitive
code is an oracle expectation, never a command to acquire or report busy.
Operation 1 derives `ok` exactly when that modeled process/root guard is free
and derives `busy` exactly when it is owned; any mismatch with the script is
rejected before entry for the initial operation. Operation 2 in normal mode
derives `ok` exactly when the root-wide writer lock is free and derives `busy`
exactly when another owner holds it. Its only injectable capability mode is
`unsupported`, which acquires nothing regardless of current lock ownership.
The expected operation-2 code is reconciled atomically when the paused/resumed
invocation reaches operation 2, because another modeled process may have
changed lock ownership after initial script admission. A mismatch is
`invalid_request_no_mutation` after ordinary cleanup; the mismatch acquires or
modifies no writer lock, while cleanup still releases this invocation's guard.

For confirmed `ok`, the corresponding ownership assignment and primitive
response are one indivisible fake operation. Confirmed `busy` and
`unsupported` acquire nothing. For latent process-death `ok`, acquisition
occurs first and the subsequent restart clears only ownership belonging to the
dead modeled process. Latent death `busy` or `unsupported` acquires nothing;
restart still clears ownership already belonging to the dead process and never
clears another process's writer lock. These rules also apply to the oracle
code retained for a death response that the engine cannot observe.

Ownership stores exact owner IDs. A no-op release never clears another
invocation's ownership. Deterministic pause/resume at operation boundaries
must prove both same-process guard contention and cross-process writer-lock
contention in both winner orders, without threads.

Modeled death is process-wide, not invocation-local. The invocation executing
the death appends the dying operation and then stops; every other live
invocation in that modeled-process slot stops at its existing cursor. The
transition atomically and permanently invalidates every such invocation,
makes every ordinary result unavailable, discards and separately audits every
credential and scratch owner, and clears all guard/writer-lock ownership held
by that process. It never clears another modeled process's guard or writer
lock. Thus latent death plus `busy` at operation 1 also kills the same-process
invocation that owned the guard; it cannot leave a resumable owner after a
process-wide restart. A separate `restart_teardown_audit` covers every killed
invocation and proves that no secret was serialized. This explicitly
supersedes ADR-0019's portable simulated-death cleanup sentence; ADR-0020's
no-dead-process-cleanup rule controls. Operations 90-92 never appear in any
dead-process trace.

## Cleanup and result visibility

Every ordinary path that invoked operation 1 appends exactly operations 90,
91, and 92. Operation 90 proves the credential, derived-state placeholder,
tag, scratch, and temporary witness storage are zero before operation 91 can
release the current invocation's writer lock. Operation 92 releases only its
process-local guard after 90 and 91. Releases are no-throw no-ops when the
current invocation never acquired that ownership.

The test driver is a step machine. Its result query returns `not_visible`
until pre-entry wipe or the complete 90-92 suffix has finished. Modeled death
has no ordinary result query value at any point. The trace and teardown audit
remain test evidence, not an engine result.

## Fixed state and restart projection

The fake state exactly follows ADR-0020's typed fixed capacities. It is
preallocated by the test fixture, never placed as a multi-megabyte automatic
object inside an invocation, and contains independent live and durable
namespace, bytes, length, and completeness for every modeled file plus
separate directory projections.

The restart image is a distinct typed fixed-capacity value containing only
durable storage projections. Fieldwise encode/restore is required; copying a
padded C++ object representation is forbidden. It structurally contains no
script, trace, credential, key, tag, scratch, witness, latent code, completion,
guard, lock, invocation ID, or process ID. Restoring makes the selected durable
projection both live and durable and applies the modeled-process ownership
rule above. Operations 1-4 cannot change either storage projection.

## Qualification and graph closure

The focused suite must exhaust the 600-product algebra, execute all 43 admitted
operation 1-4 products, reject all 437 forbidden products before entry, test
out-of-range enums and payloads, prove exact status mappings and traces, cover
all 11 response-loss and 16 process-death products, round-trip every fixed
entry family and directory projection, prove secret-exclusion differentially,
exercise both contention scopes and winner orders, and prove invocation-time
allocation freedom.

The existing six-archive wire audit remains unchanged as an independent
control. A separate seven-archive closure is required for the read-only target:
read-only, wire, registry successor, protected registry, auth, format, and the
selected-base SHA-256 primitive. Windows and Linux archive/object audits must
prove that exact closure, no product reverse edge, no concrete-observation or
OS/filesystem/thread/callback/environment/clock/RNG/logging symbol, and no
donor/import marker. The contract test must prove that only the focused test
links the internal target and that no public/install/export surface exists.

Promotion requires Windows Release and Debug, the full inherited and HaloFPX
suites with feature-off control, and Linux ASan/UBSan plus the independent
archive audit. An independent adversarial review is required before commit.

## Explicit non-claims

This decision does not authorize or implement operation 5 execution, decoding,
recovery classification, operation 6 or any mutation, initialization, Linux
I/O or locks, persistent writes, provider/cache/runtime linkage, a public
result, a positive authority observation, cache reuse, or performance claims.
