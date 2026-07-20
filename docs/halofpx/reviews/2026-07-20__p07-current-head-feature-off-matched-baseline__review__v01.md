# P07 current-HEAD matched feature-off baseline independent review

Date: 2026-07-20

Verdict: **ACCEPT after rollback-incident disclosure correction; final G9/G10 remains open**

## Scope and reconciliation

Independent review recomputed all 20 retained raw samples from the nimo-2
response and curl files. The recomputed means, sample deviations, deltas, and
approximate normal confidence intervals exactly match the analyzer, milestone
document, and receipt. All retained requests are HTTP 200 with 1129 prompt
tokens, 128 generated tokens, and the admitted exact decoded-content SHA-256.

The review verified that only `control-b1`, `candidate-b2`, `candidate-b3`, and
`control-b4` enter the C-A-A-C result. System-Clang runs and failed compiler-pin
attempts remain explicitly excluded diagnostics. The final AMD ROCm HIP
compiler identity, configuration, source archive, request, control and candidate
binary hashes, node manifests, and bundles reconcile with retained evidence.
Both Linux scripts pass Bash syntax checks, the Python analyzer reproduces the
sealed analysis byte-for-byte, the receipt parses as JSON, and `git diff
--check` passes.

## Required correction and closure

The initial draft disclosed only the downstream nimo-1 coordinator abort during
rollback restoration. Review found the material initiating event in the nimo-2
post-restore journal: the kernel OOM-killed the RPC worker, after which nimo-1
aborted in `ggml_backend_rpc_buffer_set_tensor`. Both services restarted once;
the second load reached HTTP 200 health.

The milestone document and receipt now state the OOM kill, downstream RPC
abort, one restart per service, and successful second load. No rerun is required
because the event occurred after the admitted matrix and is retained as a
rollback-memory-pressure reliability defect rather than a throughput result.

## Promotion boundary

P07 is accepted as a bounded, risk-proportionate feature-off baseline harness.
It establishes exact compiler authority, deterministic output equality, raw
matched performance evidence, and recoverable deployment restoration. Slightly
adverse point estimates and intervals crossing zero do not close strict final
non-inferiority. No speedup, universal superiority, persistent-feature
enablement, L14Q promotion, or zero-restart rollback claim is authorized.
