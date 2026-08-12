---
section_id: "67"
title: "Configuration, Hardware Profiles, Model Manifests, and Plan Manifests"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project", "ggml-org/llama.cpp", "charlie12345/ROCmFPX", "fewtarius/CachyLLama"]
  software_versions: ["llama.cpp 788e07d", "ROCmFPX a5605a7", "CachyLLama 6be7459"]
  hardware_revisions: ["two matched AMD Strix Halo systems; exact profiles open"]
related_sections: ["03", "09", "18", "23", "29", "30", "38", "47", "49", "60", "66", "68", "70", "72"]
---

# Configuration and manifest contract

**[RECOMMENDATION]** HaloFPX configuration should be declarative, schema-validated, versioned, fully explainable, and divided into four immutable inputs:

1. hardware profile—what a node and fabric actually are;
2. model manifest—what exact artifacts and semantics are admitted;
3. plan manifest—how that model runs on that hardware/workload;
4. service configuration—policy, paths, API, security, and defaults.

The runtime should emit a redacted effective configuration and a compatibility hash before loading a model. Request hints may select among admitted plans but cannot override safety, identity, memory, or compatibility gates.

See [source constraints](facts_and_constraints.md), [schema and examples](design_implications.md), and [validation procedure](procedures_and_checks.md).

## Research split

- Internet research established upstream flags/environment variables and standards for schema validation/canonical JSON.
- Machine work must generate exact node/link/storage profiles and prove plans against measured limits.
- Contingent decisions include file format, secret provider, override allowlist, migration support window, and hash field set.

