---
section_id: "45"
title: "Persistent Rank Protocol Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["future HaloFPX implementation"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo premise"]
related_sections: ["39", "46", "48", "49", "53", "54", "75", "80"]
---

# Open questions

| ID | Question | Required evidence / owner |
|---|---|---|
| DR45-O1 | **[OPEN]** Is the logical ring transported as framed messages, mapped host memory, or another doorbell/data mechanism on each USB4 link? | sections 49, 53, 54 plus `DR-45-E1` |
| DR45-O2 | **[OPEN]** Are 128-byte commands and 64-byte completions the best frozen ABI sizes? | encoding audit, cache-line/transport measurements |
| DR45-O3 | **[OPEN]** Which head/tail/phase and atomic publication scheme is correct for the selected cross-host transport? | memory-model review and stress test |
| DR45-O4 | **[OPEN]** Which decode/prefill shapes produce stable graph keys on the pinned fork and target models? | traced graph signatures from `DR-45-E2` |
| DR45-O5 | **[OPEN]** Which HIP graph node/parameter updates are correct on gfx1151 with the pinned ROCm stack? | update allowlist with eager comparison |
| DR45-O6 | **[OPEN]** Can the chosen custom collective be graph-captured, and how is a hung collective aborted? | section 42/49 implementation and fault injection |
| DR45-O7 | **[OPEN]** What ring depth, lane count, polling policy, graph capacity, and replay window minimize tail latency within CPU/power limits? | `DR-45-E4` matched sweep |
| DR45-O8 | **[OPEN]** What progress/deadline thresholds distinguish congestion from failed rank without false fencing? | jitter, saturation, and soak data |
| DR45-O9 | **[OPEN]** What is the last jointly committed session boundary from which single-node fallback can resume exactly? | section 48 and cache/state sections 57-63 |
| DR45-O10 | **[OPEN]** Which ABI changes are additive-minor versus incompatible-major, and how long are older versions supported? | implementation/upgrade ADR and compatibility tests |

## Internet follow-up

- Track changes to `ggml-rpc.cpp`, `llama-context.cpp`, `ggml-backend.cpp`, and the HIP graph backend after the pinned commits; diff semantics, not just names.
- Pin the exact ROCm/HIP/RCCL release chosen for the machines and archive its graph/update/error documentation.
- Review the selected transport’s official memory-ordering, integrity, and reconnection contract before ABI freeze.

## Three-way status

- **Completed now:** primary-source baseline and gaps are identified.
- **Machine-required:** O1-O9 depend partly or wholly on actual topology and workloads.
- **Decision-required:** O2, O7, O9, and O10 need explicit ADRs after evidence; none is silently resolved here.
