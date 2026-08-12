# HaloFPX Worker Start

Every worker must complete this sequence before changing project material.

## Required sequence

1. Read [`AGENTS.md`](AGENTS.md).
2. Read [`PROJECT_GOAL.md`](PROJECT_GOAL.md).
3. Read [`CURRENT_STATE.md`](CURRENT_STATE.md).
4. Read the Project Lead [`CURRENT_STATUS.md`](project-management/lead/CURRENT_STATUS.md).
5. Read the Project Lead [`DECISIONS.md`](project-management/lead/DECISIONS.md).
6. Read the relevant [Wiki category manifest](wiki/HaloFPX_Wiki/README.md).
7. Read each accepted decision that the category manifest links.
8. Verify current source and linked evidence.
9. Check the exact production authority before an authorized transition.
10. Preserve unrelated worktree files.
11. Keep feature-off behavior unchanged.
12. Report exact commits, binaries, evidence, and cleanup.

## Project Lead records

- [`OBJECTIVES.md`](project-management/lead/OBJECTIVES.md) defines the end state and gates.
- [`CURRENT_STATUS.md`](project-management/lead/CURRENT_STATUS.md) records the current verified state.
- [`MONITORING.md`](project-management/lead/MONITORING.md) defines observation and intervention rules.
- [`DECISIONS.md`](project-management/lead/DECISIONS.md) records accepted boundaries.
- [Documentation task specification](project-management/lead/worker-specs/DOCUMENTATION_STE_ORGANIZATION_TASK.md)
  controls documentation organization work.
- [L111 visible implementation specification](project-management/lead/worker-specs/L111_VISIBLE_IMPLEMENTATION_TASK.md)
  controls the active implementation task.

## Authority rules

Use this precedence:

1. Exact current source and commit.
2. Machine evidence and immutable receipts.
3. Accepted project decisions.
4. Official documentation and standards.
5. Research papers.
6. Labeled inference or recommendation.

Do not convert `[INFERENCE]` into a fact.
Do not convert `[OPEN]` into an accepted result.
Do not change exact hashes, measurements, versions, or identifiers.

## Worktree rules

Inspect both repositories before implementation-related work:

```powershell
git status --short
git -C C:\Users\britt\Documents\HaloFPX status --short
```

Treat every unrelated change as user-owned work.
Do not edit HaloFPX implementation source unless the assigned task owns that source.
Do not rewrite raw evidence, receipts, logs, archives, third-party source, or licenses.

## Production rule

Read the exact Project Lead production authority before any authorized transition.
Do not infer current health from an older milestone.
Report any uncertainty before mutation.

## Closeout report

Report these items:

- repository HEAD and worktree state;
- changed files and owned categories;
- exact binaries, models, and commits when applicable;
- evidence paths and validation results;
- feature-off results;
- production health when touched;
- cleanup and retained artifacts;
- blockers, open work, and the next safe action.
