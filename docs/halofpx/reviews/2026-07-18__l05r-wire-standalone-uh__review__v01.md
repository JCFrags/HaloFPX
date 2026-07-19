# L05r standalone-UH wire admission review v01

- Date: 2026-07-18
- Decision authority: ADR-0024 and canonical Wiki Section 63
- Verdict: **ACCEPT** for the excluded fake wire layer only

## Outcome

The wire layer can now admit a true UH quarantine shape from a typed
predecessor-HEAD value and the exact authenticated HEAD bytes without
fabricating a PREPARE transition. The new pointer is the final member of the
existing evidence structure, preserving every prior member offset and existing
aggregate initializer. Wire bytes and the accepted quarantine golden vector do
not change.

The standalone route is mutually exclusive with a transition and admits only
an unattributable, phase-zero, no-previous-record value with one predecessor
HEAD digest. It maps the typed HEAD into the full predecessor-envelope
expectation, checks exact scope and digest equality, and then delegates raw
envelope and HEAD authentication to the accepted lifecycle witness. Successor
or PREPARE evidence, including pointer-only and size-only forms, is rejected.

## Independent adversarial review

Independent read-only review required the new descriptor to be appended rather
than inserted into the v1 structure and requested exact-byte, scope, descriptor,
credential, malformed-evidence, and invalid-shape cases. Both corrections were
applied before promotion. The review also confirmed that this seam does not
prove the initialized ROOT required by ADR-0024; that proof remains an engine
precondition before operation 69.

## Qualification

Windows Debug focused tests passed, and the Windows Release HaloFPX label
passed 40/40. Nimo-1 passed the complete 39-test Linux optimized label; nimo-2
passed the same 39 tests under ASan/UBSan. Exact focused executables then passed
50 Debug and 100 Release repetitions on Windows plus 100 optimized repetitions
on nimo-1 and 100 sanitizer repetitions on nimo-2.

Clean Linux qualification exposed three inherited test/reproducibility defects:
a missing `<algorithm>` dependency, a line-ending-sensitive reviewed CDDL hash,
and a key span copied from a temporary fixture. Each received a separate,
minimal test-only commit and all complete matrices passed afterward. A discarded
empty-set CTest loop is explicitly excluded from repeat counts.

## Boundaries and rollback

The target remains `STATIC EXCLUDE_FROM_ALL`; WebUI stayed disabled. No public
header, server link, filesystem adapter, persistent write, provider, cache,
restore, inference, or performance path changed. No GPL llama-ai or CachyLlama
implementation entered the MIT engine. Rollback is removal of this trailing
evidence member, standalone admission branch, and its tests.

The next safe slice is operation-5 complete-scan diagnosis and deterministic
reason/shape selection. It must use this UH route without a transition and must
independently prove the authenticated initialized ROOT before any operation-69
authority can exist.
