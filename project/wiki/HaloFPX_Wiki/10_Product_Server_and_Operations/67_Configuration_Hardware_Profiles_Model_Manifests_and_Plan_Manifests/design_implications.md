---
section_id: "67"
title: "Manifest Schemas and Deployment Examples"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project"]
  software_versions: []
  hardware_revisions: ["two-node Strix Halo target"]
related_sections: ["18", "29", "38", "47", "49", "60", "66", "68"]
---

# Manifest schemas and examples

## Required manifest fields

| Artifact | Required identity and content |
|---|---|
| Service config | `schema_version`, bind/auth references, paths, quotas, observability, default model/objective, allowed plans |
| Hardware profile | profile ID/version, node ID, CPU/GPU/memory, firmware/OS/kernel/drivers, backend devices, NVMe, ports/links, measured limits, evidence timestamp |
| Model manifest | stable model ID/version, file SHA-256/size, architecture, GGUF metadata, tokenizer/template hashes, quantization, license/source, shards, contexts/backends, quality evidence |
| Shard manifest | complete ordered files and hashes, rank ownership/ranges, replication, expected aggregate hash |
| Plan manifest | plan ID/version, compatibility inputs/hash, mode, ranks/devices, layer/tensor/expert placement, transport, memory reservations, cache policy, scheduler, fallbacks, evidence/expiry |

## Compatibility key

**[RECOMMENDATION]** Include schema versions, binary/patch hash, model/shard/tokenizer/template hashes, backend and state-format version, hardware-profile compatibility class, rank plan, cache/wire protocols, and all parameters that change persisted or distributed semantics. Exclude paths, timestamps, secrets, logging level, and presentation-only labels.

## Single-node example

```yaml
schema_version: halofpx.plan/v1
plan_id: qwen-coder-vulkan-single-a
model_id: qwen-coder-primary
mode: single
ranks: [{node: node-a, device: Vulkan0, role: owner}]
memory: {model_bytes: measured-required, kv_bytes: measured-required, reserve_bytes: measured-required}
cache: {scope: rank-local, durability: persistent, format_version: pending}
fallbacks: []
evidence: {status: candidate, benchmark_id: pending, expires: pending}
```

## Two-node example

```yaml
schema_version: halofpx.plan/v1
plan_id: qwen-coder-tp2-usb4
model_id: qwen-coder-primary
mode: tensor_parallel_2
ranks:
  - {rank: 0, node: node-a, device: Vulkan0, role: coordinator-owner}
  - {rank: 1, node: node-b, device: Vulkan0, role: worker}
transport: {profile: usb4-pair-v1, policy: auto, protocol_version: pending}
memory: {per_rank_reservation: measured-required}
cache: {scope: rank-local, migration: forbidden}
fallbacks: [qwen-coder-vulkan-single-a]
evidence: {status: candidate, benchmark_id: pending, fault_test_id: pending}
```

`measured-required` and `pending` intentionally prevent these examples from being runnable defaults.

## Redaction and explainability

- `config validate`: schema, references, compatibility, collision, and secret-reference checks.
- `config explain`: each effective field, value, source layer, and whether it affects compatibility hash.
- `config dump --redacted`: normalized effective configuration with secret references retained but values removed.
- `plan inspect`: reservations, ranks, transport, fallbacks, evidence, expiry, and rejection reasons.

