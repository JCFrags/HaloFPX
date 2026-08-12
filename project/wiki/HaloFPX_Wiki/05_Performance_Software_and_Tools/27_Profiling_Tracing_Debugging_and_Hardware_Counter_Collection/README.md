---
section_id: "27"
title: "Profiling, Tracing, Debugging, and Hardware-Counter Collection"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "ggml-org/llama.cpp"]
  software_versions: ["ROCprofiler-SDK 1.3.2 documentation", "Linux tracing interfaces"]
  hardware_revisions: ["gfx1151 Strix Halo", "dual USB4 host links"]
related_sections: ["24", "25", "26", "28"]
---

# Profiling, Tracing, Debugging, and Hardware-Counter Collection

## Decision summary

**[VERIFIED]** `rocprofv3` can emit HIP/HSA/kernel/memory traces, KFD page and queue events, hardware-counter collections, and Perfetto/OTF2/CSV/JSON/ROCPD outputs; available counters and compatible groups are device-dependent. [S27-01][S27-02]

**[RECOMMENDATION]** HaloFPX traces must join application request/rank markers, GPU work, kernel/network/USB4 scheduling, and storage I/O under a shared experiment ID. Do not infer a distributed critical path by visually aligning unsynchronized clocks.

**[OPEN]** Tool availability, gfx1151 counters, RGP capture support, USB4 tracepoints, clock error, and profiling overhead have not been validated on the two actual hosts.

## Authoritative pages

- [Facts and tool matrix](facts_and_constraints.md)
- [Design implications](design_implications.md)
- [Capture procedures](procedures_and_checks.md)
- [Open questions](open_questions.md)
- [Sources](sources.md)

## Research split

Internet research identifies supported interfaces. Actual tool/counter discovery, trace overhead, event correlation, clock synchronization and workload profiles require both machines. Optimization decisions remain contingent on repeated unprofiled confirmation.

