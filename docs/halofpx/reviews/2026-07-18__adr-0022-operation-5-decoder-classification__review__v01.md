# ADR-0022 independent adversarial review

- Date: 2026-07-18
- Decision reviewed: `docs/halofpx/decisions/0022-portable-registry-lab-operation-5-decoder-classification.md`
- Accepted decision SHA-256: `d755a64ed132f52f07258de13d6420391d0edebf9eee30e40bb4c8dfc86ce854`
- Result: **ACCEPT**

## Scope

The review compared ADR-0022 with ADRs 0018 through 0021, the normative
`context-store-registry-lab-v1.cddl`, the selected target-native verifier and
wire boundaries, and the project provenance, feature-off, and persistent-write
gates. It examined executable semantics, classification precedence, authority
scope, hostile-state qualification, decoder reachability, and donor isolation.

## Blocking findings and corrections

The first review rejected the draft on three substantive points:

1. The history binder did not exactly anchor zero-CLOSE current HEAD or a
   one-CLOSE PREPARE to the initialized marker's initial-HEAD digest.
2. It required a private QUARANTINE decoder and hostile-byte tests even though
   quarantine namespace presence must short-circuit before content decoding.
3. An unscoped duplicate-successor rule would poison a legitimate unique-
   attempt retry after a terminal ABORT.

The accepted revision now:

- binds zero/one-CLOSE histories through marker key 15 and PREPARE keys 15/16,
  with explicit current-HEAD, CLOSE, ABORT, and negative test requirements;
- treats `QUARANTINE` and `QUARANTINE.tmp` solely by live-or-durable namespace
  presence in operation 5 and removes the unreachable decoder claim; and
- admits same-transition retry after authenticated terminal ABORT history while
  keeping competing unresolved/CLOSE branches, duplicate attempt IDs, and
  pre-existing successor material fail-closed.

## Acceptance

The independent re-review found no remaining contradiction with ADRs 0018
through 0021 or the CDDL, impossible qualification requirement,
licensing/provenance risk, public-surface expansion, persistent-write
authority, or runtime/cache authority broadening.

Acceptance authorizes only implementation of the portable fake-only read-only
operation 5 inside the existing excluded target. Operation 6, recovery or
quarantine mutation, Linux I/O, persistent writes, concrete authority, runtime
linkage, and cache hits remain closed.
