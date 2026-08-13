# HaloFPX full recovery receipt — SAME-PC DISPOSABLE REHEARSAL

Status: **PASS for the bounded same-PC rehearsal. GitHub issue #2 remains OPEN and is not satisfied.**

This receipt covers a disposable recovery run created at
`C:\Users\britt\Documents\HaloFPX_FullRecovery_Rehearsal_20260812` on the
existing Windows 11 + WSL2 workstation. The directory was absent before it was
created. Nothing was overlaid or cleaned up. No target machine or SSH access was
used, and no credential or token was retained.

The run proves that the project code and published continuation resources used
in this lane can be recovered from GitHub on this same PC. It does **not** prove
never-used-PC continuation, AMD Strix Halo compatibility, HIP/Vulkan behavior,
model quality, performance, single-node operation, or dual-node operation.

## Outcome

| Gate | Result |
| --- | --- |
| Exact PR #56 source clone | PASS |
| Full history, tags, clean checkout, `git fsck --full` | PASS |
| Four immutable GitHub releases / API identities / attestations | PASS |
| All 52 release assets / 24,696,192,820 bytes | PASS |
| Original publication verifier | PASS |
| Two manifest-ordered split reconstructions | PASS |
| Legacy, donor, and thin FFN Git bundle recovery | PASS |
| Safe FFN archive inspection and fresh-root extraction | PASS |
| Q3/Q6/Q8 hash, census, and real CPU four-token smoke | PASS |
| Wiki, documentation, fixture, A/B, and recovery validators | PASS |
| Fresh-clone manifest byte-identity regression | PASS |
| Current PR #56 feature-OFF static build | PASS (compile-only) |
| Never-used-PC acceptance in issue #2 | OPEN / not satisfied |

Run start: `2026-08-13T06:14:21.0672173Z` (`2026-08-12` PDT)  
Receipt completion snapshot: `2026-08-13T06:59:40.7183462Z`

## Source authority

- Repository: private `JCFrags/HaloFPX`, repository ID `1332159679`, owner ID
  `222912166`.
- Candidate: draft PR #56, exact detached commit
  `bdd87e2e29d56d1e9d7fc302c08d7f6c170fa519`.
- Origin: `https://github.com/JCFrags/HaloFPX.git`.
- `origin/main` at the terminal check:
  `9bfccf25d43af0c446df591035e9cdac0b74d6c0`.
- Clone: non-shallow, status porcelain byte count `0`, full object check exit `0`.
- Continuation registry:
  `docs/publication/continuation-releases.json`, SHA-256
  `93d4874d8a836554c3da94442dff08b33ebb175a2ce4c16a8437546a7e015a96`.
- Windows Git owns the fresh-clone check. WSL Git owns only the fixture-linked
  worktree, whose `.git` pointer intentionally contains a Linux path.

The terminal authority receipt is
`raw/19c-terminal-git-fsck-clean-attempt3.txt`. Earlier failed receipt attempts
remain preserved and are classified below.

## GitHub release acquisition

Strict registry, repository, release state, API asset identity, size, digest,
tag object, peeled commit, and GitHub release-attestation checks passed for all
four exact tags:

| Tag | Release ID | Assets | Bytes | Peeled commit |
| --- | ---: | ---: | ---: | --- |
| `evidence-2026-08-12` | 369369453 | 41 | 23,317,868,085 | `7c801894062c2e09122c18447da66d50da60c050` |
| `evidence-ffn-q8-reuse-3402aa7-2026-08-12` | 369641490 | 1 | 40,697 | `7e68d8a2eaa36a5a115ca2736f6bfca66ee4770f` |
| `evidence-ffn-q8-reuse-source-bundle-3402aa7-2026-08-12` | 369647794 | 1 | 13,082 | `0db715c6e436be88a4d5444763421020f53dc728` |
| `fixture-qwen3-0.6b-rocmfpx-pure-v1` | 369653983 | 9 | 1,378,270,956 | `44f4ce5b29986e3c0bb6cd8f00238e09d24c00f3` |

Every asset was downloaded once into its exact tag-scoped directory under
`downloads/`. Exact directory membership, all 52 sizes, and all 52 SHA-256
digests passed. The downloaded total is `24,696,192,820` bytes.

The original tracked `scripts/verify-publication-assets.ps1` reported:

> Verified 39 assets, 2 split payloads, exact directory membership, and both
> trusted control files.

The fresh-clone regression separately passed the intentional distinction between
the LF tracked manifest and immutable released CRLF bytes. Its explicit exit
receipt is `raw/validation/publication-manifest-identity.exit.txt` (`0`).

## Split reconstruction and Git recovery

Both split payloads were reconstructed into a new `recovered/split-payloads/`
root in exact manifest order, with every part checked before streaming:

- `l24-source-v2.tar`: 10 parts, `17,101,714,432` bytes, SHA-256
  `5920dbdb2f1d29eac0be84c82611a9869318fae2ec5b3fe1392fd2ef9abef3cf`.
