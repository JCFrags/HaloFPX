# ADR-0023: portable registry-lab recovery mutation admission and terminalization

- Status: accepted for fake-only recovery terminalization after independent adversarial review; Linux and persistence remain closed
- Date: 2026-07-18

## Decision and narrow boundary

This decision amends ADR-0018 and ADR-0019 only enough to implement the first
fake-only mutation lane after accepted read-only operation 5. It authorizes:

- `needs_predecessor_abort` -> atomic operation-6 admission -> one recovery-
  class ABORT publication; and
- `needs_successor_close` -> atomic operation-6 admission -> durability
  reassertion -> one recovery-class CLOSE publication.

Every other operation-5 classification keeps its accepted L05p behavior.
`continue_to_mutation` still stops at private event 201. Sticky or existing
quarantine still stops without writing quarantine. Operations 10-14, 20-21,
30-32, 34, 40-42, 44, 69-75, initialization, new CAS, successor or selector
creation, rename, HEAD replacement, normal terminalization, Linux primitives,
and concrete observations remain compile-time unavailable.

The implementation remains inside the existing internal
`halofpx-context-store-registry-lab-read-only` `STATIC EXCLUDE_FROM_ALL`
target. The target name is historical; this amendment does not create a new
target or product edge. No public header, option, install/export rule,
filesystem path, syscall, server/provider/cache dependency, persistent write,
or performance claim is admitted. The only mutation is to the fixed fake live
and durable projections. Feature-off product behavior remains the control.

## Amendments to ADR-0018 publication ordering

Both target publication renames in ADR-0018 cross directory boundaries. A
destination-directory synchronization alone cannot establish removal of the
source staging name. The complete future normal orders therefore become:

- successor publication: operations 30,31,32,33,34,35,36, where operation 35
  synchronizes destination `envelopes/` and operation 36 synchronizes source
  `staging/`; and
- selector publication: operations 40,41,42,43,44,45,46, where operation 45
  synchronizes destination root and operation 46 synchronizes source
  `staging/`.

Operation 43 means synchronization of the authenticated selector file. Before
replacement it is the selector staging file; during successor recovery it is
the already-published file named `HEAD`. This correction does not authorize
either future normal publication sequence in this milestone.

## Action material before operation 6

Operation 5 derives one authenticated recovery action from its immutable
snapshot. Before operation 6, the engine uses the accepted target-owned
encoder to construct the exact recovery-class terminal in fixture-preallocated
private scratch:

- predecessor recovery encodes ABORT phase 1, terminal class 1, and the exact
  unchanged authenticated predecessor HEAD digest; or
- successor recovery encodes CLOSE phase 2, terminal class 1, and the exact
  authenticated successor HEAD digest.

Both terminals bind the decoded PREPARE's exact root, path policy, physical
slot, attempt ID, operation commitment, predecessor/successor digests, and
complete PREPARE content digest. Encoding and self-verification complete before
operation 6. Encoder failure, scratch overflow, or witness contradiction is
`quarantined_or_unavailable` without mutation; it never falls back to caller
fields or a normal terminal class. All terminal scratch is wiped on every path.

## Operation 6: atomic action-mutation admission

Operation 6 is one indivisible fake operation named `action mutation
admission`. It receives no script-supplied authority. The engine supplies a
private action commitment over the engine-derived action kind, physical slot,
attempt ID, authenticated PREPARE content digest, authenticated current HEAD
content digest, and operation commitment.

The commitment is exactly:

```text
SHA-256(
  "halofpx.registry-lab-recovery-action.v1\0" ||
  uint8(action kind: 1 predecessor ABORT, 2 successor CLOSE) ||
  uint64be(physical slot) || attempt ID[32] ||
  PREPARE content digest[32] || current HEAD content digest[32] ||
  operation commitment[32])
```

The shown domain contains exactly one terminating NUL byte. The fake
independently recomputes the commitment from its operation-5 authenticated
carrier state; caller or script bytes cannot supply it.

The fake operation atomically:

1. proves the complete current action-critical fake state still equals the
   operation-4 snapshot admitted by operation 5, including marker, HEAD,
   selected envelope, complete history, target slot, terminal absence,
   directory projections, and staging absence;
2. recomputes current regular-file logical bytes without overflow and requires
   `current + 1024 <= 16777216`, reserving the complete maximum terminal even
   if the encoded terminal is shorter;
3. rechecks the modeled free-space observation at the exact 256 MiB boundary;
   and
4. on confirmed `ok`, latches the action as uncertain before returning to the
   engine.

