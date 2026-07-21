# P09 MiniMax-M2 masked-expert decode review v01

Verdict: **accept for commit after pre-clean authority clarification**. No P0,
P1, or P2 finding remains, and neither a rerun nor an expanded test matrix is
justified.

The retained RPC change correctly preserves null-buffer graph-compute views for
later backend allocator initialization. Allocated weight views still discard
the client pointer and retain the P06g server-side reconstruction and backend
validation contract. The exact crash and successful retry, followed by clean
direct and RPC Q6-view qualification, cover the concrete defect.

Both nodes completed the 739-step clean Linux build with identical hashes.
Feature-off and L02 contracts passed 2/2; direct ROCm and disposable RPC tests
both produced reference L2 `24.3547155`, NMSE `0`, and maximum error `0`.

Independent recalculation of the retained C-A-C data reproduced the receipt's
means and deltas. The masked-expert candidate is correctly rejected at
`-4.208927%` prompt processing and `-2.760620%` generation, with identical
decoded content and token sequences throughout. No candidate implementation or
runtime remains.

Both evidence manifests verify, and both bundle hashes, sizes, and mode `0600`
match the receipt. The milestone now explicitly distinguishes the superseded
pre-clean `final-source-binaries.sha256` focused-test record (`948e11d...`)
from the authoritative clean-build test (`e5a3526...`). Keeping the superseded
record in immutable raw evidence is acceptable after that disclosure.

All five reference clones remain clean at their recorded commits and trees;
HaloFPX still has no remote. No donor code, GPL llama-ai implementation,
CachyLLama code, dependency, persistence, WebUI, or deployment replacement
entered the retained change. Rollback evidence supports the documented
worker-first/coordinator-second restoration, active zero-restart services, and
healthy known-good endpoint. The teardown race is disclosed accurately and
does not affect completed response measurements.

