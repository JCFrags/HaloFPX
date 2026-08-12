---
title: Risk Register
description: Material technical, licensing, operational, and security risks.
status: Active
evidence-date: 2026-07-17
canonical-repository: charlie12345/ROCmFPX
---

# Risk Register

> [!NOTE]
> Evidence is pinned to **2026-07-17**. Repository heads and source links are locked in [[Source-Register]] and `evidence/commit-lock.json`.


## Scoring

Likelihood and impact are rated Low/Medium/High/Critical. “Trigger” is the observable condition that invokes mitigation or rollback.

| ID | Risk | Likelihood | Impact | Mitigation | Trigger / rollback |
|---|---|---:|---:|---|---|
| R01 | Donor identity is not the repository intended by the user. | Medium | High | Maintainer confirms `fewtarius/CachyLLama`; preserve requested/resolved names in records. | No confirmation → stop source import; clean-room requirements only. |
| R02 | GPL parent code contaminates MIT engine. | Medium | Critical | Clean-room boundary, reviewer attestations, no parent code diff/copy. | Similarity/provenance concern → quarantine lane, legal review, revert. |
| R03 | Broad donor merge overwrites canonical cache/MTP safety fixes. | High | Critical | Never merge donor branch; provider-first manual lanes. | Donor branch appears as merge parent → reject PR. |
| R04 | Native/unstable cache format becomes long-term ABI. | High | Critical | New versioned manifest/components, strong fingerprints, reader-before-writer. | Unknown/mismatched entry accepted → disable persistent reads. |
| R05 | Partial target/draft/spec/recurrent restore corrupts output. | Medium | Critical | All-or-nothing validation and restore, mandatory component mask, cold fallback. | Any deterministic divergence → runtime rollback and incident. |
| R06 | Cross-tenant cache disclosure. | Medium | Critical | Keyed opaque scopes, no fuzzy cross-scope search, isolation tests. | Any cross-scope lookup → security rollback. |
| R07 | Upstream state API changes invalidate persistent entries. | High | High | Build/upstream IDs in manifest, compatibility vector, sync gate. | Critical path upstream change → freeze lanes and revalidate. |
| R08 | Existing per-run cache regresses. | Medium | High | Preserve old flags/provider, unchanged tests, feature-off equivalence. | Existing test or behavior change → revert owning lane. |
| R09 | Disk full/crash destroys old valid cache entries. | Medium | High | Stage/commit before eviction, sync discipline, write circuit breaker. | ENOSPC/EIO fault test failure → block writer release. |
| R10 | Persistent store grows without bound. | Medium | High | Payload/staging/quarantine quotas, LRU/expiry, conservative accounting. | Quota overshoot beyond headroom → read-only/ephemeral mode. |
| R11 | User cap/affinity introduces starvation or races. | Medium | High | Separate lanes, authoritative locked counter, affinity as hint. | Counter invariant or fairness failure → disable L11 then L10. |
| R12 | Expert instrumentation slows hot path or changes ABI. | Medium | Medium/High | Compile/runtime off, internal provider first, overhead budget. | Budget/ABI gate fails → omit public API and disable build. |
| R13 | Donor performance claims drive unsafe acceptance. | Medium | High | Independent controlled benchmarks; correctness gates are non-waivable. | Claim cannot be reproduced → no release promotion. |
| R14 | Page manager is treated as production-ready without evidence. | Medium | High | Defer C16; separate research lane. | Pressure to make it a dependency → require new ADR and maturity evidence. |
| R15 | Windows/POSIX lifecycle differences leak or delete data. | Medium | High | Cross-platform permissions/locks/path tests; conservative Windows cleanup. | Platform-specific cleanup/ownership failure → disable persistent mode there. |
| R16 | Scope-key secret loss/rotation makes entries unavailable. | Medium | Medium | Versioned key IDs, documented rotation, no silent remap. | Secret unavailable → cold fallback; preserve store for authorized migration. |
| R17 | Cache parser becomes an untrusted-input attack surface. | Medium | Critical | Bounded parser, fuzzing, safe paths, no server donor importer. | Crash/OOM/path escape → compile out provider and incident response. |
| R18 | Feature flags create ambiguous lifecycle semantics. | Medium | High | Existing flags unchanged; explicit precedence and startup errors. | Same flag selects different lifecycle across versions → block release. |

## Risk owners

- **Canonical maintainer:** R01, R03, R07, R08, R18.
- **Provenance/license reviewer:** R02.
- **Cache/storage owner:** R04, R05, R09, R10, R15, R17.
- **Security owner:** R06, R16.
- **Scheduler owner:** R11.
- **Performance/telemetry owner:** R12, R13, R14.

Owners must be named individuals in the implementation project tracker; roles here are placeholders.


---

**Wiki navigation:** [[Home]] · [[Executive-Decision]] · [[Capability-Decision-Matrix]] · [[Patch-Lanes-and-Dependency-Graph]] · [[Acceptance-Criteria]]