- `halofpx-project-p63-formal-evidence.tar.gz`: 2 parts,
  `2,516,292,772` bytes, SHA-256
  `412dc86ea616b91e77b8618ffae3e4cadf9597c30a32fb91b5a2d3df41a98892`.

Reconstructed total: `19,618,007,204` bytes. Downloaded plus reconstructed
total: `44,314,200,024` bytes.

Seven complete Git bundles passed `git bundle verify` and fresh-repository
recovery. The recovered legacy authorities are implementation commit
`620ef60aa446990335ef46c7d76738f797e62f8f` and wiki commit
`b1c2d8aef707fb03920fc189ccd26395fa61879d`. Donor recovery matched the
advertised ref sets for ROCmFPX (9 refs), CachyLlama (7), llama-ai (4), and
llama.cpp (7,485), plus Strix FA branch head
`a18067a85e986f7798f43d98345ed5b86b55cf88`.

The thin FFN source bundle was verified and fetched only after a fresh private
GitHub clone supplied its prerequisites. Its recovered exact head is
`3402aa7fbe820496726bfb45504549830634d7bd`.

## FFN archive safety

The 40,697-byte archive (SHA-256
`7a154b62d665c0a1324a84eda8adadde32006a1467f259bfb7e583f9797a82b0`)
contained eight regular-file members. Absolute/traversal paths, links, devices,
duplicates, and unknown member types were rejected by policy before extraction.
All eight files were then extracted with the Python data filter into the new
`recovered/ffn-evidence-portable/` root and rehashed. No archive was extracted
over an existing tree.

## Q3/Q6/Q8 fixture replay

All nine fixture-release assets were copied into a new recovery-only root and
rehashed against the already verified immutable downloads. The three GGUFs were
also copied into `derived/`; the original release download directory was not
changed.

The recipe-bearing checkout remained the clean PR #56 candidate. A separate
clean WSL worktree at exact consumer commit
`b77f2bce6e7875ab065e09894f45915585c9f156` built
`llama-completion`. The rebuilt binary is 11,466,224 bytes, reports `b77f2bce`,
and has SHA-256
`8623982c5c29d05b5f1df304f10ebf2f4eba26b058e86773b707761cc8637940`.
It does not match the historical observed binary hash, which is expected for an
environment/path-dependent rebuild and is not an acceptance requirement.

| Fixture | Bytes | SHA-256 | Census | CPU smoke |
| --- | ---: | --- | --- | --- |
| Q3 pure | 266,957,248 | `d1404c1afc61ffe49357c14c6d3dbfb252a72e87744fb7e491e7a2e205321fff` | 310 tensors: 113 F32 + 197 Q3 | exit 0; 4 requested tokens; retained text ` the word that is` |
| Q6 pure | 490,451,392 | `8d5c0eb545651c7518508632d9c00138cb64c22902eb83f5b8d7d52cf5fae8cc` | 310 tensors: 113 F32 + 197 Q6 | exit 0; 4 requested tokens; retained text ` the one that is` |
| Q8 pure | 620,822,976 | `ec152fed6e498cad29e75c32e11c8d520fed34bea26c5ad5bfef8a4e210a4bd7` | 310 tensors: 113 F32 + 197 Q8 | exit 0; 4 requested tokens; retained text ` the one that is` |

Each smoke used CPU only (`-dev none`, `-ngl 0`), tensor checking, deterministic
seed 1, temperature 0, and `n_predict = 4`. These are bounded load/tensor/generate
checks, not quality or speed results. Per-log hashes and outputs are bound in
`raw/17b-fixture-smoke-log-summary.json`.

## Validation matrix

The pinned validation environment used Python 3.12.13 and the exact tracked
`PyYAML==6.0.3` requirement. All scoped lanes exited zero:

- Wiki manifest matched; 86/86 sections were complete and schema-valid.
- 5 wiki tool tests passed.
- 41 Strix A/B and CachyOS-adapter tests passed.
- 12 fixture contract tests passed.
- 33 fresh-recovery contract tests passed.
- Documentation validation passed at candidate HEAD: 556 Markdown files, 12
  category manifests, 0 authoritative orphans, 0 broken internal links, and
  2,883 protected files with digest
  `54e0fb0e2f8eed329e64197d4cdeee3f81aa92f05a504b5d5e0c3c7047022583`.
- The documentation validator's independent rerun has explicit exit `0` in
  `raw/validation/documentation-validator-rerun.exit.txt`.
- The fresh-clone LF-vs-immutable-release manifest identity regression passed
  with explicit exit `0`.

The initial validation wrapper's 120-second capture expired while its WSL child
continued. The eventual summary contains all seven zero exits, but the wrapper
log stops before the documentation `DONE` line. A separately named clean rerun
was therefore retained as the unambiguous documentation exit authority.

## Current-head feature-OFF build

