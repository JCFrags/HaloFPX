# L30 stage-to-live-apply correctness independent review

Date: 2026-07-23

Reviewer: independent adversarial agent `l30_adversarial_review`

Verdict: **PASS**

No material findings remain.

The reviewer independently verified:

- the Q8_0 apply defect and correction, including the old 34-byte view versus
  the required 1,088-byte view for the 1,024-element fixture;
- checked divisibility, multiplication, nonzero, and `INT64_MAX` refusal;
- component identity coverage across ordinal, kind, type, dimensions, strides,
  view offset, size, and label digest;
- content hashes and normalized buffer ranges bound into deterministic leaves
  and a Merkle root, with control-key authentication of the exact summary;
- count, byte, ordinal, aggregate, Merkle, overlap, malformed-record, and
  authentication refusal behavior;
- applicable ROCm/CUDA copy and readback synchronization before return, with no
  unsupported asynchronous-copy correction;
- exact Q8/view RPC fixture results and the two-residency disposable model's
  capture=stage=apply aggregate and exact token result;
- 84 of 84 focused tests and a clean diff check;
- continuous production health, no primary artifact access, and complete
  disposable cleanup.

The reviewer independently recomputed the immutable raw evidence as 33 files,
3,954,831 bytes, canonical relative-path-plus-NUL-plus-content SHA-256
`b1378ea711d2186e8ebdfa49e4f7ef290ee4504a7dac64871550bd3094347a1b`.
Current source hashes match the receipt.

The first review pass found that malformed diagnostic-marker lines could be
ignored when valid complete records also existed. The accepted correction
rejects every marker-bearing line that fails the exact grammar, with focused
truncated, extra, and case-mutated tests.

Residual boundaries remain explicit: the full model fixture retains but does
not independently reauthenticate its component records after the random key is
cleaned; end-to-end authentication reconstruction is proven by the isolated
Q8/view fixtures and shared C++ path. Overlap remains fail-closed rather than
claiming alias semantics. No corrected primary restore is proven.

