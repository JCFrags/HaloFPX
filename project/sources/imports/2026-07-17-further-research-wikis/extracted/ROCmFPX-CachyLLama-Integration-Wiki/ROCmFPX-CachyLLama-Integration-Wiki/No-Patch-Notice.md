---
title: No Patch Notice
description: Scope statement explaining why no implementation patch is included.
status: Final for this package
evidence-date: 2026-07-17
canonical-repository: charlie12345/ROCmFPX
---

# No Patch Notice

> [!NOTE]
> Evidence is pinned to **2026-07-17**. Repository heads and source links are locked in [[Source-Register]] and `evidence/commit-lock.json`.


## Deliberate exclusion

This package contains **no implementation patch**. It contains no `.patch` or `.diff` file, no copied donor source, and no generated code intended to compile.

## Why

Repository-level license compatibility is established for the two engine forks, but implementation provenance is not complete at the capability-commit level:

- the requested donor repository name differs from the resolved public repository;
- the donor head is a broad merge of upstream work and donor additions;
- reviewed feature documents often identify behavior or commit titles without locking exact introduction SHAs;
- the parent project is GPL-3.0 and must remain outside MIT source ports;
- canonical cache/server/MTP code has materially diverged, making broad textual imports unsafe.

[S11] [S13] [S19] [S23] [S29] [S30]

## When an implementation patch becomes permissible

A patch may be produced only after:

1. a maintainer confirms the resolved donor identity;
2. the relevant `PROVENANCE-RECORD` reaches P3;
3. the treatment decision is approved;
4. the lane base and rollback tag are frozen;
5. required contract tests exist;
6. the patch contains attribution and license notices;
7. the patch is generated against canonical ROCmFPX, not by merging donor master.

## Non-implementation snippets

Shell commands, Mermaid diagrams, file-layout examples, and behavioral pseudocode in this wiki are process/design material. They are not source patches and do not claim to be buildable implementation code.


---

**Wiki navigation:** [[Home]] · [[Executive-Decision]] · [[Capability-Decision-Matrix]] · [[Patch-Lanes-and-Dependency-Graph]] · [[Acceptance-Criteria]]
