# P01 primary-model qualification independent review

Date: 2026-07-19

Verdict: **ACCEPT early matched-control milestone; final G9/G10 remains open**

## Scope reviewed

The review independently inspected the P01 Markdown, machine-readable receipt,
README routing, local Git state, node-side raw evidence roots and bundles,
deployed-service state, and immutable reference status. It did not edit source
or expand the test matrix.

The evidence supports:

- the pinned model revision, 159873097824-byte size, and full SHA-256;
- the locked control and HaloFPX candidate commit/tree identities;
- matching control/candidate binary hashes across both nodes;
- the selected matched build and runtime tuple, including gfx1151 and the
  context-store canary disabled;
- 24 HTTP-200 timing responses, 20 retained samples, and exact 1129/128-token
  workload dimensions;
- independently recomputed means, sample deviations, point deltas, and the
  cautious Welch-interval characterization;
- deterministic completion and chat-output equivalence;
- evidence-bundle integrity, service rollback health, clean reference state,
  and the stated provenance boundaries.

## Required correction and closure

The initial record did not state that the published content digest was computed
through `jq -r`, whose output adds one trailing LF. The corrected record now
defines both representations exactly:

- raw UTF-8 JSON `.content` bytes without a terminator:
  `3c9bcc5624f4a1d6558c51be89e2443ffba4b2206d10b632c89b7ef64fdd026f`;
- `jq -r .content` bytes with one trailing LF:
  `a9c38c7f948adcfa8cfab5468ab84cc089b01a34c3f270f1c487a9a5fa74b555`.

The Markdown and receipt now agree and `git diff --check` reports no content
errors. The correction changes no result.

## Claim and promotion boundary

P01 is accepted only as an early matched-control qualification. HaloFPX has
slightly favorable point estimates, but both throughput confidence intervals
cross zero. The result is correctly described as no observed slowdown, not a
speedup and not final strict zero-regression closure. The generation-above-30
tok/s objective remains a stretch target rather than a pass/fail baseline.

The compact bundles do not retain every exact HIP flag field or both complete
candidate and control command lines. This is a nonblocking evidence-hardening
item for the next benchmark bundle, not a reason to rerun or reject P01.

No provenance, security, rollback, correctness, or claim-discipline blocker
remains for committing this documentation milestone.
