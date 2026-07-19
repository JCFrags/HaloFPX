# ADR-0023 independent adversarial review

- Date: 2026-07-18
- Decision reviewed:
  `docs/halofpx/decisions/0023-portable-registry-lab-recovery-terminalization.md`
- Accepted decision SHA-256:
  `0f6ab73c88498a694fb22e50105f3fa5da31784d4e2c319829d6a09a7bd72c17`
- Result: **ACCEPT**

## Scope

The review compared ADR-0023 with ADRs 0018 through 0022, the normative
registry-lab CDDL and target-native wire/encoder behavior, the accepted L05p
implementation, canonical Wiki Section 63, and the project's feature-off,
mutation, provenance, and persistence gates. It examined operation-6 authority,
resource and uncertainty mapping, both recovery scripts, cross-directory
synchronization, terminal visibility, restart closure, and forbidden-surface
isolation.

## Independent algebra check

The reviewer independently recomputed the new closed operation algebra:

- operation 6: `6 * 1 * 3 = 18` products;
- eight synchronization operations: `25` each, `200` total;
- three reads/readbacks: `9` each, `27` total;
- terminal create operation 60: `17`; and
- terminal write operation 61: `25`.

The sum is exactly 287 admitted products across 14 newly executable operation
IDs. The confirmed-`ok` restrictions remove incomplete effects from the broad
code/effect/completion cross-products; every unlisted product remains invalid.

## Acceptance findings

The exact recovery-action commitment and independent fake recomputation prevent
script-supplied authority. Complete action-critical state equality closes the
operation-5-to-6 stale-snapshot gap, while the immediate uncertainty latch and
absence of intervening allocation/callback work close the operation-6-to-first-
action gap.

Confirmed operation-6 `unavailable` is narrowly reserved for state/commitment
mismatch and maps to `quarantined_or_unavailable`. Resource, unsupported, I/O,
and lost-response outcomes remain uncertain because the authenticated unresolved
PREPARE predates the invocation. This conservative distinction does not conceal
recovery work behind a no-mutation status.

The successor script correctly reasserts the successor and HEAD files, both
destination and source directory projections through operations 36 and 46,
and exact HEAD/successor authentication before terminal CLOSE. The predecessor
ABORT script is the minimal coherent action. Neither result becomes visible
before exact terminal readback, file and attempts-directory synchronization,
credential cleanup, lock release, and guard release.

Restart projections do not invent bytes, names, durability, attribution, or a
lost positive disposition. The decision agrees with the corruption-as-miss/
quarantine rule and makes no filesystem, persistence, durability-mode,
production-authority, cache, restore, inference, or performance claim.

After implementation review exposed ambiguity between injectable primitive
entries and engine-owned cleanup events, the decision was clarified to freeze
11/19 faultable script entries separately from the complete traces that include
operations 90-92. The independent rereview accepted that clarification and
confirmed that it changes no operation algebra or authority boundary.

## Acceptance boundary

Acceptance authorizes only implementation of fake operation 6 and recovery
ABORT/CLOSE terminalization in the existing excluded target. Normal CAS,
quarantine publication, operation 69, Linux primitives, concrete observations,
persistent writes, and product linkage remain closed. Independent implementation
review and the complete every-boundary matrix are still required before this
milestone can be promoted.
