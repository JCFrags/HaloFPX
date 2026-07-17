# Reference repository clone receipt

Date: 2026-07-17

Reason: establish an organized, read-only source-code reference layer for reviewed HaloFPX fork planning.

| Action | From | To | Status | Notes |
|---|---|---|---|---|
| Clone without submodules | `https://github.com/charlie12345/ROCmFPX.git` | `sources/repositories/charlie12345__rocmfpx/` | Complete | Current `main` HEAD retained; Wiki pin is present in history. |
| Clone without submodules | `https://github.com/fewtarius/llama-ai.git` | `sources/repositories/fewtarius__llama-ai/` | Complete | Current `main` HEAD equals Wiki pin; CachyLLama gitlink remains uninitialized. |
| Clone without submodules | `https://github.com/fewtarius/CachyLLama.git` | `sources/repositories/fewtarius__cachyllama/` | Complete | Current `master` HEAD equals Wiki and llama-ai gitlink pin. |
| Clone without submodules | `https://github.com/ggml-org/llama.cpp.git` | `sources/repositories/ggml-org__llama.cpp/` | Complete | Current `master` HEAD retained; both recorded comparison pins are present in history. |

Verification:

- Source checked: each `origin` URL and remote default branch were resolved from the clone.
- Destination checked: four collision-free canonical directories exist under `sources/repositories/`.
- Identity checked: captured HEAD commit, commit date, and subject are recorded in `manifest.yaml`.
- Historical evidence checked: every Wiki pin and comparison pin resolves as a commit object in its corresponding clone.
- Cleanliness checked: `git status --porcelain=v1` returned no entry for each clone after capture.
- Submodules checked: none were initialized; llama-ai records the expected CachyLLama gitlink `6be745998f568e379ea197fcf827baec73ff9940`.
- License evidence checked: license-like tracked paths were enumerated at captured and pinned revisions; raw Git blobs were SHA-256 hashed. Hashes did not differ across the listed revisions for a repository.
- Execution boundary checked: repository scripts, binaries, builds, and tests were not run.

Known constraints:

- The clones are local snapshots, not immutable content-addressed archives. Do not fetch, pull, checkout, or edit them in place; create a new dated capture if freshness is required.
- `llama-ai` carries GPLv3 license text and declares its submodule using an SSH URL. Cross-repository reuse requires the fork plan's licensing gate; no license conclusion is implied by this receipt.

