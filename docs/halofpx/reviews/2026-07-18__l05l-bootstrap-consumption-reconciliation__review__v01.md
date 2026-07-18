# L05l bootstrap-consumption reconciliation review v01

- Date: 2026-07-18
- Scope: ambiguity fencing, exact-head classification, commitment independence,
  concurrency, quarantine, proof ownership, default-off isolation, and evidence
- Final verdict: **ACCEPT**

## Independent adversarial review

The first review returned REVISE. Production behavior matched ADR-0015, but
promotion evidence did not independently detect a common-mode commitment bug,
did not race two fresh reconciliation attempts, did not exercise callback
reentry into `quarantined()`, and omitted material binding, hostile-witness,
and backend-outcome cases.

The corrected suite freezes an independent 956-byte Python preimage and exact
SHA-256 result, mutates the domain and every ordered field, and also implements
the serializer independently in C++ for comparison at every backend callback.
It proves exactly one callback across two fresh concurrent attempts, callback
reentry without deadlock, exact truncation/bit/oversize/contradiction behavior,
all seven non-present outcomes, all five response confirmations, wrong root and
predecessor, and materially different command, token, plan, manifest, anchor,
and material plans. The `root_quarantined` terminal result is reachable.

Final re-review found the production state machine, exact witness checks,
one-shot attempt retention, sticky quarantine, and move-only proof conformant
and returned ACCEPT.

## Verification

| Gate | Result |
|---|---|
| Clean Windows CPU/WebUI-off Release build including `llama-server` | Pass |
| Full HaloFPX-labeled CTests | Pass, 29/29 |
| Focused inherited CTests | Pass, 7/7 |
| Authority/reconciliation repetitions | Pass, 200/200 |
| Registry-successor golden repetitions | Pass, 200/200 |
| Independent reconciliation-golden repetitions | Pass, 200/200 |
| Independent adversarial review | ACCEPT after evidence corrections |

The independent vector fixes a 956-byte preimage and commitment
`649f3b5dd71f2318c03a9ab7bc8d09318e8362673681446db7a1343a3ea333bf`.
Its checker and vector hashes, build tuple, binaries, and reviewed-source hashes
are retained in the repeat receipt.

## Promotion boundary

L05l qualifies only the excluded in-memory reconciliation contract. It does
not qualify real durability, restart recovery, cross-process fencing, registry
key custody, durable material preparation, protected-anchor access, bootstrap
execution, cache admission, persistent writes, server behavior, or either node.
Exact-predecessor reconciliation deliberately grants no retry capability.
