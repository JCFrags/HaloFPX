# HaloFPX Continuation Handoff

This is the shortest safe route into the combined HaloFPX repository. It records
the publication boundary, not a claim that every historical experiment is still
current.

## Current continuation layer — 2026-08-12

The immutable publication boundary below remains valid, but current work has
advanced beyond L111:

- the remote-`main` observation made during this correction is
  `e3e8b286b8316abd246fb155044effd11e60b0eb`; the exact source base named by
  the incident record is
  `b77f2bce6e7875ab065e09894f45915585c9f156`. These are dated repository
  boundaries, not live pointers. A fresh worker must record its checked-out
  `git rev-parse HEAD` and `git rev-parse origin/main` rather than assuming a
  SHA printed here remains the branch tip.
- the cache-saving behavior reference is pinned to
  `fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940`.
  This is a behavior/reference authority, not an assertion that the donor
  implementation or its complete feature set has been imported.
- [PR #23](https://github.com/JCFrags/HaloFPX/pull/23) merged as
  `aee627bd46de21327c9082f7915818430d38f453` and closed issue #14. It
  qualifies a default-off, exact-key, world-size-1/rank-0 restart lane with a
  Linux CPU model fixture. It does not qualify prefix reuse, multiple slots,
  recurrent state, two-rank coordination, or target performance.
- [PR #27](https://github.com/JCFrags/HaloFPX/pull/27) merged as
  `bf420e9f1db4ea4ba1d7c87771b6a4d662b5be67`. It adds an optional
  default-off OpenSSL EVP SHA-256 provider to the separate run-local
  `--cache-disk` prompt-cache path while retaining its full-file integrity
  gates. It has no matched end-to-end target result yet.
- [PR #30](https://github.com/JCFrags/HaloFPX/pull/30) merged as
  `7a36e01a25bd5c27b684b489d9996b4de3afa299`. Its default-off ROCmFPX
  MMVQ sum-free specialization is compile-qualified on both CachyOS Strix
  nodes; issue #25 remains open because no candidate GPU kernel, model parity,
  or matched performance run has been accepted.
- [PR #35](https://github.com/JCFrags/HaloFPX/pull/35) merged as
  `167df62ffc8970bc408d72e97ab71a57de4b69d2` and fixes the mixed
  sampled/raw logits row-count crash. The synchronization-reduction portion of
  issue #28 remains open.
- [PR #31](https://github.com/JCFrags/HaloFPX/pull/31) merged as
  `0ba18151438cb0e7279c7c8ae08e152f6f70145b` and closed issue #16 with
  the model-general frozen-plan, counterbalanced schedule, preflight, and raw
  evidence core. Issue #37 owns the separate safe CachyOS process adapter.
  Issues
  [#15](https://github.com/JCFrags/HaloFPX/issues/15),
  [#18](https://github.com/JCFrags/HaloFPX/issues/18),
  [#26](https://github.com/JCFrags/HaloFPX/issues/26),
  [#28](https://github.com/JCFrags/HaloFPX/issues/28),
  [#29](https://github.com/JCFrags/HaloFPX/issues/29),
  [#32](https://github.com/JCFrags/HaloFPX/issues/32), and
  [#33](https://github.com/JCFrags/HaloFPX/issues/33), plus
  [#37](https://github.com/JCFrags/HaloFPX/issues/37), remain open follow-ons.
- HaloFPX targets ROCmFPX-family GGUF inference across supported model
  architectures. MiniMax is the largest stress fixture, not a model-specific
  product target.
- both physical Strix Halo targets are Nimo MME3L machines running CachyOS.
  Ubuntu is a portability/control lane; Windows is the local control PC.
- the `2026-08-12T23:06:08Z` zero-restart
  [health receipt](docs/halofpx/evidence/2026-08-12-strix-halo-health-recheck/README.md)
  is historical before-state. A later target HMM/global-OOM incident restarted
  both ranks. The recovered nimo-1 coordinator was PID `3113343`, InvocationID
  `0656332b63a140eab7214627baa43253`, `NRestarts=1`; the recovered nimo-2
  worker was PID `2248760`, InvocationID
  `d15fe49610274e77bd9a3d84a0b791a5`, `NRestarts=1`. Both units were
  active/running, coordinator health was OK, and a real 5-prompt-token plus
  1-generated-token request completed. The
  [incident and recovery receipt](docs/halofpx/evidence/2026-08-12-target-hmm-oom-incident/README.md)
  supersedes current service identity and recovery state, but does not refresh
  executable/library hashes or establish a performance result.
  [Issue #41](https://github.com/JCFrags/HaloFPX/issues/41) is the P0
  target-ownership prerequisite for any target build, quantization,
  disposable inference, or benchmark.

Read [`project/TARGET_MACHINES.md`](project/TARGET_MACHINES.md), the current
blocks at the top of
[`project/project-management/lead/CURRENT_STATUS.md`](project/project-management/lead/CURRENT_STATUS.md)
and [`project/project-management/lead/DECISIONS.md`](project/project-management/lead/DECISIONS.md),
and [`project/PERFORMANCE_WORKPLAN.md`](project/PERFORMANCE_WORKPLAN.md) before
using `project/CURRENT_STATE.md` or the publication-era status text below as
current work authority.

## Read this first

1. Read [`AGENTS.md`](AGENTS.md) for the inherited implementation policy.
2. Read [`project/AGENTS.md`](project/AGENTS.md) for HaloFPX evidence rules.
3. Complete [`project/WORKER_START_HERE.md`](project/WORKER_START_HERE.md).
4. Read the Project Lead records at your checked-out commit:
   [`CURRENT_STATUS.md`](project/project-management/lead/CURRENT_STATUS.md) and
   [`DECISIONS.md`](project/project-management/lead/DECISIONS.md).
5. Use [`ARTIFACTS.md`](ARTIFACTS.md) before restoring any release payload.

Claim labels in this file have their project meanings: `[MEASURED]` is scoped to
the recorded environment, `[VERIFIED]` requires exact evidence, and `[OPEN]`
remains unresolved.

## Repository boundary

[VERIFIED] The publication integration commit is
`728c3b441fcb38a9eb55272ed673da9d2d18c173`. It preserves both source histories
without squashing:

| Boundary | Exact commit | Location after integration |
|---|---|---|
| HaloFPX implementation and implementation-local evidence | `620ef60aa446990335ef46c7d76738f797e62f8f` | repository root |
| Engineering wiki, research, governance, and imported evidence | `b1c2d8aef707fb03920fc189ccd26395fa61879d` | `project/` |

The integration commit has those two commits as its parents. At that boundary,
the `project/` tree exactly equals the engineering-wiki commit tree. Later
additive publication or governance commits may intentionally change that tree.

Verify the preserved boundary from any clone:

```powershell
git fsck --full
git merge-base --is-ancestor 620ef60aa446990335ef46c7d76738f797e62f8f main
git merge-base --is-ancestor b1c2d8aef707fb03920fc189ccd26395fa61879d main
git show -s --format="%H %P" 728c3b441fcb38a9eb55272ed673da9d2d18c173
git diff --exit-code b1c2d8aef707fb03920fc189ccd26395fa61879d 728c3b441fcb38a9eb55272ed673da9d2d18c173:project
```

An exit code of zero from both ancestry checks and the tree comparison is the
expected result.

## Where material lives

| Need | Canonical route |
|---|---|
| Project goal and non-negotiable boundaries | [`project/PROJECT_GOAL.md`](project/PROJECT_GOAL.md) |
| Publication-era routing summary (historical where superseded) | [`project/CURRENT_STATE.md`](project/CURRENT_STATE.md) |
| Current physical target and fresh-PC access boundary | [`project/TARGET_MACHINES.md`](project/TARGET_MACHINES.md) |
| Active cache/prompt/generation work plan | [`project/PERFORMANCE_WORKPLAN.md`](project/PERFORMANCE_WORKPLAN.md) |
| Current accepted authority | [`project/project-management/lead/`](project/project-management/lead/) |
| Complete engineering wiki | [`project/wiki/HaloFPX_Wiki/`](project/wiki/HaloFPX_Wiki/) |
| Research prompts and section registry | [`project/research/prompts/`](project/research/prompts/) |
| Raw/imported documentation evidence | [`project/sources/`](project/sources/) |
| Implementation source | repository root, especially `ggml/`, `src/`, `include/`, and `tests/` |
| Implementation decisions and receipts | [`docs/halofpx/`](docs/halofpx/) |
| Formal models and runners | [`formal/`](formal/) and project-linked release artifacts |
| Non-Git payload policy and recovery | [`ARTIFACTS.md`](ARTIFACTS.md) |
| Machine-readable publication record | [`docs/publication/manifest.json`](docs/publication/manifest.json) |

The wiki under `project/wiki/HaloFPX_Wiki/` is canonical. A GitHub Wiki mirror,
if one is added later, is a convenience copy and must not become a second
authority.

## Status at the publication boundary

- [VERIFIED] Implementation commit
  `620ef60aa446990335ef46c7d76738f797e62f8f` contains the retained L111
  loader-foundation candidate and is a direct child of
  `6c88472bf5f567a1064f27f4d8a90fc8e2b47a02`.
- [VERIFIED] The independent L111 exact-diff review returned `PASS / RETAIN`
  with no P0 or P1 findings. Release and Debug focused tests each passed 1/1,
  and the feature-off static `llama` compile passed. The exact scope and
  limitations are in [`docs/halofpx/evidence/l111/`](docs/halofpx/evidence/l111/).
- [VERIFIED] At creation of the two-parent publication merge, the historical
  Lead records had not formally accepted commit `620ef60...`; the attempted
  terminal report delivery was rejected because its task was unreachable. The
  later additive Lead
  [decision](project/project-management/lead/DECISIONS.md#2026-08-12--accept-the-bounded-l111-loader-foundation)
  accepts that exact commit as `PASS / RETAIN` at the bounded loader-foundation
  boundary. Its additive
  [reconciliation note](docs/halofpx/evidence/l111/RECONCILIATION.md) preserves
  the distinction between the older report-delivery failure and the accepted
  technical result.
- [VERIFIED] L111 is only a bounded loader foundation. It does not establish
  graph wiring, scheduling, asynchronous remote procedure calls, a model run,
  production fitness, or a performance improvement.
- [MEASURED] The latest retained recovery observation followed the later
  2026-08-12 target HMM/global-OOM incident. Nimo-1 recovered as PID `3113343`,
  InvocationID `0656332b63a140eab7214627baa43253`, `NRestarts=1`; nimo-2
  recovered as PID `2248760`, InvocationID
  `d15fe49610274e77bd9a3d84a0b791a5`, `NRestarts=1`. Both units were
  active/running, coordinator health was OK, and a real 5-prompt-token plus
  1-generated-token request completed. See the
  [incident receipt](docs/halofpx/evidence/2026-08-12-target-hmm-oom-incident/README.md).
  The receipt did not re-audit executable/library hashes, packages, model
  identity, unit contents, or performance.
- [OPEN] No accepted end-user two-node persistent-cache product or accepted
  full-model speed improvement exists in the imported authority.

## First clean-clone checks

Run these from the repository root. Use a disposable build directory and do
not point tests at production services or model storage.

```powershell
git status --short
git fsck --full
python project/research/prompts/tools/generate_wiki_manifest.py project/wiki/HaloFPX_Wiki --check
python project/research/prompts/tools/validate_wiki.py project/wiki/HaloFPX_Wiki
python -m unittest discover -s project/research/prompts/tools -p "test_validate_wiki.py"
python project/project-management/documentation/validate_documentation.py
```

The wiki tools require PyYAML. Install it in a virtual environment rather than
changing system Python. The validation workflow uses Python `3.12` and
`PyYAML==6.0.3`. The documentation validator is monorepo-aware and requires the
complete imported history to verify both ancestors, the two-parent integration
tree, and the protected snapshot. New clones are full-history by default. If
`git rev-parse --is-shallow-repository` returns `true`, run
`git fetch --unshallow --tags origin`; otherwise use `git fetch --tags origin`.
Investigate a provenance failure; never weaken the check merely to make it pass.

[MEASURED] Immediately before publication work, the source documentation tree
passed its manifest check, all 86 wiki sections passed structural and schema
validation, all four focused validator unit tests passed, and the documentation
validator passed 554 Markdown files, 12 category manifests, and zero broken
internal links. A separate syntax sweep parsed 447 JSON and 268 YAML files and
reported all governed schemas passing. Those results are scoped to the
pre-publication working tree and must not substitute for the clean-clone run.

The recorded L111 build shape can be reproduced on a compatible Linux
toolchain as follows:

```bash
cmake -S . -B build-l111-release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=ON \
  -DGGML_RPC=ON \
  -DGGML_RPC_HALOFPX_LOCAL_STATE=ON \
  -DLLAMA_BUILD_TESTS=ON
cmake --build build-l111-release --target test-halofpx-loader-partition
ctest --test-dir build-l111-release -R '^test-halofpx-loader-partition$' --output-on-failure

cmake -S . -B build-l111-feature-off -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=OFF \
  -DGGML_RPC=OFF \
  -DGGML_RPC_HALOFPX_LOCAL_STATE=OFF \
  -DLLAMA_BUILD_TESTS=OFF
cmake --build build-l111-feature-off --target llama
```

These commands are a recommended reproduction shape, not a promise of parity
on a different compiler or platform. The retained receipt used CMake 4.3.4,
Ninja 1.13.2, and GCC 16.1.1 20260625. Compare results and hashes with
[`build-receipt.txt`](docs/halofpx/evidence/l111/build-receipt.txt).

The initial publication validation and Windows feature-off compile are recorded
in [`docs/publication/validation-2026-08-12.md`](docs/publication/validation-2026-08-12.md).
The final immutable release identity, artifact reconciliation, and GitHub
attestation are recorded in
[`docs/publication/release-verification-2026-08-12.md`](docs/publication/release-verification-2026-08-12.md).

## Safe continuation boundary

[RECOMMENDATION] Keep issue #25 open and the merged PR #30 feature default-off
until GPU correctness, model parity, and matched target evidence complete.
Route cache attribution, model-general prefill, the CachyOS A/B adapter,
two-rank restart coordination, sampling synchronization, FFN conversion reuse,
verified longest-prefix reuse, and live-derived compatibility through issues
#18, #15, #37, #26, #28, #29, #32, and #33 respectively. Do not infer
authorization for a production transition or broader cache-product claim from
the L111, PR #23, or compile-only PR #30 acceptance.

For any new claim:

1. Link the requirement or accepted decision.
2. Preserve exact source commits, tool versions, hardware identity at the
   appropriate privacy level, inputs, raw output, and cleanup receipts.
3. Keep feature-off behavior unchanged and test it explicitly.
4. State rank ownership, failure behavior, and single-node fallback for
   distributed work.
5. Make corruption a cache miss or recomputation, never accepted invalid state.
6. Compare matched configurations before making a performance claim.

## Publication and privacy boundary

[VERIFIED] The destination repository was made private for initial publication
because the evidence set includes machine identifiers and material with
unresolved redistribution boundaries. The implementation source has an MIT
license; the engineering-wiki repository does not have a blanket project
license, and imported sources retain their own terms.

[VERIFIED] The published private `evidence-2026-08-12` preservation release is
immutable and contains 41 files totaling `23317868085` bytes. Every remote
name, size, upload state, and GitHub-reported SHA-256 matched the locally
rehashed set; all 39 payloads also match
[`release-manifest.json`](docs/publication/release-manifest.json). The release
attestation and final tag identity passed verification. Interpret and restore
the assets with
[`asset-provenance.json`](docs/publication/asset-provenance.json) and
[`ARTIFACTS.md`](ARTIFACTS.md); use the
[`final publication receipt`](docs/publication/release-verification-2026-08-12.md)
for the frozen boundary.

[RECOMMENDATION] Keep the repository and release assets private until a fresh
privacy review, credential scan, third-party notice audit, and explicit license
decision authorize a public subset. A pre-publication text/history scan found
no credible live credentials, but that result does not prove that every opaque
binary archive is safe for public redistribution.

## Resources that a clone cannot contain

The primary model was not present in either source repository or the publication
workspace. Only its identity was available:

- size: `159873097824` bytes;
- SHA-256: `96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.

[OPEN] A successor must obtain those exact bytes through an independently
authorized source and verify both size and digest. Never substitute a model
with the same display name or a `latest` alias.

Large local evidence and source archives do not belong in ordinary Git history.
They are handled as private release assets, with oversized files split into
ordered parts and accompanied by checksums. See [`ARTIFACTS.md`](ARTIFACTS.md)
for the exact trust and restore procedure.
