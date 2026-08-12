---
section_id: "33"
title: "KV layout and attention validation"
status: "needs-machine-validation"
last_verified: "2026-07-19"
applies_to:
  repositories: ["ggml-org/llama.cpp", "charlie12345/ROCmFPX"]
  software_versions: ["788e07d", "a5605a7", "37ff5e4f"]
  hardware_revisions: ["gfx1151 L14Q-T01 test coverage qualified; runtime optimization pending"]
related_sections: ["31", "42", "61", "76"]
---

# Validation matrix

Prerequisites: validated model artifact, exact binary hash, fixed prompt/corpus, non-root shell. Capture full startup logs because they report actual KV bytes/types and FA selection.

For each architecture and backend, test:

1. cache types `f16` and `q8_0`; then `q5_0`, `q4_0`, and fork-only types where supported;
2. K/V symmetric and selected asymmetric pairs;
3. FA `off`, `on`, `auto` at prompt and decode shapes;
4. short, window-boundary, long-context, shift/eviction, save/restore and corruption cases;
5. one and multiple slots/streams;
6. single node, each node independently, intended distributed placement, forced rank loss and fallback.

Example:

```bash
./build/bin/llama-server -m model.gguf -c 32768 -np 1 -ngl 999 \
  --flash-attn on --cache-type-k q8_0 --cache-type-v q8_0 \
  --host 127.0.0.1 --port 8080
```

For fork-only TurboQuant, first confirm the binary advertises the type. Then compare at identical model, prompt, seed, context, slots, backend and build:

```bash
# experimental only
./build/bin/llama-server -m model.gguf -c 32768 -np 1 -ngl 999 \
  --flash-attn on --cache-type-k q8_0 --cache-type-v turbo4 \
  --host 127.0.0.1 --port 8080
```

Record actual cache bytes, peak resident/GTT/device memory, prompt/decode throughput, FA/fallback path, PPL/KLD, task outcomes, restore equality and rank ownership. Use Section 31 quality tiers.

**[RECOMMENDATION]** Hard failures: non-finite results, unexplained fallback, corrupt/mismatched cache acceptance, state divergence after exact restore, shift semantic error, rank ownership conflict, or partial state accepted after rank failure.

## L14Q-T01 qualified test seam

The accepted test-only control is HaloFPX commit
`37ff5e4f6ab48ed7d8b0ea2fda05a6304091ae2b`, tree
`921dd1709ab3ee343416d0d3137f46059eef6e6b`. Its sole source change is a
30-line insertion in `tests/test-backend-ops.cpp`; rollback restores parent blob
`5e712d7271f23e4ebff14b60bf234f8b7e4d394a`.

The focused positive matrix is exactly:

1. K/V pairs Q8_0/Q8_0 and Q4_0/Q4_0;
2. head dimensions 128 and 256; and
3. `(nr2, kv, nb)` shapes `(1,255,1)`, `(1,256,1)`, `(1,257,1)`,
   `(8,256,1)`, and `(8,256,9)`.

This is 20 cases. Run each selected backend case repeatedly and report CPU,
ROCm, and Vulkan separately. The associated negative is ROCm-only
Q8_0/Q8_0 at head dimension 160 and `(8,256,1)`; it must report unsupported,
not silently pass. The accepted node evidence is 200/200 focused positive
executions per backend per node with zero failures, plus identical inherited
full-inventory counts on both nodes. Backend `not supported` classifications
remain capability outcomes and are not promoted as passes.

Nimo-2's full CTest launcher attempt recorded 79/95 because 16 tokenizer
invocations were corrupted by a CRLF launcher artifact and returned rc 8. The
accepted feature-off subset instead passed 79/79 (78 non-tokenizer tests plus
the tokenizer test separately). A corresponding nimo-1 CRLF launcher artifact
is also retained and excluded. Preserve both raw artifacts and never use them
as evidence of a product pass or failure.

Do not use this seam to claim an optimization or speedup. A runtime HIP or
Vulkan lane still requires its own admitted P3 treatment, deterministic
correctness, matched repeated A/B performance, memory/scratch accounting,
fallback evidence, zero-regression decision, and independent promotion review.
