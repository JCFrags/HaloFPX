---
section_id: "84"
title: "On-Machine Research Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["Custom_Inference_Project", "HaloFPX integration repository (not yet frozen)"]
  software_versions: []
  hardware_revisions: ["two matched AMD Strix Halo hosts; exact inventory pending"]
related_sections: ["04", "05", "18", "20", "22", "24", "25", "29", "38", "55", "65", "73", "75", "76", "80", "81", "85"]
---

# On-machine research open questions

| ID | Question/gap | Resolution evidence | Owner links |
|---|---|---|---|
| `HLX-OQ-8401` | What are the exact BOM, BIOS/firmware, USB4 port/cable/controller, NVMe, cooling, power-policy, and stable host/rank/link identities? | E01 repeatable inventory and topology graph | Sections 18-23 |
| `HLX-OQ-8402` | Which local monotonic clocks, PTP hardware clocks, NTP/PTP stack, offset, drift, and uncertainty are available on both USB4 paths? | E01/E02 pre/post calibration and capability records | Sections 20, 73, 75 |
| `HLX-OQ-8403` | Which telemetry fields are stable and what collector cadence/overhead and wall-power relationship are acceptable? | E02 collector-off/on and calibration campaign | Sections 22, 27, 73, 79 |
| `HLX-OQ-8404` | Are the two nominally matched hosts statistically interchangeable, and which HIP/Vulkan/model/quantization cells are correct and viable? | E03 paired per-host correctness and baseline matrix | Sections 24-37, 74, 78 |
| `HLX-OQ-8405` | Are the two USB4 rails independent under simultaneous bidirectional GPU-path load, and where do striping/alternation/hedging help tails? | E04 matrix plus E10 holdout | Sections 20, 49-55, 75 |
| `HLX-OQ-8406` | Which mapped, registered, staged, or graph-reused buffer paths actually preserve visibility and reduce end-to-end cost? | E05 litmus and copy-attribution evidence | Sections 24, 25, 32, 45, 54 |
| `HLX-OQ-8407` | What complete state/fingerprint is necessary for exact transformer, recurrent, MTP, speculative, and sampled continuation? | E06 mutation and suffix-only oracle | Sections 56-65, 77 |
| `HLX-OQ-8408` | Which distributed mode wins in which workload region after correctness, memory, energy, cache, and fabric tails are included? | E07 paired break-even regions and E10 holdout | Sections 38-48, 76 |
| `HLX-OQ-8409` | What concurrency, long-context, thermal, cache-churn, and service duration define the supported operating envelope? | E08 stress/soak with predeclared drift and fairness rules | Sections 66-72, 79 |
| `HLX-OQ-8410` | Which destructive fault mechanisms are safe on the actual machines and what rollback/out-of-band controls exist? | operator-approved E09 preflight and dry run | Sections 71, 72, 80, 83 |
| `HLX-OQ-8411` | What independent-run/sample budget, uncertainty method, holdout design, and minimum effect define an ADR or release claim? | E02 variance pilot plus E10 protocol approved before results | Sections 05, 73, 81 |
| `HLX-OQ-8412` | Which upstream revisions or cross-project results invalidate cards, schemas, or thresholds, and how are reruns triggered? | Section 85 watch rules tied to card applicability/review triggers | Sections 04, 11-16, 23, 29-37, 85 |

## Internet follow-up

1. Re-pin linuxptp, Linux clock/timestamp documentation, MLCommons rules, ROCm/AMD SMI, Mesa/RADV, and target repositories when implementation begins; record exact commits and material changes.
2. Verify whether the actual USB4 interfaces and drivers advertise hardware timestamping and whether linuxptp supports their transport path; documentation alone cannot answer this.
3. Freeze the statistical/serialization libraries and inspect exact quantile, bootstrap, timestamp, JSON-schema, and canonical-hash behavior.
4. Track upstream backend operation coverage, ROCm/Mesa kernel fixes, USB4STREAM work, cache/checkpoint formats, speculative/MTP support, and security advisories in Section 85.

## Cross-project research gaps

- Hardware/OS sections need to turn desired inventories into actual immutable E01 records.
- Performance-tool sections need reviewed probes and a measured perturbation budget before their counters can be release evidence.
- Model sections need a frozen corpus/oracle and exact model/tokenizer hashes; quality thresholds remain product decisions.
- Distributed and transport sections need one shared event schema and failure vocabulary to avoid incompatible traces.
- HaloKV sections need disposable test fixtures covering transformer, recurrent, MTP, speculative, sampling/RNG, tenant, and topology state.
- Product/operations sections need approved SLOs, privacy-safe representative workloads, and recovery semantics before pass/fail thresholds can be set.
- Governance sections need populated stable question/ADR/issue records; Section 84 can link them but must not invent accepted decisions.

**[OPEN]** These gaps block architecture finalization. A completed source review or successful command does not close them without the specified raw machine evidence.
