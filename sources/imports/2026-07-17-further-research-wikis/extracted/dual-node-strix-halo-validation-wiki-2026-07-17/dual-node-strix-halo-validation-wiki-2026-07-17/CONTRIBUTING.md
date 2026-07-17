# Contributing to the Validation Wiki

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


Changes that alter a metric definition, raw schema, release gate, threshold, SLO, cache state, or fault expectation require:

1. A change rationale and affected claim list.
2. A version bump in the relevant schema/config.
3. A migration note for historical data.
4. Re-execution of affected canary experiments before the change becomes normative.
5. Two reviewers: one performance/observability reviewer and one correctness/reliability reviewer.

Do not delete failed runs. Mark them invalid only under a predeclared invalidation rule, retain raw evidence, and record the reason.
