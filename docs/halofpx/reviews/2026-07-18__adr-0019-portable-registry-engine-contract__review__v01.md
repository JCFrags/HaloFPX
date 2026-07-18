# ADR-0019 portable registry-engine contract review v01

- Date: 2026-07-18
- Scope: authority boundary, canonical encoders, fake operation/effect algebra,
  transaction/recovery semantics, cleanup ordering, and promotion tests
- Final verdict: **ACCEPT**

## Review history

The first adversarial review returned `REVISE` for seven blockers: the fake
lane constructed the future concrete authority type; fault operations were too
coarse; encoder self-verification could be tautological; initialization status
had no initialization engine; credential wipe-before-unlock was unspecified;
the recovery/capacity/staging test matrix was incomplete; and isolation checks
did not prove dependency/object/link closure.

The revision introduced fake-only dispositions, separately validated lifecycle
witnesses and transactional scratch encoding, atomized operation IDs, a move-
owned wipe-before-unlock credential lifetime, no initialization claim, exact
ADR-0018 ABORT predicates, the missing recovery/quarantine/capacity tests, and
source/dependency/object/link negative audits.

The second review found that effect extent and completion were conflated and
that the first-mutation gate was incorrectly fixed to new-CAS PREPARE. The
contract now models effect and completion orthogonally, branches restart over
permitted retained/discarded projections, and applies dynamic reserve and
uncertainty admission independently to recovery, quarantine, and CAS actions.

The final review required and accepted a normative table for every operation
ID. It freezes atomic create/rename/HEAD behavior, bounded write prefixes,
file/directory durability projections, confirmed-sync requirements, all
response-lost/process-death combinations, and non-faultable ordered credential
wipe, lock release, and guard release. Explicit invalid-product tests are now a
promotion gate. Independent final verdict: ACCEPT.

## Accepted boundary

ADR-0019 authorizes only deterministic target-owned encoders and a portable
transaction/recovery engine over fake state. The fake target cannot define or
construct `concrete_registry_lab_observation`. The first later Linux lane may
qualify only sealed credential and pre-initialization primitives; initialization
and compare-and-advance mutation remain separately compile-gated and closed.

No node, filesystem, persistence, durability, rollback resistance, material,
anchor, cache, restore, inference, or performance claim is admitted.
