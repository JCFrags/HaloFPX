---
title: Acceptance Criteria
description: Mandatory provenance, build, correctness, security, performance, and rollback gates.
status: Proposed release gate
evidence-date: 2026-07-17
canonical-repository: charlie12345/ROCmFPX
---

# Acceptance Criteria

> [!NOTE]
> Evidence is pinned to **2026-07-17**. Repository heads and source links are locked in [[Source-Register]] and `evidence/commit-lock.json`.


## Gate A — Provenance and licensing

Every source-derived change has:

- exact repository, commit, parentage, path, and blob SHA;
- file-level license/SPDX evidence;
- author/committer and contribution history;
- upstream ancestry/patch-equivalence classification;
- confirmation that no GPL-parent source was copied;
- treatment decision (CP/MP/IF/CR) and approving reviewer;
- attribution/commit trailers and notice updates where required.

**Pass condition:** all records P3 before code import; clean-room work has separate spec author/implementer attestations.

## Gate B — Build matrix

At minimum:

| Platform/backend | Feature off | Feature on | Required variants |
|---|---:|---:|---|
| Linux GCC CPU | Pass | Pass | static/shared where supported; ASan/UBSan parser tests. |
| Linux Clang CPU | Pass | Pass | warnings-as-errors for touched modules. |
| Linux HIP/ROCm | Pass | Pass | ROCmFPX quant/runtime and MTP smoke. |
| Linux Vulkan | Pass | Pass | AMD APU/small-GPU regression and upstream tuning assertion. |
| Windows MSVC | Pass | Pass | UTF-8 path, shutdown, permissions/cleanup semantics. |
| macOS Clang | Pass | Pass where supported | readahead/no-op platform behavior. |

**Pass condition:** every lane commit is green in its required subset; release branch is green in the full matrix.

## Gate C — Feature-off equivalence

- Existing `--cache-disk*` tests pass unchanged.
- No change in default cache lifecycle, HTTP surface, scheduling, or C ABI.
- Median throughput/TTFT regression ≤ **1%**, with no unexplained p95 regression above **2%** on the controlled baseline.
- Binary size and startup-time changes are reported.

Thresholds may be tightened after baseline collection; they cannot be loosened without an acceptance-record rationale.

## Gate D — Persistent-store correctness

- Save, restart, restore, and continue produce the same deterministic next-token logits/tokens as a cold path for controlled fixtures.
- Target/draft/speculative/recurrent components round-trip as one transaction.
- Missing or corrupt mandatory component yields cold fallback with no partial context mutation.
- Context shift and overflow preserve state boundaries.
- Same prompt under incompatible model/tokenizer/template/KV types is rejected.
- Old/new major/minor/feature-bit matrix behaves as specified.
- Transformer, target+draft, and supported hybrid fixtures pass.
- Multimodal entries are rejected or supported explicitly—never accidentally accepted.

## Gate E — Crash, storage, and concurrency safety

Fault injection covers:

- process kill after each write/rename/sync step;
- ENOSPC, EIO, permission failure, short write/read, stale lock, and read-only filesystem;
- corrupted manifest, duplicate JSON key, integer overflow, huge length, path traversal, symlink, truncated component, digest mismatch;
- concurrent read/write and, if claimed, multi-writer operation;
- bounded staging/quarantine and quota accounting;
- restart cleanup that cannot remove a live owner's data.

**Pass condition:** no partial entry is accepted, no pre-existing committed entry is destroyed by a failed incoming write, and service can cold-fallback.

## Gate F — Tenant and scheduling isolation

- Explicit scope A cannot enumerate, match, load, or evict scope B.
- Explicit scope never falls back to anonymous/cross-conversation search.
- Raw user ID never appears in path or default logs.
- Per-user cap is authoritative under races and maps to the expected HTTP 429 envelope.
- Anonymous-scope policy is explicit and tested.
- Slot affinity off is scheduler-equivalent; `prefer` cannot starve other scopes.

Any violation is a release blocker and security incident.

## Gate G — Performance

Measure on the same hardware/model/build with confidence intervals:

- cold TTFT and tokens/s;
- warm RAM hit;
- persistent SSD hit after restart;
- entry write latency and bytes;
- hit-rate/retention behavior under mixed conversations;
- p95/p99 server latency under parallel load;
- expert tracking overhead off/on.

Suggested budgets:

- feature-off overhead: ≤1% median;
- expert tracking disabled: ≤0.5% median; enabled: ≤2% decode overhead;
- persistent hit must improve TTFT versus cold evaluation for the target workload and must not be promoted solely on donor-reported absolute figures. [S12]

## Gate H — Upstream survivability

- Rebase/range-diff against the latest frozen upstream sync.
- No unresolved Critical conflict path.
- ABI diff reviewed.
- Existing ROCmFPX quantization/MTP/backend gates pass.
- Donor-surveillance report classifies new relevant donor changes.

## Gate I — Documentation and operations

- Flags, format, metrics, failure modes, and storage permissions documented.
- Source register and `AI_CHANGES.md` updated. [S04]
- Canary and rollback runbooks rehearsed.
- Previous binary successfully starts in ephemeral mode while the v1 store remains untouched.

## Release decision

All gates are conjunctive. A performance win cannot waive correctness, provenance, isolation, format, or rollback failures.


---

**Wiki navigation:** [[Home]] · [[Executive-Decision]] · [[Capability-Decision-Matrix]] · [[Patch-Lanes-and-Dependency-Graph]] · [[Acceptance-Criteria]]
