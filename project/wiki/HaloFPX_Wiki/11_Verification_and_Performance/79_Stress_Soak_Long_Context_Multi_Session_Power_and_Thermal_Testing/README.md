---
section_id: "79"
title: "Stress, Soak, Long-Context, Multi-Session, Power, and Thermal Testing"
status: needs-machine-validation
last_verified: 2026-07-17
applies_to:
  repositories:
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
  software_versions: ["AMD SMI CLI documentation 26.5.0", "Linux hwmon and USB4 documentation accessed 2026-07-17"]
  hardware_revisions: ["dual-Strix-Halo target; exact revisions open"]
related_sections: ["73", "74", "75", "76", "77", "78", "80", "81"]
---

# Stress, Soak, Long-Context, Multi-Session, Power, and Thermal Testing

This section specifies how to find failures that short correctness and speed runs miss: resource leaks, queue starvation, cache churn, transport instability, storage saturation, thermal throttling, cancellation races, and long-context degradation. It reports no HaloFPX measurements.

Stress, soak, and maximum-context tests answer different questions. Stress intentionally drives a declared bottleneck. Soak holds a representative workload long enough to expose drift. Maximum-context testing increases the context staircase while retaining correctness and observability. A single “ran overnight” result cannot substitute for all three.

Every result must bind workload, runtime/model hashes, topology, power profile, ambient conditions, cooling state, duration, warm-up, request trace, and telemetry. Results from different power profiles are separate experiments.

See [procedures_and_checks.md](procedures_and_checks.md) for the proposed phases and [open_questions.md](open_questions.md) for hardware limits and release durations that remain unresolved.