The next action operation is invoked immediately. There is no allocation,
callback, logging, exception-producing work, result construction, or
injectable definite-state gap between successful operation 6 and that call.
Any exception or invariant violation after the latch maps to
`uncertain_requires_recovery`.

Operation 6 may report latent primitive code `ok`, `capacity_exhausted`,
`reserve_exhausted`, `unsupported`, `unavailable`, or `io_failure`; its storage
effect is always `none`, and confirmed, response-lost, and process-death
completions are admitted. `busy` and `invalid_request` are forbidden.
Confirmed `unavailable` is reserved for an action-critical state/commitment
mismatch and maps to `quarantined_or_unavailable` without new mutation. Every
other non-`ok` or response-lost outcome maps to
`uncertain_requires_recovery` in this recovery-only lane. A confirmed resource failure proves that this invocation
made no new mutation, but it cannot erase or conceal the already-authenticated
unresolved PREPARE. This specializes ADR-0020's definite resource-check wording
for recovery; a future new-CAS lane must separately decide its pre-PREPARE
resource mapping. Process death has no ordinary result.

## Exact recovery scripts

The injectable primitive script remains immutable and fully validated before
engine entry. It is a closed discriminated shape whose suffix is selected only
after operation 5 independently derives and matches the oracle classification.
It contains the faultable operation prefix through operation 64: 11 entries
for predecessor recovery and 19 for successor recovery. Operations 90-92 are
non-faultable engine-owned cleanup events, not injectable script entries. The
complete emitted execution traces, including cleanup, are exactly:

Predecessor recovery is exactly:

```text
1,2,3,4,5,6,60,61,62,63,64,90,91,92
```

Successor recovery is exactly:

```text
1,2,3,4,5,6,33,35,36,43,45,46,50,51,60,61,62,63,64,90,91,92
```

For successor recovery, operations 33,35,36 reassert the already-published
successor file plus both sides of its earlier cross-directory rename;
operations 43,45,46 reassert the authenticated current `HEAD` file plus both
sides of its earlier replacement; and operations 50,51 re-open and
authenticate exact HEAD and its exact selected successor before CLOSE. These
operations do not rewrite or rename successor or HEAD. Operation-5 staging
absence remains mandatory. A retained staging name is sticky invalid state,
not an object that this lane repairs.

Operations 60-64 are terminal unique-create, exact bounded write, exact
readback/authentication, terminal-file synchronization, and attempts-directory
synchronization. Operation 60 never replaces an existing name. Operation 61
writes only the preverified exact terminal. Operation 62 compares every byte,
redecodes, reauthenticates, and rebinds the terminal before any later step.

## Normative primitive-product amendment

The existing primitive-code, storage-effect, and completion axes remain
orthogonal. The following rows add exactly 287 admitted products across 14
newly executable operation IDs. Every omitted product rejects before engine
entry.

| operations | codes | effects | completions and restrictions |
|---|---|---|---|
| 6 | `ok`, `capacity_exhausted`, `reserve_exhausted`, `unsupported`, `unavailable`, `io_failure` | `none` | C, L, or D; no payload |
| 33,35,36,43,45,46,63,64 | `ok`, `unavailable`, `io_failure` | `none`, bounded partial durability, or complete durability | C, L, or D; confirmed `ok` requires complete durability |
| 50,51,62 | `ok`, `unavailable`, `io_failure` | `none` | C, L, or D; confirmed `ok` requires exact authenticated bytes |
| 60 | `ok`, `unavailable`, `io_failure` | `none` or atomic empty `complete_live` name | C, L, or D; confirmed `ok` requires `complete_live` |
| 61 | `ok`, `unavailable`, `io_failure` | `none`, bounded partial bytes, or exact `complete_live` bytes | C, L, or D; confirmed `ok` requires exact `complete_live` |

Here C, L, and D retain ADR-0019's meanings. For every L or D product, the
latent code is oracle-only and the engine observes no code. Confirmed failure,
lost response, and death may retain any effect permitted by the row. A sync
may have made a complete durability projection despite reporting failure; that
still grants no result. Bounded partial bytes are exact issued prefixes.
Namespace effects are atomic, never partial names.

Operations 90-92 remain the only non-faultable cleanup suffix. Modeled death
first applies an admitted restart projection and returns no ordinary result;
the fake-only teardown audit then proves credential wipe and projected lock/
guard release without claiming a killed real process ran cleanup code.

## Results and visibility

This lane adds exactly:

- ordinary `recovered_not_applied_no_authority` after durable ABORT; and
- private fake-only `modeled_recovered_successor_closed` after durable CLOSE.

It does not add `modeled_advanced_closed`.

