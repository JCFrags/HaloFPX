# P04 current-HEAD feature-off requalification independent review

Date: 2026-07-20

Verdict: **ACCEPT after metric correction; final G9/G10 remains open**

## Scope and evidence

Independent review recomputed all retained throughput and latency statistics from the raw nimo-2 responses and curl records. It checked admitted response counts and hashes, the disclosed setup failures, two-subflow MPTCP and nonzero dual-rail evidence on both nodes, exact build and source identities, node manifests and bundles, rollback health, feature-off gates, provenance and licensing boundaries, and the five immutable reference repositories.

The review confirmed 16 admitted HTTP-200 requests, six retained samples per variant, exact 1129-token prompt and 128-token continuation counts, and byte-identical decoded output across control and candidate. Throughput statistics, interface arithmetic, binary/source hashes, verified manifests, active zero-restart rollback services, direct nimo-1 health, and clean reference commits/trees reconcile with retained evidence.

## Required correction and closure

The initial draft labeled `prompt_ms + predicted_ms` as end-to-end latency. Review returned REVISE because those fields are engine-reported phase timings, not request wall time. The corrected report and receipt use retained curl `time_total`:

- control: 13227.2325 +/- 13.5419 ms;
- candidate: 13231.4487 +/- 16.0385 ms;
- candidate delta: +4.2162 ms, or +0.03187%;
- approximate Welch 95% interval: -14.9508 to +23.3831 ms.

The correction is internally consistent and changes no conclusion. `git diff --check` and JSON parsing pass.

## Promotion boundary

P04 is accepted as a bounded feature-off requalification milestone. It establishes deterministic output equality, correct dual-subflow execution, clean rollback, and no statistically demonstrated regression. Its slightly adverse point estimates and intervals crossing zero do not satisfy strict final non-inferiority, so G9/G10 remains open. No speedup, universal superiority, transport promotion, or persistent-feature enablement is authorized by this result, and no test expansion is required for this milestone.
