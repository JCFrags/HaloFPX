# L05o registry-lab wire validator review v02

- Date: 2026-07-18
- Scope: target-native, no-I/O authenticated and semantic validation of the
  ADR-0018 registry-lab wire objects
- Final verdict: **ACCEPT**

## Review history

The first implementation review returned `REVISE` because Release assertions
could be compiled out, HEAD and selector validation was tied to the golden
fixture's `H=40` and generations 1/2, expectation lengths were unsafe, the
quarantine cross-fields were incomplete, credential ownership was copyable,
and the target inherited the full bootstrap-consumption dependency.

After those repairs, the second review returned `REVISE` because every object
kind still required a fabricated future transaction, root nonzero fields were
not independently enforced, the product-tail CMake check did not cross line
boundaries, and most C++ mutations stopped at HMAC failure.

The accepted revision validates expectations by lifecycle object kind,
independently authenticates the exact registry envelope selected by HEAD,
enforces the root nonzero fields, narrows linkage to the successor codec, and
uses a real product-tail extraction with a synthetic detector self-test. The
C++ suite now includes recomputed-tag semantics, lifecycle-order verification,
arbitrary `H=123` and selector generations 7/8, independently valid wrong HEAD,
a fully authenticated and repaired `H+2` transition, wrong operation binding,
terminal recovery classes, attributable and unattributable quarantine, and
credential clear/move/destructor cases. Independent rereview returned ACCEPT.

## Verification

| Gate | Result |
|---|---|
| Windows CPU Release target build | Pass |
| Focused L05o CTests | Pass, 3/3 |
| Full configured CTests | Pass, 80/80 |
| HaloFPX-labeled CTests | Pass, 36/36 |
| Wire-validator process repetitions | Pass, 200/200 |
| Static isolation-contract repetitions | Pass, 200/200 |
| Independent golden-oracle repetitions | Pass, 200/200 |
| Golden hostile checks per oracle run | 3,260 |
| Independent adversarial rereview | ACCEPT after all blockers closed |

All four immutable reference repositories were clean and remained at their
locked commits after qualification. No CachyLLama or GPL llama-ai code was
introduced.

## Promotion boundary

`authenticated_semantic_only` is non-authoritative. This milestone performs no
filesystem operation, creates no protected state, grants no cache hit, and
claims no persistence or durability. A future backend may admit a successor
HEAD only together with a successfully verified PREPARE/transition and exact
byte comparison. The target remains `STATIC EXCLUDE_FROM_ALL` and absent from
the server/product graph.
