# Scope and exact source pins

## Immutable assessment baseline

| Scope | Repository | Exact commit | Commit date | Evidence |
|---|---|---|---|---|
| Parent | `fewtarius/llama-ai` | `1017f3dfdce3ca2b06aa9007b23295db3bb35722` | 2026-07-09 | [E-001](20-Evidence-Index.md#e-001) |
| Pinned component | `fewtarius/CachyLlama` | `6be745998f568e379ea197fcf827baec73ff9940` | 2026-07-09 | [E-002](20-Evidence-Index.md#e-002), [E-020](20-Evidence-Index.md#e-020) |
| ROCmFPX target | `charlie12345/ROCmFPX` | `a5605a72768c6562241b248e268e33dc92787394` | 2026-07-17 | [E-200](20-Evidence-Index.md#e-200) |
| Historical corrective evidence | `fewtarius/CachyLlama` | `c8ead677a7fe42fb0a67e6e866fb254cc338e9fd` | before the assessed merge | [E-070](20-Evidence-Index.md#e-070) |

The parent submodule declaration tracks the CachyLlama repository's moving `master` branch, but this inventory does not use that moving branch as evidence. The exact component gitlink selected by the parent commit is `6be745998f568e379ea197fcf827baec73ff9940`.

## Included

- Parent launch, model discovery, profile selection, build, service, hardware detection, and host-tuning behavior.
- CachyLlama custom and relevant server capabilities: persistent KV state, system-prefix cache, matching/continuation, checkpoint restore, user isolation, scheduling, APIs, router lifecycle, observability, and backend tuning.
- ROCmFPX's existing prompt-cache, MTP, backend, test, and licensing baseline where it changes a porting decision.
- Configuration defaults from source, documentation, and parent overrides—kept separate when they conflict.

## Excluded

- Uncommitted local worktrees.
- Runtime behavior not supported by pinned source, tests, documentation, or explicit code inference.
- Reproduction of throughput figures or hardware correctness.
- A full audit of every upstream llama.cpp feature unrelated to the requested capability areas.
- Model-weight licenses and third-party binary redistribution analysis.

## Evidence convention

Every feature row lists one or more evidence IDs. Each evidence record contains:

1. repository;
2. exact commit;
3. exact source path;
4. symbol/behavior locator;
5. the narrow claim supported;
6. source URL;
7. any caveat.

An implementation gap established by source comparison is labeled as such. Portability recommendations are analysis, not source claims.

## Maturity rubric

| Level | Meaning |
|---|---|
| M0 | Documented, dead, duplicate, or unbuilt path. |
| M1 | Prototype or integrated path with material correctness, security, observability, or validation gaps. |
| M2 | Operational source path with active churn, recent fixes, or incomplete focused tests. |
| M3 | Integrated path with focused tests or repeated operational evidence; may still be explicitly experimental. |
| M4 | Stable, released, compatibility-governed feature. No assessed custom feature qualifies. |

## Static-analysis limitation

No binary was built or run. Therefore, statements such as “the code registers this route,” “the target test suite covers this failure,” or “the storage engine uses atomic rename” are supported. Claims such as “this improves throughput on all Strix Halo systems” are not made.
