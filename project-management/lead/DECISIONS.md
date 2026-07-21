# Project-Lead Decisions

## 2026-07-20 — retain the current primary worker

Decision: retain the existing multiday HaloFPX task as primary implementation
owner.

Reason: despite its age, the worker uses current repository/node evidence,
obeys risk-proportionate testing steering, rejects slower code, restores the
known-good service, and is not repeating closed work. Context age alone does not
justify a disruptive handoff.

Trigger to revisit: contradictory state claims, repeated closed-lane work,
ignored steering, unsafe promotion, or three consecutive checks on the same
blocker without a materially new approach.

## 2026-07-20 — no P13 steering

Decision: do not intervene in P13.

Reason: the worker measured the micro-kernel gain, translated it to an estimated
whole-token contribution of only about 0.1%, and chose to close integration.
That is aligned with the project objective and prior speed steering.

Outcome: P13 committed cleanly as `ea49690a`. The default-off proof remains;
product integration is closed because the projected whole-token contribution is
only about 0.1%. No follow-up steering is required.

## 2026-07-20 — accept P14 rejection and L10 product pivot

Decision: no steering; accept the P14 row-split rejection and observe the L10
exact-key operational cache canary.

Reason: P14 used one bounded exact-output screen, found mixed/noise-scale prompt
and generation results with generation slightly worse, rejected the candidate,
restored production, and committed the evidence. L10 addresses a recorded
product gap rather than opening another marginal performance permutation: the
current cache is laboratory-only because clients must provide a manifest handle.
The proposed private authenticated exact-key lookup remains default-off,
generation-one, non-enumerating, non-prefix, and no-overwrite.

Trigger to revisit: L10 broadens into shared/prefix discovery, permits overwrite,
weakens authenticated fixed-anchor authority, mutates live state before complete
validation, or expands testing beyond the bounded canary without a defect.

## 2026-07-20 — accept L10a authenticated selection boundary

Decision: no steering; accept L10a and observe the L10b runtime canary.

Reason: the first attempt exposed a genuine positive-path failure and was held
open rather than promoted. The repaired seam authenticates and parses the fixed
anchor before revealing the selected manifest, rejects wrong scope and corrupt
anchors as misses, performs no directory scan, and has no server runtime edge.
Focused and inherited nimo-2 tests passed 4/4, independent review accepted the
milestone, the tree is clean at `975b1550`, and production recovered.

Trigger to revisit: normal-path writeback occurs before a clean prompt boundary,
restore mutates live state before full validation, misses fail to recompute cold,
or feature-off/default behavior changes.

## 2026-07-20 — accept L10b exact-session authority boundary

Decision: no steering; accept L10b and observe the separate L10c server canary.

Reason: L10b keeps request identity target-owned and opaque, derives its lineage
from authenticated private scope and exact canonical inputs, fails closed on
profile ambiguity, and remains library-only. Its 3/3 focused Linux tests and
inherited authentication/scope controls passed; independent review corrected
two defects before the clean `d7950c43` commit. Ordinary server behavior remains
unchanged because the runtime edge is deliberately deferred to L10c.

Trigger to revisit: L10c accepts caller-chosen cache identity, publishes before
a completed cold prompt boundary, mutates live state during validation, fails to
fall through cold on any cache error, changes feature-off behavior, or touches
the known-good deployment before its disposable canary passes.
