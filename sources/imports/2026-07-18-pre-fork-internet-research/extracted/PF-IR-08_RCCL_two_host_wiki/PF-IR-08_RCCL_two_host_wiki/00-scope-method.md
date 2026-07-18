# Scope, method, and evidence rules

> **Audit ID:** PF-IR-08  
> **Access date:** 2026-07-18  
> **Final observed authority head:** `801a9ca2ad8940ac7cd7d571163e003f3a3d6cab`  
> **Fully inspected active base:** `3dddea1389e5d28e84fe2c29f50450f00b7474a1`  
> **Stable 7.2.x release pin:** `96a25b5fd6f73fba58c7d83eb57cf19a50230aa4`

## Scope resolution

The request did not enumerate the exact local “candidate ROCm lanes.” This audit therefore covers the visible relevant release lanes **6.4.3, 7.0.2, 7.1.1, 7.2.1–7.2.4**, plus active `develop`. The lane table records exact RCCL versions and tag commits. The installed local `librccl.so` remains the operational source of truth and must be fingerprinted before testing.

## Authority handling

Active source lives under `ROCm/rocm-systems/projects/rccl`. The standalone `ROCm/rccl` repository is retired for active development but remains authoritative evidence for its release tags. The monorepo advanced during collection from `3dddea1389e5d28e84fe2c29f50450f00b7474a1` to `801a9ca2ad8940ac7cd7d571163e003f3a3d6cab`. Critical RCCL files checked at both points had identical Git blob hashes; the later head changed another project.

## Evidence hierarchy

1. Versioned public headers and plugin ABI declarations.
2. Pinned implementation source.
3. Official AMD documentation.
4. Upstream tests, limited to what their topology and assertions actually exercise.
5. Maintainer statements and PR test reports.
6. Community reports.
7. Inferences, always labeled and never promoted to machine evidence.

## Preservation model

This is a **commit-pinned evidence snapshot**, not a full repository mirror. Relevant complete small files and verbatim source excerpts are preserved with repository, path, ref, blob SHA, line locator, access date, completeness, license, and local SHA-256. Large issue/PR logs are marked partial when the connector response was truncated. No partial snapshot is represented as complete.

## Search boundary

The audit searched RCCL source, docs, tests, issues, and PRs for `gfx1151`, Strix Halo, socket interface selection, USB4, Thunderbolt, DMA-BUF, Net ABI, communicator timeout/abort/revoke/shrink/grow, remote errors, and multi-node test execution. No exact two-host Ethernet-over-USB4 upstream integration test was found in the audited corpus. That is negative evidence, not proof that no private or later test exists.
