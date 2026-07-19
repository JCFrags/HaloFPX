# Authority Debug baseline repair review v01

Status: **ACCEPT**

Scope: inherited test-authority repair required before opening L05r provider
implementation. This is not an L05r capability and changes no production
engine source or runtime default.

## Independent review

An independent adversarial reviewer inspected the complete diff, the authority
and authentication implementation, the test contract, and the retained
qualification evidence. The reviewer returned ACCEPT with no blocking finding.

The review confirmed:

- the named `manifest_key_domain` array removes the invalid cross-literal
  iterator range that depended on MSVC literal pooling;
- guaranteed-elision construction plus destination copy/move rebinding removes
  the fixture's optional-NRVO dependency and keeps all borrowed key and registry
  pointers owned by the destination fixture;
- `_set_error_mode(_OUT_TO_STDERR)` and `_set_abort_behavior` retain assertion
  text and a nonzero abort while suppressing ReportFault UI;
- the MSVC-only 8 MiB reserve applies only to the monolithic synthetic test
  executable, not any engine binary;
- the contract pins the source-shape invariants while runtime tests remain the
  behavioral authority; and
- no donor code, persistence path, feature enablement, deployment, or reference
  mutation entered this milestone.

The reviewer identified one nonblocking future improvement: add a small,
explicit behavioral probe for fixture move construction and copy assignment.
Existing copy-construction and move-assignment use, forced no-NRVO builds,
sanitizers, and repeated qualification are sufficient for this repair, so this
does not block acceptance.

## Evidence reviewed

The exact commands, hashes, counts, environment tuple, rollback-service state,
and nonclaims are retained in
`docs/halofpx/evidence/l05-precondition-authority-debug-repair-receipt.json`.

| Gate | Result |
|---|---:|
| Windows Debug focused runs | 10/10 |
| Windows Release focused runs | 20/20 |
| Windows Debug `/Zc:nrvo-` runs | 10/10 |
| Windows forced assertion stderr/nonzero/no-process probe | Pass |
| Windows Debug inherited suite | 84/84 |
| Windows Release inherited suite | 84/84 |
| nimo-1 Linux optimized authority runs | 550/550 |
| nimo-2 Linux ASan/UBSan authority runs | 120/120 |
| Reference clone identity and cleanliness | Pass, 4/4 |
| Known-good nimo-1 and nimo-2 services after qualification | Healthy |

## Provenance and rollback

The repair was derived only from the selected-base test and engine source. No
CachyLlama or GPL llama-ai code or documentation was copied. The direct
cherry-pick roster remains empty and no P3 donor record is needed.

Rollback is a test-only revert of the authority fixture/CRT changes, its CMake
stack reserve, its contract markers, and this evidence/review pair. No cache,
model, service configuration, persistent object, or deployment requires
migration.

## Nonclaims

This milestone does not qualify ROCm, HIP, Vulkan, WebUI, persistent writes,
cache hits, model inference, distributed recovery, or performance. Linux CPU
qualification here establishes portable test authority only; matched ROCm/HIP
Strix Halo control/candidate evidence remains required for production engine
milestones and all performance claims.
