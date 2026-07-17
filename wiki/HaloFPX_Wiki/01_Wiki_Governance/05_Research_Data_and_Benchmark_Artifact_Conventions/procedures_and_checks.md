---
section_id: "05"
title: "Research Data and Benchmark Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: []
  hardware_revisions: []
related_sections: ["02", "03", "04", "70", "71", "72", "73", "74", "75", "76"]
---

# Procedures and checks

## Before a run

Prerequisites: approved non-destructive protocol, sufficient storage, exact model/source access, and known root requirements.

1. Allocate experiment/run IDs and state hypothesis, independent variable, fixed keys, metrics, repetitions, warm-up, failure/exclusion rules, and acceptance criteria.
2. Hash model, tokenizer/template, prompt set, configs, and executables.
3. Capture source commit/dirty state, build flags, OS/kernel, firmware, ROCm/driver, backend, power/thermal policy, storage, topology, and time synchronization.
4. Record exact argv as an array and an allowlisted environment. Redact secrets before persistence.
5. For distributed runs, capture rank ownership, world size, both links, routing, transport settings, failure behavior, and fallback mode.

## During and after a run

1. Preserve stdout and stderr separately; record exit code and start/end UTC.
2. Emit machine-readable per-sample metrics; retain failures and timeouts.
3. Finalize checksums before deriving results; mark the raw bundle immutable.
4. Generate normalized tables and plots only from hashed raw inputs.
5. Run the same analysis script/config for compared groups.
6. State sample count, central tendency, dispersion/interval, and every exclusion.
7. Review whether environmental drift or thermal state invalidates comparison.

## Safe checksum commands

```powershell
# Windows PowerShell; no elevation required.
Get-FileHash -Algorithm SHA256 -LiteralPath '<artifact>'
```

```bash
# Linux; no root required.
sha256sum -- '<artifact>'
```

## Required two-node validation experiments

1. Collector repeatability: two no-op environment captures on each node produce only expected timestamp/volatile-field differences.
2. Matched baseline: run the same pinned single-node workload on each host; report variability without assuming equivalence.
3. Dual-link topology capture: verify the manifest maps logical links to actual interfaces/routes.
4. Integrity failure: corrupt a copy of a raw artifact and confirm validation fails; never alter the preserved original.
5. Restore drill: retrieve one external large artifact from its pointer and verify size/hash.
6. Analysis regeneration: delete only a disposable derived copy, regenerate it from raw inputs, and compare checksums or documented nondeterministic fields.

Root access may be required for selected counters/topology inspection; each experiment must declare it. No benchmark should silently change system tuning.
