---
section_id: "02"
title: "Evidence Policy Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: ["Git 2.x"]
  hardware_revisions: []
related_sections: ["01", "04", "05"]
---

# Procedures and checks

## Add a source

Prerequisites: lawful access to source; exact revision if versioned. Root access: not required.

1. Classify authority: source code, official document/standard, paper, official issue/PR, benchmark, or secondary report.
2. Capture title, publisher, immutable revision, publication date, access timestamp, URL/path, license, and checksum when preserved.
3. State exactly which claims it supports and any limitations.
4. Add claim-level references next to conclusions; do not cite a whole page for unrelated claims.
5. If the source conflicts with another, record both and create an open question with a resolution method.
6. Set a review trigger based on volatility, not an arbitrary promise of permanence.

## Exact Git evidence capture

```powershell
# Run in the upstream checkout; no elevation required.
git rev-parse HEAD
git status --short
git remote -v
git show -s --format=fuller HEAD
git submodule status --recursive
```

Record dirty state separately. A commit hash does not identify uncommitted changes.

## Citation review checklist

- Full commit or document revision present.
- Access date and applicability present.
- Source ID resolves and supports the nearby claim.
- Claim label matches evidence type.
- Vendor/repository claim is not presented as `[MEASURED]`.
- Quotation is necessary, short, and licensed/attributed.
- Source conflicts and deprecation are explicit.
- Machine results link to raw data and environment metadata under section [05](../05_Research_Data_and_Benchmark_Artifact_Conventions/README.md).

## Machine validation

For each checked-out upstream on both nodes, record commit, dirty state, submodules, build flags, compiler/ROCm versions, and license files. Compare the resulting snapshot with wiki applicability. Any mismatch changes affected claims to stale or needs-machine-validation; it does not silently update them.
