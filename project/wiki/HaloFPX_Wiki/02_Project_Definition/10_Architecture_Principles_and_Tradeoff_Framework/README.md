---
section_id: "10"
title: "Architecture Principles and Tradeoff Framework"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project and pinned upstreams"]
  software_versions: ["commits in sources.md"]
  hardware_revisions: ["two-node Strix Halo target"]
related_sections: ["06", "07", "08", "09", "15", "38", "47", "48", "49", "60", "78", "80"]
---

# Architecture principles and tradeoffs

HaloFPX decisions follow hard safety/correctness gates, then workload-specific measurement. A higher headline tokens/s result cannot compensate for invalid state, quality regression, hidden fallback, irreproducibility, or unmaintainable divergence.

## Principle set

1. Correctness before speed.
2. Measurement before optimization.
3. Explicit rank-local ownership.
4. Graceful, observable fallback.
5. Compatibility is declared and tested.
6. Reproducibility and provenance are product features.
7. Minimize token-path work and coordination.
8. Prefer bounded, upstreamable changes.
9. Tune by model, workload, backend, and topology—not global folklore.
10. Invalid persistent state must miss or recompute.

The [evidence basis](facts_and_constraints.md), [decision framework](design_implications.md), and [decision procedure](procedures_and_checks.md) make these principles operational.

