# L05m offline bootstrap-material preparation review v01

- Date: 2026-07-18
- Scope: synthetic source ownership, manifest/object exactness, commitment
  independence, attempt fencing, witness ownership, isolation, and evidence
- Final verdict: **ACCEPT**

## Independent adversarial review

Contract review required fixed registry-to-material root admission, honest
digest-transitive manifest wording, numeric source limits, heap-bounded move-
only witnesses, permanent synthetic/concrete type separation, consume-first
authority invalidation, fixed provenance tags, full L04c frame validation, a
feasible five-commitment ownership model, and non-evicting terminal history.

Implementation review then found aggregate-limit subtraction underflow, a
moved-from anchor copy, missing independent C++ recomputation, incomplete proof
accessors, backend-only positive revalidation, an ineffective runtime-link
static check, post-positive exceptions misclassified as local exhaustion, and
an incomplete required fault/adversarial matrix. Each defect was corrected.

Final review directly verified the 15-case manifest matrix, source and readback
frame structural matrices, all response echoes and terminal outcomes, normal
fresh serialization, exact capacity/replay/last-slot behavior, post-positive
quarantine, and 28 before/after synthetic publication boundaries. It returned
ACCEPT with no remaining concrete blocker.

## Verification

| Gate | Result |
|---|---|
| Clean Windows CPU/WebUI-off Release build including `llama-server` | Pass |
| Full HaloFPX-labeled CTests | Pass, 31/31 |
| Focused inherited CTests | Pass, 7/7 |
| Authority/material repetitions | Pass, 200/200 |
| Static-contract repetitions | Pass, 200/200 |
| Independent golden repetitions | Pass, 200/200 |
| Independent adversarial review | ACCEPT after all blockers closed |

The independent vector covers six exact one-NUL domains, three no-NUL
provenance tags, 126 mutations, all five preparation commitments, and the
terminal-close commitment. Exact checker, vector, build, binary, and source
hashes are retained in the repeat receipt.

## Promotion boundary

L05m is permanently synthetic. It proves only wrapper/state-machine conformance
to an injected memory backend. No later concrete writer may relabel, deserialize,
convert, or accept its proof as storage durability or production-anchor
authority. A real backend requires a distinct accepted decision and sealed
proof type after filesystem, restart, fencing, recovery, and power-loss gates.
