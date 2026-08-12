# PF-IR-01 — llama.cpp / RPC / GGUF security applicability and backport ledger

[VERIFIED] Package version `1.0.0`; evidence access date `2026-07-18`; scope is the exact source snapshots named by the request.

[RECOMMENDATION] **Release decision: HOLD.** The candidate source contains the material published fixes reviewed, but the standard release workflow still compiles RPC and the HTTP server, while deployed-binary, loaded-library, listener, and negative-reachability evidence remains local.

[VERIFIED] Start with [`01-executive-decision.md`](01-executive-decision.md), then use [`08-release-gate.md`](08-release-gate.md) as the sign-off checklist. Open [`index.html`](index.html) for the styled offline wiki.

[VERIFIED] This package contains bounded raw primary-source captures, exact commit and blob identifiers, source/claim/license manifests, SHA-256 integrity data, static source sentinels, and safe negative-test specifications.

[VERIFIED] No exploit code was executed or included, and no live service was probed.

## Contents

| Label | Path | Purpose |
|---|---|---|
| [VERIFIED] | `index.html` | Offline LLM-Wiki dashboard with search and filters |
| [VERIFIED] | `00-scope-method.md` | Scope, evidence taxonomy, method, and limitations |
| [VERIFIED] | `01-executive-decision.md` | P0 decision, required controls, and concise disposition |
| [VERIFIED] | `02-advisory-ledger.md` | Per-advisory applicability for llama.cpp and historical GGUF reports |
| [VERIFIED] | `03-snapshot-equivalence.md` | Exact snapshots, blob identity, semantic equivalence, proof boundaries |
| [VERIFIED] | `04-build-runtime-reachability.md` | Compile flags, listeners, auth, paths, and trust boundaries |
| [VERIFIED] | `05-security-relevant-fixes-after-b3561.md` | Post-known-RPC security change inventory |
| [VERIFIED] | `06-backport-plan.md` | Required policy/build backports and no-op source backports |
| [VERIFIED] | `07-safe-negative-tests.md` | Safe lab prerequisites and non-exploit regression specifications |
| [VERIFIED] | `08-release-gate.md` | Release-blocking evidence checklist |
| [VERIFIED] | `09-open-local-evidence.md` | Local-only binary, library, listener, and reachability evidence |
| [VERIFIED] | `10-dependency-ledger.md` | Bundled dependency provenance and official advisories |
| [VERIFIED] | `evidence/` | Bounded primary-source captures and source excerpts |
| [VERIFIED] | `manifests/` | Sources, claims, source-to-claim, licenses, snapshots, and hashes |
| [VERIFIED] | `tests/` | Static sentinels and manifest verifier; no exploit harness |

## Integrity

[RECOMMENDATION] Run `python3 tests/verify_manifest.py .` from this directory. A clean result proves package-file integrity only; it does not prove external source freshness or deployed-binary equivalence.
