---
section_id: "63"
title: "Durability Modes, Atomic Commit, Crash Recovery, and Corruption Handling"
status: "needs-machine-validation"
last_verified: "2026-07-18"
applies_to:
  repositories: ["fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo"]
related_sections: ["57", "58", "59", "61", "77", "80"]
---

# 63 - Durability and recovery contract

The non-negotiable rule is: cache failure causes a miss and recomputation, never acceptance of uncertain inference state.

- **[VERIFIED]** CachyLLama can fsync checkpoint files or skip it; it writes directly to final paths and rebuilds an index by scanning [S63-01].
- **[VERIFIED]** The pinned record has magic/version/length fields but no payload checksum; directory fsync and temp-file rename commit were not identified [S63-01].
- **[VERIFIED]** The deployed ggml RPC tensor cache at `rocmfp4-llama@4860505e` uses a 64-bit FNV-derived filename, direct final-path writes, and no read-time content rehash in the inspected path [S63-L01]. It is separate from HaloKV, but demonstrates a live corruption-acceptance risk that must not be inherited.
- **[MEASURED]** nimo-1 held about 112 GiB across 187 RPC tensor-cache files while only about 43 GiB of filesystem headroom remained [S63-L01].
- **[VERIFIED]** HaloFPX `b8123fe5` adds only a disabled, offline publication-coordinator slice: exact predecessor/manifest-anchor binding, bounded ordered steps, ambiguous-anchor fail-stop handling, and a shared in-process root fence. It is not M63-01, a filesystem writer, or persistence authorization [S63-08].
- **[RECOMMENDATION]** HaloKV needs immutable data files plus a two-phase, checksummed generation manifest and explicit durability modes.
- **[OPEN]** Power-loss semantics of the selected SSD/filesystem/kernel are unmeasured.

## Research split

- **Internet/source-code research completed:** the pinned CachyLLama write path and fixed filesystem/NVMe/state references define predecessor behavior and candidate primitives only.
- **Target-machine work required:** test each advertised mode against the exact filesystem, mount, kernel, SSD firmware/write-cache, flush, rename, directory-sync, rank-failure, and recovery path on disposable data.
- **Contingent decisions:** shipped mode names, acknowledgement points, power-loss scope, quarantine retention, and whether strict durability is achievable remain unapproved until the matching fault matrix passes.
