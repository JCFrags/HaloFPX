# P06f ROCmFPX Q6 expanded-device view-offset review v01

Status: **ACCEPT for commit**

## Scope and verdict

This independent read-only review examined the HIP buffer-view correction,
focused full/global-ID versus nonzero-view/local-ID oracle, CMake integration,
P06e regression authority, retained direct/RPC evidence, inherited smoke,
rollback, claims, and provenance. It added no test permutations and changed no
files.

The packed-to-expanded conversion is correctly limited to expanded-device
`Q6_0_ROCMFPX` views. Core ggml view construction flattens nested views to the
base tensor with an absolute `view_offs`; its existing logical packed bounds
check remains authoritative. A mismatched type or non-block-aligned offset
fails safely. Generic RPC serialization and compute kernels are unchanged.

The oracle validly compares the same Q6 expert data and activations through
full/global-ID and nonzero-view/local-ID routes. Retained evidence reconciles
NMSE 0 and maximum absolute error 0 for both direct HIP and RPC, 73/73 inherited
Q6 `MUL_MAT_ID` cases, byte-identical node binaries, verified manifests, and a
healthy rollback. The wrong-endpoint attempt is correctly excluded because it
aborted during RPC initialization before backend creation, allocation, or graph
execution.

Claims correctly stop before performance, physical sharding, generic RPC, or
exact-model rank-local execution. The implementation is target-native and has
no donor, GPL, persistence, dependency, or distribution-boundary impact.

## Nonblocking debt

- No separate pre-patch synthetic oracle result was retained; P06e's exact-model
  zero-peer failure and source analysis remain the regression authority.
- Unaligned, retyped, and nested-view negative permutations are deferred.
- The exact 96-expert rank-local canary remains the next milestone.

These items do not block this risk-proportionate backend-correction milestone.

**Final verdict:** accept for commit.