The exact `.github/workflows/halofpx-ci.yml` `feature-off-build` configuration
was run against clean candidate commit `bdd87e2e...` with CMake 4.2.3, Ninja
1.13.2, and GCC/G++ 15.2.0. Release/static mode was used, and `GGML_NATIVE`,
RPC, HaloFPX local state, tests, tools, examples, server, web UI, and OpenSSL
were all OFF.

Target `llama` built successfully. `src/libllama.a` is 9,359,840 bytes with
SHA-256
`711d02e407223e06755499a48de4df985e9638e17d6d7c92074636950cae42cc`.

This result is deliberately labeled **compile-only**: the exact documented lane
sets `LLAMA_BUILD_TESTS=OFF` and contains no CTest or runtime step. The broader
validation matrix above is separate and must not be misrepresented as a runtime
test of this all-off static library.

## Retained interruptions and retries

All failed or interrupted attempts remain in place; nothing was deleted or
silently replaced.

1. Validation wrapper: the outer capture timed out, while the WSL child later
   completed all seven lanes. A separate documentation rerun recorded exit 0.
2. Fixture venv: an initial PowerShell/Bash quoting error attempted `/workspace`
   and was denied before creating the intended venv. A literal-path,
   containment-checked retry succeeded in `workspace/fixture-venv/`.
3. Fixture consumer build: an accidental 1-second outer timeout left only
   `build/fixture-consumer-b77/CMakeFiles/`; this attempt was never reused. The
   second directory built all 242 steps and linked the binary; its outer capture
   later timed out during post-build reporting. A separate read-only receipt
   proved no active process, exact clean source, cache flags, an up-to-date
   target, binary hash, and embedded b77 commit.
4. Terminal Git receipt: attempt 1 failed because a helper passed an invalid
   `git -C` directory. Attempt 2 reached the WSL-linked worktree, which native
   Windows Git cannot parse because its `.git` pointer uses a Linux path.
   Attempt 3 correctly separated Windows-Git fresh-clone authority from WSL-Git
   fixture-worktree authority and passed, including `fsck_exit=0`.

## Environment and storage

- Windows 11 Pro `10.0.26200` build 26200, PowerShell 7.6.3.
- GitHub CLI 2.96.0 at the required explicit path.
- Windows Git 2.55.0.windows.2.
- WSL2 kernel `6.18.33.2-microsoft-standard-WSL2`.
- WSL Git 2.53.0, Python 3.12.13, CMake 4.2.3, Ninja 1.13.2,
  GCC/G++ 15.2.0, GNU tar 1.35.
- Initial C: free space: `201,880,731,648` bytes (188.02 GiB), above the
  required 70 GiB.
- Terminal pre-receipt snapshot: `152,791,101,440` free bytes (142.3 GiB).
- Retained rehearsal snapshot before receipts: 49,896 files and
  `50,107,213,417` bytes.

The fixture parsing environment installed the repository's editable `gguf-py`
and recorded its resolved freeze in `raw/14b-fixture-venv-bootstrap-rerun.txt`.
Public package indexes and the existing host toolchain were used for build/test
dependencies. Project code and retained continuation assets came from GitHub;
this was not an offline or sterile-host dependency proof.

## GitHub tracking and remaining open boundaries

A bounded result was posted to issue #2 at
<https://github.com/JCFrags/HaloFPX/issues/2#issuecomment-5277106922>. The issue
state was checked before and after the comment and remained **OPEN**.

Still open:

- Repeat the GitHub-only continuation proof on a genuinely never-used PC.
- No Strix Halo target or SSH execution occurred here.
- The primary large model is outside this recovery lane.
- The fixture BF16 source remains external; release-derived verification and
  smoke did not require it.
- Historical FFN binaries/raw checksum stdout that were never published remain
  unavailable.
- Agent Harness remains an external referenced authority.
- Private release access does not imply blanket public redistribution rights.

## Evidence and continuation routes

The machine-readable companion receipt is
`receipts/2026-08-12__halofpx-same-pc-full-recovery__receipt__open-v01.json`.
The SHA-256 ledger covers every retained file under `raw/` plus both receipts;
the ledger excludes only itself to avoid a circular digest.

Start future work from the clean clone and read, in order:

1. `HANDOFF.md`
2. `ARTIFACTS.md`
3. `project/WORKER_START_HERE.md`
4. `project/wiki/HaloFPX_Wiki/README.md`
5. `project/wiki/HaloFPX_Wiki/evidence-map.md`
6. `project/wiki/HaloFPX_Wiki/decision-map.md`
7. `docs/publication/continuation-releases.json`

Retained directories:

- `raw/`: command output, API/attestation payloads, summaries, validators.
- `downloads/`: immutable assets in exact tag-scoped directories.
- `recovered/`: reconstructed archives, recovered repositories, fixtures.
- `build/`: fixture consumer and feature-OFF build evidence, including failed
  attempt directories.
- `workspace/`: fresh source clone, worktree, and validation environments.
- `tmp/`: exact rehearsal helper scripts and the issue-comment body.
- `receipts/`: this receipt, machine-readable companion, and SHA ledger.

No cleanup was performed.