Neither result exists or becomes visible until operation 62 has authenticated
the exact terminal, operation 63 has confirmed complete file durability,
operation 64 has confirmed complete attempts-directory durability, and cleanup
90-92 has completed in order. The private CLOSE disposition contains no bytes,
path, reusable authority, or conversion seam. It is test evidence only.

Any non-`ok` operation-6 outcome, response loss, post-admission primitive
failure, late completion, readback contradiction, or exception returns no
positive disposition. Confirmed operation-6 `unavailable` and a confirmed
authenticated readback contradiction map to `quarantined_or_unavailable`;
other post-admission failures map to `uncertain_requires_recovery`. Process death returns no ordinary result. A
terminal that may already be durable after an error or lost response is never
rediscovered as a new positive result: a later operation-5 scan observes
terminal history and applies replay/current-history rules.

## Fake state and restart closure

Successful predecessor recovery changes only the selected attempt slot by
adding one exact authenticated recovery ABORT whose bytes and namespace are
durable. HEAD and immutable envelopes remain unchanged.

Successful successor recovery does not rewrite successor or HEAD. It proves
their file and source/destination directory durability, reauthenticates them,
then adds one exact authenticated recovery CLOSE whose bytes and namespace are
durable.

Every fault suffix enumerates all and only restart projections permitted by
the effect table:

- before or at operation 6, the original unresolved PREPARE remains;
- a terminal create may restart absent or as one complete empty name;
- terminal writes may retain no bytes, an exact issued prefix, or exact
  complete bytes, never invented or mixed bytes;
- unsynchronized terminal namespace may be absent or wholly present according
  to its directory projection;
- empty, partial, incomplete, live/durable-disagreeing, or malformed retained
  terminal state becomes sticky invalid state;
- an exact durable ABORT becomes terminal history with no retry authority;
- an exact durable CLOSE becomes one-CLOSE current history, but never
  fabricates a lost prior disposition; and
- successor reassertion faults preserve an exact recoverable state or become
  sticky, never inferred success.

Restart cannot invent bytes, a name, a durability projection, terminal
attribution, authority, or cleanup. The serialized restart image continues to
exclude scripts, traces, credentials, derived keys, terminal scratch, and
dispositions.

## Required qualification before implementation promotion

At minimum, the focused suite must prove:

- all 287 admitted products execute and every other product for the 14 new
  operation IDs rejects before entry;
- exact predecessor and successor traces and rejection of every extra,
  missing, reordered, or wrong operation;
- before/after faults and every retained/discarded restart projection at each
  new boundary, including crash after operation 64 before result visibility;
- zero, exact-limit, one-byte-over, and integer-overflow logical accounting;
  exact 256 MiB reserve and one-byte-below; and external reserve loss after
  successful operation 6 treated as uncertainty rather than retroactive
  rejection;
- mutation of any action-critical state between operations 5 and 6 rejects
  before new storage mutation;
- both recovery actions at every one of 512 physical slots, after zero, one,
  and many valid older ABORT pairs;
- generated recovery terminals require class 1 and exact kind phase, slot,
  attempt, PREPARE digest, operation commitment, predecessor/successor digest,
  terminal HEAD, key tuple, and tag;
- recomputed-tag semantic attacks, every terminal truncation, and every
  one-bit terminal mutation fail closed;
- no operation 10-14,20-21,30-32,34,40-42,44,69-75 can be scripted, called,
  linked, or traced, and no QUARANTINE, successor, selector, or HEAD mutation
  occurs;
- allocation-free and `noexcept` execution after fixture construction,
  complete wipe-before-release, process-wide death/restart behavior, and
  secret exclusion from fake state, restart images, traces, results, and raw
  evidence;
- Windows Release and Debug, Linux ASan/UBSan, optimized isolation/archive
  audits, feature-off contract, full HaloFPX and focused inherited regression
  suites; and
- repeated exact-candidate matrices with source/executable/evidence hashes and
  independent milestone review.

## Explicit non-claims and next gates

This decision does not authorize quarantine publication or event-ID operation
69, new compare-and-advance, initialization, Linux credentials/locks/syscalls,
filesystem or process-crash evidence, concrete observations, persistent
writes, a durability mode, M63-01 closure, production key custody, cache reuse,
server/provider linkage, restore, inference behavior, or performance claims.

Quarantine publication requires its own reviewed reason/event-ID decision.
Normal CAS requires a separate amendment covering operations 10 onward and the
corrected 36/46 source-directory synchronization. A Linux adapter remains
blocked until the complete portable fake mutation and every-boundary restart
matrix pass independent review.
