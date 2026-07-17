---
section_id: "67"
title: "Configuration and Manifest Sources"
status: "verified"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp", "charlie12345/ROCmFPX", "fewtarius/CachyLLama", "local Agent_Harness"]
  software_versions: ["commits and standards below"]
  hardware_revisions: []
related_sections: ["18", "29", "47", "49", "60"]
---

# Sources

## S67-01 — llama.cpp server documentation

- URL/revision: https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/server/README.md; commit `788e07dc91d266ad3162a1ce9037665656269689`, 2026-07-17 UTC; accessed 2026-07-16.
- Supports: CLI/env options, router model sources, preset and multi-file layout behavior.
- Limitations: upstream defaults are not HaloFPX precedence or distributed manifests.

## S67-02 — ROCmFPX README

- URL/revision: https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/README.md; commit `a5605a72768c6562241b248e268e33dc92787394`, 2026-07-17 UTC; accessed 2026-07-16.
- Supports: AMD/backend-specific build/runtime inputs and qualified tuning claims.
- Limitations: self-documentation and local measurements; not a hardware profile schema.

## S67-03 — CachyLLama README

- URL/revision: https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/README.md; commit `6be745998f568e379ea197fcf827baec73ff9940`, 2026-07-09; accessed 2026-07-16.
- Supports: cache/user/tuning runtime flags and their declared defaults.
- Limitations: fork-specific claims; imported behavior requires source audit.

## S67-04 — JSON Schema Core 2020-12

- Publisher/URL: JSON Schema project/IETF draft, https://json-schema.org/draft/2020-12/json-schema-core
- Revision/access: draft-bhutton-json-schema-01, 2022-06-16; accessed 2026-07-16.
- Supports: schema identifiers, vocabularies, validation model, instance equality.
- Limitations: work-in-progress informational draft; implementation vocabulary support must be pinned.

## S67-05 — RFC 8785 JSON Canonicalization Scheme

- Publisher/URL: RFC Editor, https://www.rfc-editor.org/rfc/rfc8785.html
- Revision/access: RFC 8785, June 2020; accessed 2026-07-16.
- Supports: deterministic JSON canonicalization for repeatable hash input.
- Limitations: canonicalization does not decide which semantic fields belong in a compatibility key.

## S67-06 — Agent Harness architecture

- Path/revision: `C:\Users\britt\Documents\Agent_Harness\guide\architecture.md`; local working-tree authority accessed 2026-07-16.
- Supports: provenance, deterministic validation, lifecycle, review, and reversible promotion.
- Limitations: local conceptual authority; no executable HaloFPX schema.

