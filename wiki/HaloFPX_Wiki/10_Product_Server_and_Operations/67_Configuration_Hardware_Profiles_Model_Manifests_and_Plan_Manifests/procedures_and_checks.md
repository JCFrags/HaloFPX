---
section_id: "67"
title: "Configuration Generation and Validation"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project"]
  software_versions: ["schema/tool version under test"]
  hardware_revisions: ["actual node A/node B deployment"]
related_sections: ["18", "23", "29", "47", "49", "60", "68", "70", "72"]
---

# Configuration generation and validation

## Internet/source follow-up

1. Inventory pinned upstream CLI flags, environment variables, defaults, deprecated aliases, and config-bearing files.
2. Map each imported option to a HaloFPX schema field, an intentional pass-through, or an explicit unsupported item.
3. Freeze JSON Schema dialect and canonicalization/hash algorithms; retain conformance vectors.
4. Review migrations and renamed upstream options at every rebase.

## Machine profile procedure

Prerequisites: both nodes booted in the intended topology. Read-only inventory needs no root for most fields; firmware, topology, and privileged driver settings may require root and must be recorded.

1. Collect stable hardware IDs/revisions, BIOS/firmware, memory, devices, NVMe, USB4 ports, OS/kernel/drivers/backends, and clock/time source.
2. Store raw command outputs as evidence; generate the profile deterministically from them.
3. Measure allocatable memory, model load peak, cache budget, link behavior, and disk limits under the exact software manifest.
4. Recollect twice and diff; classify volatile fields rather than hashing them as identity.
5. Sign or checksum the published profile and bind it to node identity.

## Manifest admission checks

1. Validate schema version and reject unknown required vocabularies/fields.
2. Verify every artifact length/hash, shard completeness/order, license/source, tokenizer/template, and GGUF metadata.
3. Resolve effective configuration and record field provenance.
4. Compute canonical compatibility JSON and SHA-256 twice in independent test implementations.
5. Check plan requirements against measured hardware, reservations, backend/model support, transport, cache/wire versions, and fallback availability.
6. Perform dry-run placement without allocating model memory, then real load/warmup acceptance.
7. Dump redacted config and scan for known secrets and filesystem credentials.

## Migration checks

Every schema migration is version-to-version, deterministic, idempotent, preserves the original, emits a diff, refuses lossy changes without explicit approval, and is tested for upgrade plus rollback. Runtime must never silently rewrite trusted manifests.

