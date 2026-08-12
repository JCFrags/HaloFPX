# Raw Evidence Layout

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.



This directory intentionally contains no machine measurements in the distributed design bundle. The `examples/synthetic-non-machine/` records are invented parser tests and cannot satisfy a release gate.

```text
raw-data/
  <run-id>/
    manifest.json
    requests.jsonl
    tokens.jsonl
    correctness.jsonl
    faults.jsonl
    telemetry/
      node-a.jsonl
      node-b.jsonl
      client.jsonl
    logs/
      coordinator.log
      worker.log
      kernel-node-a.log
      kernel-node-b.log
    provenance/
      commands.txt
      packages.txt
      hardware.json
      software.json
      usb4-before.json
      usb4-after.json
    derived/
      request-summary.json
  upstream/
    events.jsonl
    watch-state.json
  baselines/
    <baseline-id>.json
```

Raw data should be stored on a filesystem that supports atomic rename. Finalize into a read-only evidence packet, calculate `sha256sum`, and sign or attest the packet before release review.
