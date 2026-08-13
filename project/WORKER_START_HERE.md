# HaloFPX Worker Start

Every worker must complete this sequence before changing project material.

## Required sequence

1. Read the monorepo root [`AGENTS.md`](../AGENTS.md).
2. Read the project [`AGENTS.md`](AGENTS.md).
3. Read [`PROJECT_GOAL.md`](PROJECT_GOAL.md).
4. Read [`CURRENT_STATE.md`](CURRENT_STATE.md).
5. Read the Project Lead [`CURRENT_STATUS.md`](project-management/lead/CURRENT_STATUS.md).
6. Read the Project Lead [`DECISIONS.md`](project-management/lead/DECISIONS.md).
7. Read the relevant [Wiki category manifest](wiki/HaloFPX_Wiki/README.md).
8. Read each accepted decision that the category manifest links.
9. Verify current source and linked evidence.
10. Check the exact production authority before an authorized transition.
11. Preserve unrelated worktree files.
12. Keep feature-off behavior unchanged.
13. Report exact commits, binaries, evidence, and cleanup.

## Project Lead records

- [`OBJECTIVES.md`](project-management/lead/OBJECTIVES.md) defines the end state and gates.
- [`CURRENT_STATUS.md`](project-management/lead/CURRENT_STATUS.md) records the current verified state.
- [`MONITORING.md`](project-management/lead/MONITORING.md) defines observation and intervention rules.
- [`DECISIONS.md`](project-management/lead/DECISIONS.md) records accepted boundaries.
- [Documentation task specification](project-management/lead/worker-specs/DOCUMENTATION_STE_ORGANIZATION_TASK.md)
  controls documentation organization work.
- [L111 visible implementation specification](project-management/lead/worker-specs/L111_VISIBLE_IMPLEMENTATION_TASK.md)
  is the completed historical contract for the bounded retained foundation.
  It does not authorize a successor milestone.

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

Inspect the complete monorepo before implementation-related work:

```powershell
$repoRoot = (git rev-parse --show-toplevel).Trim()
git -C $repoRoot status --short
git -C $repoRoot log -1 --format='%H'
```

Treat every unrelated change as user-owned work.
Do not edit HaloFPX implementation source unless the assigned task owns that source.
Do not rewrite raw evidence, receipts, logs, archives, third-party source, or licenses.

## Fresh-clone validation bootstrap

For a newly cloned worktree, fetch the complete tag namespace and reject a
shallow history before relying on provenance checks:

```powershell
git fetch --tags --force origin
if ((git rev-parse --is-shallow-repository).Trim() -eq 'true') {
    git fetch --unshallow --tags --force origin
}
git fsck --full
```

Create the repository's pinned Python 3.12 validator environment and run the
offline documentation, Strix A/B harness, and fixture contracts from the
repository root:

```powershell
python3.12 -X utf8 -m venv .venv
./.venv/bin/python -m pip install --requirement requirements/requirements-halofpx-validation.txt
./.venv/bin/python -X utf8 -B project/research/prompts/tools/generate_wiki_manifest.py project/wiki/HaloFPX_Wiki --check
./.venv/bin/python -X utf8 -B project/research/prompts/tools/validate_wiki.py project/wiki/HaloFPX_Wiki
./.venv/bin/python -X utf8 -B -m unittest discover -s project/research/prompts/tools -p "test_*.py"
./.venv/bin/python -X utf8 -B -m unittest tests/test_halofpx_strix_ab.py tests/test_halofpx_strix_ab_cachyos.py -v
./.venv/bin/python -X utf8 -B tests/test_materialize_rocmfpx_fixture.py -v
./.venv/bin/python -X utf8 -B project/project-management/documentation/validate_documentation.py
```

On a Windows control checkout, substitute `py -3.12` and
`.\.venv\Scripts\python.exe -X utf8`. Issue #2's acceptance environment is
clean Linux.

These are offline contract checks; they do not contact the Strix Halo targets,
download release payloads, or prove a fresh-PC recovery. Follow the full
preflight in [`TARGET_MACHINES.md`](TARGET_MACHINES.md). GitHub
[issue #11](https://github.com/JCFrags/HaloFPX/issues/11) owns that bootstrap
prerequisite; completion of it alone does not close the end-to-end clean-PC
acceptance in [issue #2](https://github.com/JCFrags/HaloFPX/issues/2).

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
