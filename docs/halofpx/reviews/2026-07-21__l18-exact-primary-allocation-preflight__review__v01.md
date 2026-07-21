# Independent review: L18 exact-primary allocation preflight

Date: 2026-07-21

Reviewer: independent adversarial agent

Verdict: **PASS — no P1/P2 finding.**

## Source and loader authority

The reviewed implementation records allocation evidence only when the real
model loader is in `no_alloc` mode. It enumerates the architecture-created GGML
contexts after `done_getting_tensors()`, calls the same
`ggml_backend_alloc_ctx_tensors_from_buft_size()` logic used to size a material
allocation, binds every created tensor to its GGUF source descriptor and
bounded source range, and refuses incomplete, unknown, unaccounted,
ambiguous-group, or arithmetic-invalid plans. Normal material loading retains
the existing allocation path; the new plan is observational and default-empty
outside `no_alloc`.

The final executed lineage is closed by hashes for the planner executable,
resolved `libllama`, `libllama-common`, and all project `libggml` shared
libraries, the final source patch/hashes, exact argv, and reviewed raw plan.

## Exact accounting and capacity

The pinned 159,873,097,824-byte artifact passed its exact SHA-256 identity. The
loader accounted for 809 source tensors as 809 created and 809 unique names,
with zero unknown/unaccounted tensors, zero views/slices, and 159,864,809,984
source tensor bytes. Three planned allocation requests sum exactly to that
total: RPC0 80,950,550,528 bytes; ROCm0 device 78,280,456,704 bytes; ROCm0 host
633,802,752 bytes.

Adversarial reconciliation found that L17's resolver-only output prediction was
not the final loader placement: `output.weight` is in RPC0,
`output_norm.weight` is in the ROCm0 device group, and `token_embd.weight` is
the ROCm host group. The result and receipt now state this explicitly and
correctly make L18's real loader groups higher authority.

Checked capacity arithmetic separately labels exact weights, no-alloc
KV/context and graph estimates, 10% fragmentation policy, and a 16 GiB reserve.
Required totals are 106,643,119,104 bytes on RPC0 and 104,737,304,935 bytes on
ROCm0, leaving 26,500,867,072 and 28,406,681,241 bytes against each
backend-reported total. This is admissibility, not allocator proof.

## Qualification and operational boundary

Focused identity, order, accounting, margin, and overflow tests passed. The
corrected reviewed plan returned success. Source inspection confirms zero-byte
model/KV sentinels and no-alloc graph sizing; the isolated RPC journal records
zero alloc-buffer and zero tensor GET/SET operations. Production coordinator
PID 2144857 and worker PID 1305879 remained active with `NRestarts=0`, port
8081 stayed HTTP 200, and no primary inference, material load, cache write,
production restart, L19 work, or retry occurred.

The final v4 archive is mode 0600, 29,317 bytes, SHA-256
`8c05b1c31644b919af8bd9a30953022b58ef4e0588ca694d0edf7fd3fc52b8a9`;
zstd integrity passes. Cleanup is complete: the disposable unit is unloaded,
port 50194 is closed, and disposable nimo-1 binary plus nimo-2 source/build
roots are absent.

## Review reconciliation and residual uncertainty

Review initially found stale post-correction execution evidence, missing
dynamic-library lineage, contradictory hash/read wording, incorrect non-layer
group labels, an unacknowledged L17 output-placement divergence, and incomplete
cleanup. All were corrected and independently rechecked. No P1/P2 remains.

L18 does not prove allocator success, real fragmentation behavior, runtime
residency, inference correctness, throughput, or a maintenance transition. Any
later admission must use the real loader plan rather than the resolver alone;
L19 and a primary retry remain closed.
