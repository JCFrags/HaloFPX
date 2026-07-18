---
section_id: "85"
title: "Internet Research and Freshness Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-18"
applies_to:
  repositories: ["HaloFPX wiki, donor repositories, and upstream dependencies"]
  software_versions: ["observed 2026-07-17 snapshot"]
  hardware_revisions: ["exact two-node revisions pending"]
related_sections: ["02", "04", "11", "15", "18", "23", "29", "50", "62", "72", "84", "86"]
---

# Open questions

| ID | Priority | Question | Closure evidence / owner route |
|---|---:|---|---|
| OQ85-01 | P0 | What exact OEM, SKU, board, BIOS/EC/PD, USB4 controller/retimer, cable, NVMe, and firmware identities apply to both nodes? | Signed paired inventory and authoritative OEM/LVFS feed URLs; Sections 18/84. |
| OQ85-02 | P0 | Which distro/kernel/config/backport tuple provides USB4STREAM without breaking documented gfx1151 ROCm/Mesa operation? | Candidate source/packaging provenance plus EX85-01/02; Sections 23/50. |
| OQ85-03 | P0 | Who owns security/corruption triage, decision freeze, emergency validation, rollout, and unfreeze? | Accepted governance/ADR with response drill; Sections 72/86. |
| OQ85-04 | P1 | What machine-readable source-claim-section-experiment-decision dependency schema will drive impact propagation? | Schema, validator, migration of representative claims, stale-change test; Sections 02/04/86. |
| OQ85-05 | P1 | Where are raw feed snapshots, checksums, ETags, poll failures, and semantic diffs preserved, and for how long? | Storage/retention/privacy decision and recovery test. |
| OQ85-06 | P1 | What owner, credentials, rate limits, mirrors, and alert channel keep monitoring reliable offline or during API failure? | Operational runbook with simulated 403/unreachable/stale-cache behavior. |
| OQ85-07 | P1 | Which exact upstream paths/symbols/tests define the llama.cpp-to-HaloFPX semantic watch surface? | Approved map linked to Sections 12-15 and a rehearsal diff. |
| OQ85-08 | P1 | How are the ROCm 7.2.1 Ryzen-supported control, 7.2.3 research baseline, Core SDK/TheRock 7.14.0 unqualified candidate, installed tuple, and component versions compared without numeric-lane errors? | Explicit lane-aware version model and test cases covering all four roles; installed tuple evidence remains required. |
| OQ85-09 | P1 | What signature/checksum policy applies to Git tags, tarballs, packages, model weights, firmware, and containers? | Trust-root/key-retention policy and verification receipts. |
| OQ85-10 | P1 | What constitutes complete model identity and what changes force reconversion, cache invalidation, or quality revalidation? | Manifest covering weights/config/tokenizer/template/license/converter/runtime plus mismatch tests; Sections 29/31/57. |
| OQ85-11 | P1 | Which distro backports and kernel configs correspond to upstream USB4/amdgpu/io_uring commits? | Package source/SRPM or distro Git ancestry, config, build ID, and installed binary hashes. |
| OQ85-12 | P1 | Which hardware/security bulletins actually include the exact Strix Halo SKUs, and what fixed OEM firmware exists? | Product-specific bulletin mapping and signed OEM release/rollback evidence. |
| OQ85-13 | P2 | Which RCCL changes in the active `rocm-systems` subtree matter to two-host USB4 sockets or a future network plugin, and what failure semantics are usable? | Exact source audit and EX85-02 two-rank fault/correctness data; Section 24. |
| OQ85-14 | P2 | Which io_uring/liburing changes alter HaloKV cancellation, resource lifetime, direct-I/O, or late-completion safety? | API/source map and EX85-05; Section 62. |
| OQ85-15 | P2 | What cadence and false-positive budget make automated semantic triage sustainable? | Two or more monitored cycles with alert precision, review time, missed-change audit. |
| OQ85-16 | P2 | How are upstream performance reports converted into fair local experiment candidates? | Section 73/84 template with matched controls and one rehearsal; never direct promotion. |
| OQ85-17 | P3 | Which new architectures/models deserve catalog entry without turning discovery into scope creep? | Workload-fit rubric, license/fit screen, approved catalog change; Section 29. |
| OQ85-18 | P3 | Which USB-IF/PCI-SIG/specification errata are publicly accessible and relevant to the implemented Linux interfaces? | Revision/errata register with access constraints and kernel implementation mapping. |

All items are **[OPEN]**. No empty feed, unavailable proprietary erratum, issue comment, or unrun experiment is treated as a negative result.

## Immediate Internet follow-up

1. Revalidate a returned package's exact primary sources only when its candidate is selected for a build, release, security, firmware, or destructive-test decision.
2. Continue the registered freshness feeds; returned research is a dated snapshot, not a replacement for monitoring.

## Decisions contingent on closure

- Production kernel/distro, USB4STREAM/backport, ROCm/Mesa/RCCL lane, and firmware rollout.
- Initial HaloFPX source baseline and upstream synchronization cadence.
- Supported model set, conversion/cache invalidation rules, and persistent state ABI.
- Automated freshness enforcement, emergency response, signing, retention, and ownership.

No decision should be inferred from the priority order alone.
