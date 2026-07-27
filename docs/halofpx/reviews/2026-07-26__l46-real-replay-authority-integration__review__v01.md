# L46 independent adversarial review

Verdict: **PASS — NOT PROMOTED blocker is source-backed**

The reviewer independently inspected the accepted L42/L44 implementation and
the real MiniMax replay path. No files were changed by the review.

The L44 commit walk requires every recursively discovered leaf to be
registered or excluded before it filters for RPC storage. Registration and
exclusion both reject non-RPC tensors. The mixed RPC0/ROCm0 MiniMax graph
therefore cannot pass the accepted census API. Nested views add an independent
coverage gap because dynamically created view leaves cannot be fully excluded
at model-load call sites.

The public scheduler API does not expose the private per-RPC split graph, so the
caller cannot scope the L44 commit to the exact RPC graph. L44 also snapshots
L42 admission before compute, while the final L42 split/copy transcript is
available only after execution. The current APIs cannot bind both authorities
to the same pre-compute session.

The reviewer agrees that fixing this requires an L42/L44 API or census
extension rather than runner-only wiring. Retaining no candidate and closing
L46 as NOT PROMOTED is the correct fail-closed result.
