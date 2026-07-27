# L54 warmup lineage and arm-state discriminator

Status: **NOT PROMOTED**
Date: 2026-07-27
Base: `20af537f0d36d9de3877af860e4f24d89d7e2641`

## Result

L54 corrected the retained L52 chronology before altering the runtime. The
ordinary RPC graph belongs to common warmup. The authenticated sequence-1
graph belongs to the immediately following first 512-token prompt chunk.
The coordinator's “removing memory module entries” message is decode-failure
cleanup, not the common warmup's explicit cache clear.

Consequently, the retained evidence does not show an unarmed warmup failure,
unexpected warmup arm transition, or graph authentication while unarmed. It
shows successful ordinary warmup followed by a failure in the first explicitly
armed prompt execution after the remote graph executed successfully.

L54 explicitly forbids prompt execution. A common-warmup-only run cannot reach
that failure boundary and therefore cannot provide the decisive classification
required by the milestone. Adding the requested trace and consuming the one
runtime would only establish warmup success, leaving the known armed boundary
unobserved. L54 closes **NOT PROMOTED** without speculative instrumentation or
runtime execution.

## Lineage finding

The preserved source archive is
`C:\Users\britt\Documents\HaloFPX-L52-source.tar`, SHA-256
`e4ad1ff9929a94a56bd7dbab104b5ff9560b603cbf6a8cc1cf7b7e650521e557`.
Its canonical 4,562-file tree root is
`4bd4bc49cebdf543f39213c2f90e26a422f074d9c9b39c766367af82c6cb1d0d`.

L52 independently bound selected source hashes and the exact worker/canary
binary hashes. It did not embed the source root in those binaries, so exact
build derivation is not cryptographically proven. This is an admitted lineage
coverage gap, not evidence that a mismatch occurred.

## Safety and next authority

No source candidate was retained. No build, remote command, disposable unit,
model load, primary access, prompt, capture, restore, or production mutation
occurred. The worktree contains only this reviewed closeout and evidence.

The smallest useful future discriminator must permit exactly the first armed
prompt chunk, with compile-time provenance and authenticated arm/scheduler/
L42/L44/RPC/client-return evidence. No semantic correction is authorized or
recommended before that evidence.
