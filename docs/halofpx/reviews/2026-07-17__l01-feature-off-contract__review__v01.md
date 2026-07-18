---
type: implementation-milestone-review
status: accept-with-later-gates-open
date: 2026-07-17
lane: L01
source_commit: 61f2f2d7bc4955e9bca821095ef69125837133b5
source_tree: 0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd
---

# L01 feature-off contract review

## Verdict

**Accept as the first safe local implementation milestone.** The change records
the restored clean baseline and adds one executable characterization test. It
does not alter the runtime, add a provider, import donor code, create a remote,
or add a persistent read/write path.

L02 state, scope, format, and threat-model decisions remain open. This review
does not authorize L03 or later lanes.

## Evidence

| Check | Result |
|---|---|
| Locked bundle SHA-256 | Match: `bcbe6cf910f4dd183d8ad96ea0d936ac85bd636a1cfe570179599d3fc5e307fa` |
| Restored commit/tree | Match: `61f2f2d7...` / `0a35143f...` |
| Requested branch | `codex/integration-base-61f2f2d` |
| Configured remotes | None |
| Local CPU Release build | Pass with `LLAMA_BUILD_WEBUI=OFF` |
| L01 contract CTest | Post-change `build/halofpx-milestone` registry selected exactly one test; pass, 1/1 |
| Focused inherited CTests | Pass, 7/7 including the model fixture dependency |
| Pre/post `llama-server --help` | Identical text |
| `git diff --check` | Pass |
| Donor/persistence markers in implementation diff | None |

The pre/post server executable hashes differ because the milestone build embeds
dirty/build identity, so binary equality is not claimed. The exposed help
contract is identical, and the new change contains no runtime source.

The clean-source `build/halofpx-baseline` registry predates the L01 test. Its
manifested hashes remain immutable baseline evidence. L01 test execution uses a
separately configured post-change `build/halofpx-milestone` tree; the test was
enumerated before execution to prevent CTest's zero-selected-tests success code
from being misreported as a pass.

## Wiki reconciliation

- Section 11 requires an offline clean checkout, exact object identity, no moving
  branch authority, and preserved bundle evidence. The restore meets that local
  portion; release qualification remains open.
- Section 12 locates server/API behavior in `tools/server` and treats cache,
  scheduler, backend, and state boundaries separately. The test observes existing
  routes and defaults without coupling or changing those modules.
- Section 13 requires ROCmFPX formats, TurboQuant, backend behavior, and the
  selected pin to remain visible. The focused suite retains TurboQuant and CPU
  backend-op coverage; no broader quality or target-backend claim is made.
- Section 14 and the reviewed license boundary prohibit wholesale donor import.
  No CachyLLama or GPL llama-ai material was copied or adapted.
- Section 15 requires small, buildable, bisectable lanes and feature-off
  preservation. L01 is test/documentation only and stops before the open L02
  contract decisions.
- Section 16 requires exact source/build/test/provenance records and human review
  ownership. The baseline manifest records the available local identities and
  hashes; no SBOM, release reproducibility, or legal-review claim is made.
- Section 82 orders evidence freeze before later product and persistence work.
  This milestone does not bypass platform, single-node, cache, or release gates.

Canonical Wiki authority remains in
`C:\Users\britt\Documents\Custom_Inference_Project\wiki\HaloFPX_Wiki`.

## Remaining genuine gates

1. L02 cannot close until `OPEN-STATE-01`, `OPEN-SCOPE-01`, and `OPEN-FMT-01`
   receive approved decisions.
2. Donor-derived capability work remains blocked on capability-level P3
   provenance/license disposition.
3. Persistent writes remain blocked on format, state completeness, scope,
   corruption/crash, storage reserve/quota, rollback, and acceptance gates.
4. Full G3 requires matched post-change CLI/API/cache/backend evidence on the
   approved target matrix; this local characterization test is not sufficient.

These are expected phase boundaries, not defects in L01.
