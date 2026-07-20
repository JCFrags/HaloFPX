# P06g MiniMax-M2 rank-local Q6 view canary

Status: **exact-artifact rank-local shadow compute qualified; physical storage sharding remains closed**

P06g replaces P06e's full-tensor/global-ID shadow branches with explicit
96-expert views and rank-local IDs. Experts 0 through 95 remain on the local
ROCm branch. Experts 96 through 191 use a nonzero-offset view of the replicated
RPC tensor and IDs remapped into 0 through 95. The full authoritative MoE
result still passes through unchanged; a CPU oracle compares it with the sum
of both shadow branches and aborts before HTTP output on any mismatch.

The feature remains behind both existing default-off environment gates and the
strict P06d/P06e model, topology, graph, sequence, adapter, tensor-type, and
output checks. Storage is still replicated on both ranks. This milestone proves
rank-local execution semantics, not physical sharding or a speed path.

## Two defects corrected

P06f corrected expanded-device Q6 view offsets in the HIP buffer initializer,
but its RPC oracle did not require nonzero reference energy. P06g found that an
RPC graph reconstructed a view from the serialized client pointer without
running the server backend's view initializer. The server now requires a view
and its source to name the same buffer, discards the derived client pointer,
and reinitializes the view from the server-side source allocation. The focused
oracle now rejects an all-zero reference before accepting equivalence.

The first exact P06g attempts still produced a zero peer contribution. A single
instrumented run showed selected IDs `[82,61,2,163,52,31,101,143]`, nonzero raw
peer expert output (L2 `59.8007941`), but all-zero peer IDs and weights.
`ggml_clamp()` is an in-place operation returning a view. Applying it to the
shared float-ID tensor allowed the local clamp to overwrite the input consumed
by the peer remap and ownership mask. The accepted graph creates independent
float casts for mask construction and both clamped ID domains, so no in-place
operation aliases another consumer.

## Focused qualification

Both nimo-1 and nimo-2 built byte-identical Release binaries for `gfx1151`
with HIP, Vulkan, RPC, forced MMQ, no VMM, and WebUI disabled. The strengthened
Q6 oracle reported reference L2 `24.3547155`, NMSE 0, and maximum absolute
error 0 on direct nimo-2 `ROCm0` and nimo-2 `RPC0` to nimo-1 ROCm. The final
feature-off contract and locked L02 controls passed 2/2.

The exact artifact remained revision
`dba517197f2854f3d362529e13abddcdcad6c10b`, file
`saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf`, size
159,873,097,824 bytes, SHA-256
`96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.
The retained deterministic request contained 1,129 prompt tokens and generated
128 tokens. It returned HTTP 200, and its newline-terminated content SHA-256
was `a9c38c7f948adcfa8cfab5468ab84cc089b01a34c3f270f1c487a9a5fa74b555`,
byte-identical to the P06d/P06e control.

The accepted request reported 194.80 prompt tokens/s and 1.67 generation
tokens/s. These are not performance evidence: the default-off shadow path
computes the authoritative full MoE plus both rank branches and synchronizes a
CPU oracle on every token. The first corrected client used an insufficient
64-second timeout; the server nevertheless completed all 128 tokens without an
oracle failure and returned to idle. The retained HTTP-200 rerun used a client
timeout long enough to capture the response.

## Boundaries and next seam

P06g does not compact or physically shard model storage, make the shadow result
authoritative, remove the correctness oracle, change feature defaults, enable
persistence, or establish a performance improvement. The next performance
seam is a target-owned physical 96-expert load plan with a default-off rollback
path, followed by one exact correctness canary before matched timing.

Raw node evidence is hash-manifested and bundle-verified separately. All five
immutable reference clones remain clean at their locked commits and trees.
No donor expression, GPL llama-ai implementation, CachyLLama code, new
dependency, model mutation, remote, WebUI, notice, license, or SBOM change
entered the milestone.
