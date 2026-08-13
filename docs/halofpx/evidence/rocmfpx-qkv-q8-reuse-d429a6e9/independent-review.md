# Independent post-fix review

Review task: `/root/qkv_q8_reuse_impl/qkv_alias_fix_final_audit`

Final publication-source head:
`760276e39123622aadf5ef915e9c2b4f92172f8f`

Core implementation/hardening head:
`7a6984c5a03df384be269d1c266ee993fa2184ea`

Exact base: `3758febacfc07fdc6e84b63637131b02d413de59`

Retained-evidence manifest SHA-256:
`756207e980a48b45e35b5416c6cd3ec4942720ffac98b5e1d621ce0453bfff13`

## Final verdict

**PASS / RETAIN after one CI-command correction and two evidence-only base
advances.** The reviewer found no remaining
implementation correctness blocker in the alias barrier, production optimizer
seam, scratch lifetime, stream order, geometry/strides, fallback behavior,
metrics, selector tests, or source contract.

The review initially blocked publication because the pinned ROCm container job
called bare `hipconfig`, while that image does not put `/opt/rocm/bin` on
`PATH`. The final source commit fixes the command to
`/opt/rocm/bin/hipconfig --full` and binds that exact command in the source
contract. This correction changes CI invocation only. Git range-diff and blob
comparisons prove the implementation patches stayed equivalent through the
subsequent main advances; shared indexes/workplan additionally preserve the
merged prefix-selector and OOM-authority records.

## Reviewed paths

- `ggml/src/ggml-cuda/rocmfpx-qkv-q8-reuse.h`;
- `ggml/src/ggml-cuda/ggml-cuda.cu`;
- `ggml/src/ggml-cuda/mmq.cu` and `mmq.cuh`;
- `ggml/CMakeLists.txt` and `ggml/src/ggml-hip/CMakeLists.txt`;
- `tests/test-halofpx-rocmfpx-qkv-q8-reuse.cpp`;
- `tests/test-backend-ops.cpp`;
- `tests/test-halofpx-rocmfpx-qkv-q8-reuse-source-contract.cmake`;
- `.github/workflows/halofpx-ci.yml`;
- ADR/candidate documentation, the evidence capture driver, and every retained
  host-evidence file.

## Findings closed

- The original P1 compaction bug is closed: the planner scans the complete
  moved interval and rejects a non-metadata in-place write rooted at the shared
  activation or a Q/K/V weight.
- ggml canonicalizes nested views to the owning root, matching the barrier's
  identity comparison.
- Negative activation/weight-write tests and positive metadata/output-local
  controls bind the intended boundary.
- The backend-operation case invokes the registered production optimizer
  before allocation and preserves its planned-group count through execution.
- The retained evidence manifest and every listed file rehashed cleanly; the
  executed binary hashes reverified; implementation blob identities match the
  captured pre-final-base source and the rebased implementation blobs compare
  byte-identically.

## Claim boundary

The reviewer did not access either Strix Halo host or a HIP runtime. Host
focused tests are retained as 3/3 and the representative CPU backend harness as
54/54. Target runtime parity, model reachability, graph replay, launch traces,
and performance remain blocked/open under issue #41.
