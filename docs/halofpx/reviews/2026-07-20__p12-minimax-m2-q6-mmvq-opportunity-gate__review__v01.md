# P12 MiniMax-M2 Q6 MMVQ opportunity-gate independent review

Status: **accepted; no blockers**

The review checked AGENTS.md, P08 through P12, the complete source diff,
receipt arithmetic, admitted and excluded evidence boundaries, the CMake gate,
and the scope of every performance claim.

The compile-time `HALOFPX_MINIMAX_M2_EXPERT_PARTITION_CANARY` option remains
default-off. Runtime, model, RPC, persistence, and cache sources are untouched.
The four correctness and four perf registrations match the exact Q6 full
192/top-8 and compact 96/top-4 gate/up and down projection shapes. The perf
screen correctly uses the inherited `test_mul_mat_id` FLOP accounting rather
than the invalid whole-chain output accounting.

The reviewer independently recomputed 310.08 microseconds for the inherited
two-gate-plus-down chain, 160.65 microseconds for compact owned compute,
51.8092105% compact/full, and 11.40 microseconds or 3.6764706% for the
conservative five-add proxy. The invalid chain harness and zero-selected
correctness invocation are explicitly excluded and do not support a claim.

The opportunity claim is appropriately bounded: it is not RPC overlap,
product implementation, exact-model speedup, or non-inferiority evidence. Raw
log names and hashes, the node evidence root, and bundle hash are recorded;
the reviewer did not independently access the node and therefore did not
re-hash remote files.

A nonblocking wording ambiguity was corrected before promotion so the document
now distinguishes the sole implementation/test source change from its
documentation and evidence changes.
