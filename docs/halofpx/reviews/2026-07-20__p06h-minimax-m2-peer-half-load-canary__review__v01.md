# P06h MiniMax-M2 peer half-load canary independent review

Date: 2026-07-20

Verdict: **accept for commit**

No P0, P1, or P2 blocking finding remains.

The loader slice is bounded and bound to the exact created tensor pointer. It
rejects invalid dimensions and ranges, checks arithmetic and packed row
geometry, accounts only the admitted slice, and applies the source offset to
mmap, host, asynchronous/direct, fallback, validation, and `load_data_for()`
paths. The strict-device helper rejects tensor overrides and accepts only
buffer types owned by the exact admitted device.

P06h remains default-off and fail-closed. The new environment gate accepts
only the exact value `1`, requires both earlier gates, and retains the exact
ROCm/RPC topology and local layer-owner checks. The peer physical 96-expert
tensor is consumed directly with IDs 0 through 95; the local branch uses
experts 0 through 95 from the full local tensor. The full local MoE result
remains authoritative, and missing routing/placement or divergent branch
output rejects execution before HTTP visibility.

The milestone and receipt agree with Wiki sections 32, 34, 44, 48, and 86.
They state nimo-2 coordinator and authoritative ownership, nimo-1 experimental
upper-half ownership, peer failure behavior, and the absence of an admitted
single-node plan for the 160 GB artifact. Recovery is a fresh known-good
default-off dual-node restart or a separately admitted smaller model, never
partial continuation.

Retained evidence verifies both manifests, mode-0600 bundles, byte-identical
node binaries, 2/2 focused contracts per node, nonzero direct and RPC Q6
oracles, exact packed source ranges `[368050176,736100352)`, nonzero local and
peer contributions, HTTP 200, and byte-identical response content versus
P06g. The original nimo-2 worker and nimo-1 coordinator were restored with
zero restarts, expected binary hashes, the worker listener, and HTTP 200.
All five immutable reference clones remain clean at their recorded commits
and trees. No donor implementation, GPL/CachyLLama code, dependency,
persistence, WebUI, deployment replacement, or remote change entered P06h.

Non-blocking debt: the exact canary exercised the no-mmap/direct-I/O route.
Broader mmap and injected peer-failure permutations remain deferred until the
seam approaches authoritative sharding. The observed 202.92 prompt tokens/s
and 15.91 generation tokens/s remain canary timings, not performance evidence.

