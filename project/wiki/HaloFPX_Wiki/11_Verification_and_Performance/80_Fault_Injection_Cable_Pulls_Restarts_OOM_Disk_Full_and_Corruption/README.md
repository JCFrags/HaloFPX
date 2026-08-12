---
section_id: "80"
title: "Fault Injection: Cable Pulls, Restarts, OOM, Disk Full, and Corruption"
status: needs-machine-validation
last_verified: 2026-07-17
applies_to:
  repositories:
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
  software_versions: ["Linux fault-injection documentation accessed 2026-07-17"]
  hardware_revisions: ["dual-Strix-Halo target; exact revisions open"]
related_sections: ["54", "56", "62", "65", "68", "76", "77", "78", "79", "81"]
---

# Fault Injection: Cable Pulls, Restarts, OOM, Disk Full, and Corruption

Fault testing exists to prove fail-closed behavior, bounded recovery, and usable diagnostics—not merely that a process restarts. The overriding invariant is: no injected fault may produce silently wrong inference or cause corrupt, stale, partial, or mismatched state to be accepted.

This section is a proposed destructive-test plan. None of these injections were performed during research. Run only on an isolated test deployment with copied models, disposable cache/storage, out-of-band access, explicit targets, and a rehearsed reset path.

Distributed recovery must state rank ownership and state completeness. A single-node fallback is valid only when the surviving node holds all required model and request state; otherwise the request fails explicitly and may be retried from a known boundary.

See [procedures_and_checks.md](procedures_and_checks.md) for the staged matrix and safety controls.
