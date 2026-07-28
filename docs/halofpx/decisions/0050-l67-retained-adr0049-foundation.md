# Decision 0050: retain the L67 ADR-0049 foundation

Status: accepted
Date: 2026-07-27
Scope: default-off foundation only

## Decision

Retain L67 as the bounded reusable ADR-0049 capability and lifecycle
foundation. It does not authorize a stories milestone, model execution, cache
qualification, performance claim, or production mutation.

The retained foundation:

- builds an unsigned expected admission independently from scheduler source
  authority, then verifies every sealed field and the admission HMAC on both
  sides;
- binds the authenticated execute intent and signed server receipt to the same
  admission object and expected-admission digest, with server consumption
  immediately before backend execution;
- uses a versioned finite table of exact success, lifecycle-refusal, and
  transport-refusal productions;
- publishes one read-only per-attempt authority file through atomic no-replace
  linkage, file and directory durability, and byte-exact reopen validation;
- retains a compact machine-validated manifest whose authority records and
  binary identity receipt are HMAC-authenticated.

## Qualification boundary

Focused gates passed for independent/partial/self binding, reuse,
expired/aborted state, exact grammar and structural negatives, a real
transport failure, separate-process collision refusal, compile-off,
runtime-off, and the real composed no-model fixture. The final composed result
was exact and overlapping, with multiple UIDs/sequences and allocation epoch
rollover.

Independent adversarial review found no remaining correctness, security, or
manifest P1/P2 and recommended PASS/retention. The exact evidence authority is
`docs/halofpx/evidence/l67-focused-manifest.json`.
