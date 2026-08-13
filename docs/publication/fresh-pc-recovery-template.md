# HaloFPX Fresh-PC Recovery Receipt Template

Status: **OPEN — TEMPLATE — NOT EVIDENCE**

This file is a blank acceptance record. Copy it to a dated, run-specific path
before filling it in. The template itself proves nothing, and a metadata-only
result is not a fresh-PC recovery PASS.

Tracking: [issue #2](https://github.com/JCFrags/HaloFPX/issues/2) owns the full
fresh-PC acceptance gate. [Issue #11](https://github.com/JCFrags/HaloFPX/issues/11)
owns prerequisite/bootstrap work; completing it does not close issue #2.

## Run identity

| Field | Recorded value |
|---|---|
| Receipt status | `OPEN` |
| Started/completed UTC | `[NOT RUN]` |
| Operator | `[NOT RECORDED]` |
| Recovery-machine OS and architecture | `[NOT RECORDED]` |
| Repository | `JCFrags/HaloFPX` (expected; not checked by this template) |
| Clone path | `[NOT RECORDED]` |
| Candidate `HEAD` | `[NOT RECORDED]` |
| `origin/main` | `[NOT RECORDED]` |
| Registry SHA-256 | `[NOT RECORDED]` |
| Runner state/receipt paths | `[NOT RECORDED]` |
| Original-asset verifier deadline | `43,200 seconds default; record any bounded override` |

Never copy credentials, tokens, private keys, cookies, authenticated command
output, or secrets into this receipt.

## Gate ledger

Use only `PASS`, `FAIL`, `BLOCKED`, or `NOT RUN`. Link raw output or a
machine-readable receipt for every claimed `PASS`.

| Gate | Status | Evidence/result |
|---|---|---|
| Fresh authenticated clone of the private repository | `NOT RUN` | — |
| Exact repository ID, owner ID, and private visibility | `NOT RUN` | — |
| Full non-shallow history, forced tag fetch, `git fsck --full`, clean worktree | `NOT RUN` | — |
| Candidate `HEAD` and `origin/main` recorded and intentionally selected | `NOT RUN` | — |
| Required tool versions and authenticated GitHub CLI preflight | `NOT RUN` | — |
| At least 53,687,091,200 free bytes; 64,424,509,440 recommended | `NOT RUN` | — |
| Pinned Python validation environment installed | `NOT RUN` | — |
| Generated Wiki manifest check and Wiki validator | `NOT RUN` | — |
| Complete Wiki tool discovery, including manifest-generator tests | `NOT RUN` | — |
| Documentation validator | `NOT RUN` | — |
| Portable fixture registry/materializer offline tests | `NOT RUN` | — |
| All four release metadata records match the continuation registry | `NOT RUN` | — |
| `gh release verify` succeeds for all four exact tags | `NOT RUN` | — |
| All 52 release assets downloaded and size/SHA-256 verified | `NOT RUN` | — |
| Original 41-asset release accepted by `verify-publication-assets.ps1` | `NOT RUN` | — |
| Runner has no stale `RUNNING` state after a caught recovery error | `NOT RUN` | — |
| Both split payloads reconstructed in manifest order and rehashed | `NOT RUN` | — |
| Legacy and donor Git bundles pass `git bundle verify` and expected-ref checks | `NOT RUN` | — |
| FFN evidence archive verified and inspected safely | `NOT RUN` | — |
| Thin FFN source bundle verifies and recovers exact source `3402aa7fbe820496726bfb45504549830634d7bd` | `NOT RUN` | — |
| Nine-asset Qwen3-0.6B fixture release verified and materialized | `NOT RUN` | — |
| Archive member safety checks completed before extraction to new directories | `NOT RUN` | — |
| Documented focused source/build/test lane succeeds on the recovery machine | `NOT RUN` | — |
| A successor can locate source, Wiki, decisions, evidence, licenses, and open work from the clone | `NOT RUN` | — |

## Metadata-only partial result

Record the runner's cheap, non-bulk verification here. This subsection may be
`PASS` while the overall receipt remains `OPEN`.

| Check | Status | Evidence/result |
|---|---|---|
| Preflight | `NOT RUN` | — |
| Four release metadata records | `NOT RUN` | — |
| Four release attestations | `NOT RUN` | — |

Metadata-only disposition: **`NOT RUN`**. Even a metadata-only `PASS` does not
prove asset bytes, reconstruction, extraction, build, runtime behavior, target
behavior, or full fresh-PC continuation.

## Bulk recovery and reconstruction

Record the exact download directory, byte count, and verifier output. Bulk
download must use the four pinned tags from
`docs/publication/continuation-releases.json`; never use an unpinned `latest`
release. Reconstruct split payloads only in manifest order and into a new
scratch directory. Do not overlay the clone, an active worktree, or live
configuration.

| Field | Recorded value |
|---|---|
| Bulk-download authorization/intent | `[NOT RUN]` |
| Download directory | `[NOT RECORDED]` |
| Downloaded release bytes | `[NOT RECORDED]` |
| Reconstructed payload bytes | `[NOT RECORDED]` |
| Original-release verifier receipt | `[NOT RECORDED]` |
| Split-payload reconstruction receipt | `[NOT RECORDED]` |
| Bundle and fixture receipts | `[NOT RECORDED]` |

## Deliberate exclusions and unresolved resources

These exclusions are not failures when recorded accurately, but none may be
silently described as restored:

- `[NOT RUN]` No Strix Halo target access, production mutation, deployment, or
  target performance measurement is performed by this recovery template.
- `[NOT RUN]` No 24,696,192,820-byte release set or 44,314,200,024-byte
  release-plus-reconstruction set has been downloaded merely by creating this
  file.
- `[OPEN]` The primary model is absent from Git and all project releases. Only
  its recorded size and SHA-256 identity are retained; no authorized source is
  recorded.
- `[OPEN]` The fixture BF16 source remains an exact external Hugging Face
  input. Its absence does not prevent recovery of the nine published fixture
  assets, but it prevents claiming source redistribution through HaloFPX.
- `[OPEN]` Historical FFN target binaries and raw checksum-command stdout were
  not retained. The published evidence and thin source bundle do not recreate
  those missing bytes.
- `[OPEN]` Agent Harness remains an external reference authority and is not
  imported into this repository.
- `[INTENTIONALLY EXCLUDED]` Credentials, private keys, authenticated sessions,
  live service state, generated build trees, and caches are not continuation
  artifacts.
- `[NOT AUTHORIZED]` The private release set and mixed-license research
  collection are not approved for blanket public redistribution.
- `[OPEN / THREAT BOUNDARY]` The runner assumes one trusted local operator and
  one process per work root. It rejects links/reparse points and unsafe
  containment, but it does not lock against a second local actor changing a
  validated file before use. Keep the root operator-owned; concurrent-actor/
  TOCTOU hardening remains a distinct follow-up.

## Final disposition

Overall fresh-PC recovery: **`OPEN / NOT RUN`**

Issue #2 may be closed only after every required gate above has run on the
recorded fresh machine, every required byte is independently verified, all
failures and exclusions are explicit, and an independent reviewer accepts the
complete run-specific receipt. Record reviewer identity, review commit, and
review result below.

| Field | Recorded value |
|---|---|
| Independent reviewer | `[NOT RECORDED]` |
| Reviewed commit | `[NOT RECORDED]` |
| Review result | `NOT RUN` |
| Remaining blockers | `Full acceptance remains OPEN` |
