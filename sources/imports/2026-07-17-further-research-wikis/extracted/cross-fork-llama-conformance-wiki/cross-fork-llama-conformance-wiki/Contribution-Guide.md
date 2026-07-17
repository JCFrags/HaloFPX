# Contribution guide

## Adding a case

1. Choose a stable area prefix and next three-digit ID.
2. State one semantic intent; split unrelated contracts.
3. Select an oracle before writing execution code.
4. Add logical fixture IDs and manifest entries.
5. Map upstream tests that can be reused.
6. Declare CI tier, backend scope, and all four fork applicability states.
7. Define expected behavior without inventing model-specific numbers.
8. Add failure injection only with a watchdog and isolation plan.
9. Add or update harness unit tests.
10. Run `python3 scripts/verify-suite.py`.

## Oracle review questions

- Can the result be exact after narrow normalization?
- Is this actually a capability probe rather than a correctness test?
- Does a numeric comparison have an approved reference prerequisite?
- Are model and quant bytes identical?
- Is a stochastic statistic preregistered?
- Does the case preserve raw evidence?
- Can a missing feature be distinguished from a broken test environment?
- Does the error contract compare semantics rather than brittle prose?

## Changing fixtures

Never edit a fixture in place when an approved reference depends on its digest. Add a new versioned file/ID, update the matrix intentionally, and retire affected references.

External models require license review, source pinning, and SHA-256. Do not commit model binaries to this wiki repository.

## Upstream updates

When upstream changes a reused test:

- update the source inventory pin;
- inspect test additions/removals/renames;
- preserve upstream assertions;
- update the reuse map;
- invalidate stale observations;
- recalibrate only the profiles whose scope changed.

## Fork-specific behavior

A fork-specific extension may have a separate oracle only where upstream has no corresponding semantic implementation. Shared behavior still compares to upstream. Document intentional divergence explicitly; do not encode it as a silent normalization.

## Review checklist

- [ ] Case ID unique and schema-valid.
- [ ] Fixture paths exist; external digests are locked before execution.
- [ ] No candidate-generated baseline update.
- [ ] No populated numeric threshold in an unapproved profile.
- [ ] Capability skip states are evidence-backed.
- [ ] Failure cases are bounded and isolated.
- [ ] CI lane and artifact retention are specified.
- [ ] Documentation and machine-readable records agree.
