---
review_id: HALOFPX-L08A-REVIEW-20260719-V01
date: 2026-07-19
lane: L08a
verdict: ACCEPT_L08A_LABORATORY_ONLY
---

# L08a direct-session canary independent adversarial review

## Verdict

**Accept for committing L08a as a restricted disposable-root laboratory
milestone.** This verdict does not promote L08 or production persistence.
Protected anchor/generation, replay authority, and publication reconciliation
remain mandatory before either claim can open.

## Review history and closed findings

The first pass returned `REVISE` for two focused issues:

1. entry/quota capacity rejection occurred before an existing session was
   classified, which could mask an equal retry or unequal collision; and
2. the sealed compatibility record stated context 256 while the tiny model's
   observed effective slot context was 128.

The corrected provider authenticates and classifies an existing destination
before entry, quota, or reserve rejection. Its full-capacity contract now proves
equal retry returns `already_exists`, unequal retry returns `conflict`, and a new
session returns `quota_exceeded`. The final qualification explicitly requests
context 128, observes slot `n_ctx = 128`, and binds context 128 into
compatibility root
`fb420ed96d9fac65e76e8db9c4836a4da0ca529320965409e55d7862635444f3`.

## Accepted evidence

The v3 source hashes matched the reviewed local files. The sanitized bundle is
`/var/tmp/halofpx-l08a-canary-evidence-20260719-v3/halofpx-l08a-canary-nimo1-20260719.tar.zst`,
7,507 bytes, SHA-256
`fdef5e5e2f61f6975341db18b913b41af23f2305fcb937f0c49dc056f7605409`.
Its sealed manifest covers the retained files and excludes authority keys and
store contents.

The reviewed qualification passed:

- focused contracts: 5/5;
- real server canary: 1/1;
- inherited slot save/restore smoke: 1/1;
- feature-off and L02 controls; and
- feature-off help exposure: zero canary matches.

## Security, provenance, and rollback assessment

The exact ADR-0002 namespace preimage is implemented and golden-tested.
Parser, tool, reasoning, grammar, adapter, speculative/MTP, recurrent, and
non-greedy state remain outside the codec admission. Corruption is a miss, and
provider/accounting rejection preserves healthy cold inference. Unequal
existing-session content is neither accepted nor overwritten.

No donor implementation, GPL llama-ai code, CachyLLama code, new dependency,
remote, WebUI, release, or deployment surface was introduced. Build-time and
runtime defaults remain off, so reverting the canary-only paths restores the
feature-off control without a storage migration.

The direct laboratory format still lacks the protected anchor, generation,
replay high-water mark, and post-rename reconciliation required by the frozen
publication authority. Those are accurate documented nonclaims and are the
next safety gate, not a reason to reject this narrowly labeled L08a milestone.
