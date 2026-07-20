# L14Q-H03 HIP quantized-KV early-dispatch review v01

Status: **ACCEPT for a default-off experiment after documentation correction**

## Scope

This independent read-only review examined the H03 source change, matched-build
helper, milestone document, receipt, existing H01/P3 authority, retained node
evidence, rollback state, and provenance boundary. It did not add trials or
modify services.

## Findings

1. The original milestone text said the screen satisfied a "predeclared retain
   condition," but no committed H03 artifact predeclared that rule. The text now
   says the results support retaining H03 as an experiment. This resolves the
   only commit-blocking documentation finding.
2. Canonical preparation Wiki section 33 still names the older `37ff5e4f` /
   L14Q-T01 state and says runtime optimization is pending. The H03 document now
   records this as release-documentation debt. It does not block a default-off
   experimental milestone, but it must be reconciled before final release.
3. Some archived filenames contain trailing carriage-return characters. The
   manifests and bundles verify, so this is non-blocking evidence hygiene rather
   than an integrity failure.

## Verification and verdict

The early return is compile-gated and reuses H01's exact eligibility. It occurs
after the existing Turbo batched and fused-VEC decisions. Standard symmetric
Q8_0/Q4_0 decode with admitted dimensions and GQA can enter; ROCmFPX, ROCmFP4,
TurboQuant, Vulkan, prompt, ineligible, and feature-off routes remain excluded
or unchanged. The helper's fourth argument defaults to `OFF` and accepts only
`ON` or `OFF`.

All 18 retained requests reconcile with the recorded hashes, counts, summaries,
manifests, and bundles. Every confidence interval crosses zero; the no-speedup,
no-final-non-inferiority wording is correct. Both restored services and all five
immutable reference clones were independently rechecked. H03 remains within the
approved H01 target-native provenance boundary and imports no donor expression.

**Verdict:** accept for commit and retention as a default-off experiment. Do not
enable it by default or claim speedup or final non-inferiority from this screen.
