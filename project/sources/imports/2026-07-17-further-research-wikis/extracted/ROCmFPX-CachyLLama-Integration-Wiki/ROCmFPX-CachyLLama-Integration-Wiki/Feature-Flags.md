---
title: Feature Flags
description: Compile-time and runtime controls, compatibility semantics, and rollout states.
status: Proposed
evidence-date: 2026-07-17
canonical-repository: charlie12345/ROCmFPX
---

# Feature Flags

> [!NOTE]
> Evidence is pinned to **2026-07-17**. Repository heads and source links are locked in [[Source-Register]] and `evidence/commit-lock.json`.


## Compatibility rule

Existing ROCmFPX options retain their current meaning. In particular, `--cache-disk` and `--cache-disk-limit` continue to select the **per-run, owner-scoped, cleanup-on-shutdown** cache. They must not silently become a persistent cross-restart store. [S06] [S08] [S09]

## Compile-time gates

| CMake option | Initial default | Purpose |
|---|---:|---|
| `ROCMFPX_ENABLE_PERSISTENT_CACHE` | `OFF` | Build canonical v1 reader/writer and persistent provider. |
| `ROCMFPX_ENABLE_DONOR_CACHE_IMPORT` | `OFF` | Build an offline read-only donor-format inventory/import tool; never linked into server by default. |
| `ROCMFPX_ENABLE_HYBRID_CACHE_RESTORE` | `OFF` | Build attention/recurrent state capability implementation. |
| `ROCMFPX_ENABLE_EXPERT_TELEMETRY` | `OFF` | Build expert counters/provider. |

Compile-time gates are temporary containment boundaries, not substitutes for runtime checks. A feature may become always-built only after its parser/hot-path risk is accepted.

## Runtime flags

Proposed names are canonical-facing and intentionally do not copy donor names:

| Flag | Values / default | Behavior |
|---|---|---|
| `--context-store-mode` | `off` (default), `ephemeral`, `persistent-read-only`, `persistent-read-write` | Select provider lifecycle. Existing `--cache-disk` maps only to `ephemeral` when no explicit new mode is supplied. |
| `--context-store-path` | none | Root of canonical persistent store; required for persistent modes. |
| `--context-store-limit-mib` | operator-set, bounded default after benchmarks | Payload quota; metadata/staging headroom reported separately. |
| `--context-store-fail-policy` | `cold` (default), `read-only`, `fatal` | `cold` rejects bad entry and evaluates prompt; write faults open a write circuit breaker. |
| `--context-store-verify` | `required` | Digest/size/model compatibility checks cannot be disabled in stable builds. |
| `--system-prefix-cache` | `off` (default), `explicit`, `template` | Enables global prefix namespace; no heuristic mode in stable v1. |
| `--system-prefix-max-entries` | `0` (disabled) | LRU/expiry count when prefix cache enabled. |
| `--max-concurrent-per-user` | `0` (disabled) | Per-scope in-flight cap; anonymous scope policy documented explicitly. |
| `--slot-affinity` | `off` (default), `prefer` | Adds a cache-locality score only. |
| `--expert-tracking` | `off` (default), `counts` | Runtime telemetry mode when compiled. |

## Request fields

The donor design uses `llama_user_id` and Anthropic `metadata.user_id`. The manual port may retain those wire-compatible inputs, but both map immediately to a neutral internal `cache_scope_input`; raw values are validated, never placed in paths, and never reused as the on-disk key. [S19]

Validation baseline:

- maximum 512 bytes unless canonical API policy sets a smaller value;
- conservative ASCII allowlist for the compatibility field;
- explicit empty/absent semantics;
- keyed digest for on-disk scope key;
- no implicit cross-scope continuation search.

## Flag precedence

1. Explicit `--context-store-mode` wins.
2. Without it, existing `--cache-disk PATH` selects `ephemeral` and preserves all old lifecycle semantics.
3. `persistent-*` without `--context-store-path` is a startup error.
4. `persistent-*` when the compile option is absent is a startup error, not a silent fallback.
5. A donor-style `--cache-ssd` alias is unknown until L15; if later added, it must print the canonical mode it selects.

## Rollout states

| State | Compile | Runtime | Writes | Reads | Intended use |
|---|---|---|---:|---:|---|
| F0 | off | unavailable | no | no | Baseline/rollback binary. |
| F1 | on | off | no | no | Integration build. |
| F2 | on | persistent read-only | no | yes | Parser/canary validation. |
| F3 | on | shadow writer | disposable only | validation only | Atomicity/load testing. |
| F4 | on | bounded read-write | yes | yes | Canary deployment. |
| F5 | on | opt-in stable | yes | yes | Supported feature. |
| F6 | on | default-on | yes | yes | Requires a separate ADR and two-release rollback evidence. |

## Metrics required for every mode

Export provider name, format version, read/write circuit state, hits/misses, accepted/rejected/quarantined entries, bytes read/written/evicted, restore component mask, compatibility rejection reason, and tenant-scope policy—without exposing raw prompt or identity content.


---

**Wiki navigation:** [[Home]] · [[Executive-Decision]] · [[Capability-Decision-Matrix]] · [[Patch-Lanes-and-Dependency-Graph]] · [[Acceptance-Criteria]]
