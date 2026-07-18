---
type: experiment-record
status: candidate
experiment: P63-00
date: 2026-07-18
implementation_repository: C:\Users\britt\Documents\HaloFPX
implementation_parent: 214a3432f6df862c1fb81da5cf46aeea65eed092
---

# P63-00 HaloFPX publication model checking

The final TLC candidate is
`runs/2026-07-18-l05-model-final-v05/`. Its manifest SHA-256 is
`35069f6bfe387aa15218a8c3040e2dac678030cb7268346be3687bb08058ca7a`.
The exact model source SHA-256 is
`320d294949624469a5c636fc510300f6f558845094139402a6845d765b1c38fe`.

The candidate contains 17 contract-checked TLC runs: 12 exhaustive positive
safety/liveness outcomes and five required negative counterexamples. Aggregate
generated/distinct states are 44,539,476 / 5,968,128. The raw directory
contains 160 files totaling 126,359,264 bytes; 159 outputs are recursively
length/hash-declared and the manifest is the final file.

The secondary checker evidence is `runs/2026-07-18-l05-apalache-v05/`, manifest
SHA-256 `cf57171d9cb1df719c9f07a5093cdfab5aa68c43c877d673d3d030c239bc17d6`.
Apalache `v0.57.0` passed a full typecheck and an independent bounded `Safety`
check through computation length 5 against the same exact source hash.

Earlier TLC `candidate-v01` through `candidate-v04`, Apalache `v01` through
`v04`, and `development-artifacts/` are retained development evidence. They
record runner/checker defects, superseded model revisions, and intentionally
non-promoted state-space attempts. None is a promoted result.

Independent acceptance opens only implementation of a disabled, offline,
target-native writer and fault harness. Persistent writes, server lookup/hits,
and canaries remain closed until the concrete crash/space/I/O/device/rollback
and machine gates pass. This experiment makes no C++ conformance, filesystem,
device, power-loss, or performance claim.
