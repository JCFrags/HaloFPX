# HaloFPX Project Objectives

## Product objective

Deliver a maintainable fork of `charlie12345/ROCmFPX` that preserves its Strix
Halo advantages while adding safe SSD-backed persistent context state and
selected compatible capabilities derived from the reviewed CachyLlama/llama-ai
requirements.

## Performance objective

Make HaloFPX the best measured Strix Halo inference engine for its supported
ROCmFPX GGUF model matrix. The pinned 160 GB
`saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf` artifact across
nimo-1 and nimo-2 is the largest current stress fixture, not the model-specific
optimization target. Prompt processing and generation are separate metrics.
Generation above 30 tokens/s for that exact fixture is a stretch objective,
not a general product gate.

## Ordered end state

1. A stable single-node-capable ROCmFPX fork with feature-off equivalence.
2. A safe, default-off persistent-cache product canary through the real server.
3. A usable persistent-cache mode with corruption-as-miss, rollback, bounded
   storage, explicit scope, lifecycle controls, and retained cold fallback.
4. Matched exact-model comparisons against named compatible engines.
5. Evidence-selected single-node and dual-node performance improvements with no
   accepted baseline regression.
6. Only after the fork is stable, sustained work on two-node execution speed
   for the 200–230 GB model class over dual USB4.

## Non-negotiable gates

- New behavior remains default-off until its correctness and rollback gates pass.
- Corrupt, incompatible, incomplete, or unauthorized state is a miss/recompute.
- No accepted performance lane may be slower under matched repeated controls.
- Model artifacts, source revisions, binaries, runtime tuples, and results use
  exact identities and retained evidence.
- GPL llama-ai implementation does not enter the intended MIT engine.
- Rejected experiments are removed; narrowly justified generic repairs may stay.
- Universal “fastest” claims require a named, versioned comparator matrix.
- The known-good endpoint remains available or is restored before a milestone closes.
