# Provenance and Reproducibility

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


## Required run manifest

Every run records:

- Run ID, experiment/card version, profile, UTC and monotonic anchors, operator, location, ambient.
- Node identities, hardware revision, hashed serials, memory, storage, cable/port identity, wall-meter identity/calibration.
- BIOS/AGESA, firmware, OS image, kernel, boot parameters, amdgpu/KFD, ROCm/Mesa, compiler, engine commit, patches, build flags, binary/container hashes.
- Model repository/revision, shard names/sizes/SHA-256, quantization, GGUF metadata, tokenizer/template and dataset hashes/licenses.
- Full commands, environment, service units, CPU affinity/governor, GPU profile/clocks, memory/swap/zram, filesystem/mount, network/MTU/offloads/firewall/time sync.
- Cache state and preparation method, randomized run order, warm-ups, invalidation/deviation records, injected faults.
- Collector versions/configuration and raw output inventory.

## Evidence layout

```text
runs/<run-id>/
  manifest.json
  raw/
    request-trace.jsonl
    token-events.jsonl
    telemetry-node-a.jsonl
    telemetry-node-b.jsonl
    fault-events.jsonl
    correctness.jsonl
    logs/
  derived/
    summary.json
    release-evaluation.json
  MANIFEST.sha256
  SIGNOFF.md
```

Raw files are append-only and checksummed before aggregation. Derived data may be regenerated but must record tool version and input hashes.

## Reproduction definition

A result is `R1` only when an independent run block, normally on a different day with fresh boots, produces the same gate outcome and stays inside regression thresholds. Re-running a failed request in the same service process is not independent reproduction.

## Privacy and secrets

Do not store access tokens, raw private prompts, personal identifiers, or unredacted serials. Store prompt IDs/hashes and approved redacted text when content cannot be retained. Redaction must not remove fields required to reproduce tokenization.

## Machine-readable audit

The applicability checklist is [`config/provenance-requirements.yaml`](../config/provenance-requirements.yaml). Run `python tools/audit_provenance.py <run>/manifest.json --run-dir <run>` before aggregation. Schema validity alone is insufficient: the audit separately checks required non-null fields, record-type coverage, raw byte counts/SHA-256, and forbidden secret-bearing keys.
