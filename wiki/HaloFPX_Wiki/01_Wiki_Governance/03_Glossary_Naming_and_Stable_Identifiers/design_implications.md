---
section_id: "03"
title: "Glossary and Naming Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: []
  hardware_revisions: []
related_sections: ["01", "04", "05", "43", "49", "57"]
---

# Design implications

## Stable identifier namespaces

| Pattern | Record |
|---|---|
| `HLX-SEC-NN` | Wiki section (maps to global numeric ID) |
| `HLX-CLM-NN-NNN` | Claim |
| `HLX-SRC-NN-NN` | Source record |
| `HLX-ASM-NNNN` | Assumption |
| `HLX-OQ-NNNN` | Open question |
| `HLX-ADR-NNNN` | Decision record |
| `HLX-EXP-YYYYMMDD-NNN` | Experiment definition |
| `HLX-RUN-<UTC-basic>-<short-random>` | One execution/run |
| `HLX-MODEL-sha256-<64hex>` | Exact model bytes |
| `HLX-BUILD-<git12>-<config12>` | Source/config build identity |

**[RECOMMENDATION]** IDs use uppercase ASCII type prefixes, digits, and hyphens. Files use lowercase ASCII `snake_case`; enum values use lowercase `kebab-case`; environment variables use uppercase `HALOFPX_*`. Never put mutable titles, hostnames, secrets, or local drive letters in stable IDs.

**[VERIFIED]** RFC 8141 defines URNs as intended persistent, location-independent identifiers [S03-08]. **[INFERENCE]** HaloFPX does not need to register a URN namespace now; the important transferable property is separating identity from location.

## Concrete naming rules

- Physical nodes: inventory IDs `node-01`, `node-02`; user-friendly aliases may coexist.
- Ranks: integer `rank_id` starting at `0`, plus role enum; never derive rank identity solely from hostname.
- Links: `usb4-link-01`, `usb4-link-02`; interface names are observations, not stable identity.
- Experiment folders: `HLX-EXP-YYYYMMDD-NNN_<short_slug>/`.
- Run timestamp: UTC RFC 3339 in metadata; filename form `YYYYMMDDTHHMMSSZ` [S03-09].
- Checksums: lowercase algorithm-qualified form `sha256:<64hex>`.
- Model display name is metadata only; exact identity is bytes plus relevant tokenizer/template artifacts.

## Compatibility IDs

**[RECOMMENDATION]** Compute a compatibility manifest from canonical, explicitly typed fields, then hash its canonical serialization. At minimum consider model bytes, tokenizer, chat template, runtime commit/build, backend, cache format/ABI, tensor placement, K/V types, context parameters, RoPE settings, and distributed world/rank layout. The owning cache section must determine the final set through correctness tests.

## Alias handling

Aliases are searchable but never canonical. Record `alias`, `canonical_term`, `source_scope`, `valid_from`, and `deprecated_at`. Ambiguous aliases such as "TP", "cache", "memory", "GPU split", and "bandwidth" are prohibited in normative text unless expanded locally.
