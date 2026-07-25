# L37 independent adversarial review

Verdict: **PASS** for the bounded no-production, no-primary-load L37 milestone.

The first review found one material qualification gap: Python verifier
sentinels changed already-hashed records and did not exercise the C++ collector
admission/content path. The final source closes that gap. The production
collector calls `llama_halofpx_graph_input_content_digest`, and the
feature-gated Release C++ test exercises every admitted mutable name, proves a
content perturbation changes its digest, and refuses unknown, null and empty
inputs with explicit return codes.

The reviewer independently confirmed:

- the proven context use-after-free is removed;
- all result fields are captured while live and the context is freed once;
- the complete result record is authenticated, fsynced and flushed before free;
- the runner compares durable and emitted authority and requires live
  `n_batch=512` for the accepted capture and restore paths;
- default-off activation and the closed L37 fixture configuration are retained;
- 56 focused Python tests pass and the Release collector test passes;
- source blob identities match the receipt;
- fixture evidence, production non-mutation and cleanup reconcile;
- RPC internal tensor-ID/copy-split and opaque workspace gaps are explicitly
  residual uncertainty rather than claimed coverage.

No remaining material finding blocks the L37 PASS commit.

