# EXP-NNN — Title

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.



| Field | Value |
|---|---|
| Objective | |
| Release profiles | |
| Required evidence | |
| Estimated measured duration | |
| Risk class | |
| Owner / reviewer | |

## Decision question

State the falsifiable question this card answers.

## Hypotheses

- **H0:**
- **H1:**

## Preconditions and provenance

List SUT freeze, topology, model/tokenizer hashes, cache-state evidence, clocks, collectors, safety controls, and upstream snapshot.

## Factors, controls, and run order

Define independent variables, matched controls, randomization/counterbalancing, repetitions, warm-up, and stop rules.

## Procedure

1. Preflight and create immutable `run_id`.
2. Execute the declared state transition without reusing evidence from another state.
3. Record raw client, server, node, link, and environment channels.
4. Execute the workload and retain all attempts.
5. Finalize hashes and classify invalidations.

## Required measurements

Reference canonical metric IDs and schemas.

## Acceptance and regression rules

State absolute SLO, relative threshold, confidence rule, and hard-event rule. Missing data is `INSUFFICIENT_EVIDENCE`.

## Invalidation and abort rules

State predeclared invalidations separately from safety aborts. Keep invalid raw evidence.

## Outputs

- `manifest.json` against `schemas/run-manifest.schema.json`
- Raw `.jsonl` streams against applicable schemas
- Derived summary with code/version provenance

## Interpretation limits

State what this card does **not** prove.
