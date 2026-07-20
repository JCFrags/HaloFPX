# L08b protected-session canary independent review

Date: 2026-07-20

Verdict: **ACCEPT FOR GENERATION-ONE LABORATORY CANARY ONLY.**

The first pass found one P2 in the `renameat2(RENAME_NOREPLACE)` collision
branch. An exact anchor observed after `EEXIST` could return idempotent success
without a fresh post-collision direct-manifest authentication and complete
receipt comparison. That was narrower than ADR-0028's equal-retry contract.

The corrected path reopens and authenticates the direct manifest and requires
complete receipt equality before `already-exists`. An exact anchor paired with
different or unauthenticated direct material is `lineage-quarantined`; a
nonexact anchor is `conflict`. Focused re-review accepted the correction with
no remaining P1 or P2 blocker.

The product link graph is clean. The protected server path links only the Linux
direct provider and canary-owned anchor codec through the existing target-owned
authentication/format/SHA chain. It does not link the excluded L05 anchor
codec, publication coordinator, bootstrap, registry, simulator, or synthetic
backend targets. Feature-off compile guards, explicit runtime authority, key
lifetime, cold degradation, full-envelope equality, reconciliation, and
rollback are coherent with ADR-0028.

This acceptance covers only the default-off generation-one disposable canary.
It does not promote full ADR-0004, L08, production persistence, generation
advancement, distributed state, or final zero-regression.
