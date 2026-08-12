# HaloFPX Continuation Handoff

This is the shortest safe route into the combined HaloFPX repository. It records
the publication boundary, not a claim that every historical experiment is still
current.

## Current continuation layer — 2026-08-12

The immutable publication boundary below remains valid, but current work has
advanced beyond L111:

- `main` at the start of this update was
  `4a156395db62604cf37e27e6459e3ee0e3949c48` after PRs #12, #17, and #20.
- issue #5 is closed; PR #20 made the automatic run-local SSD spill cache
  reject same-size corruption. It did not create restart persistence.
- issue #14 is the active restart-cache correctness boundary. Cache metrics,
  cold prompt processing, and generation are tracked separately.
- HaloFPX targets ROCmFPX-family GGUF inference across supported model
  architectures. MiniMax is the largest stress fixture, not a model-specific
  product target.
- both physical Strix Halo targets are Nimo MME3L machines running CachyOS.
  Ubuntu is a portability/control lane; Windows is the local control PC.

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
- [MEASURED] The last production observation in the imported Lead record was
  made on 2026-07-29. It is historical evidence, not current health. Re-observe
  under current authority before making any production statement or mutation.
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

[RECOMMENDATION] The next implementation action is a separately authorized,
narrowly specified milestone after reading the reconciled Lead authority. Do
not infer authorization for graph, scheduler, transport, model, runtime, or
production work from the L111 acceptance.

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
