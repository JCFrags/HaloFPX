---
section_id: "33"
title: "KV layout and attention validation"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp", "charlie12345/ROCmFPX"]
  software_versions: ["788e07d", "a5605a7"]
  hardware_revisions: ["gfx1151 pending"]
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

