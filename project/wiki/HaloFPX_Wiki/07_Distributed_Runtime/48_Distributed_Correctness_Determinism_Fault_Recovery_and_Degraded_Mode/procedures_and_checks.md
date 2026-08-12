---
section_id: "48"
title: "Distributed Correctness Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX proposed runtime"]
  software_versions: []
  hardware_revisions: ["two matched AMD Strix Halo systems (exact BOM open)"]
related_sections: ["53", "63", "73", "78", "80", "81"]
---

# Procedures and checks

## Deterministic fixture

Prerequisites: immutable model/tokenizer/runtime/build/plan hashes; fixed prompt token IDs, seed, sampler chain, batch/ubatch, concurrency, cache state, power profile; raw tokens/logits and per-rank traces retained. Root is not required for baseline tests.

1. Establish single-node reference for HIP and Vulkan separately.
2. Repeat identical runs across cold/warm cache, batch shapes, and co-tenancy. Report bitwise token equality and logit error distribution; do not relabel tolerance as identity.
3. Repeat for each distributed mode. Capture per-rank `step_id`, collective descriptor/order, state-prefix hash, and output index.
4. Validate attention, recurrent/SSM, native MTP, external draft, grammar, and sampling state separately where supported.

## Fault injection matrix

At queued, prefill, decode, collective, checkpoint, and streaming phases:

- pull/disable link A, link B, then both; inject delay, loss, corruption, duplication, and reordering below the protocol where tools allow;
- stop/kill/restart each worker and coordinator; force communicator timeout/asynchronous error;
- present wrong model, tokenizer, runtime build, plan, rank ID, cache fingerprint, cache page hash, and checkpoint prefix;
- fill KV/memory, output buffer, disk/cache volume, and command ring;
- cancel twice and race cancel with completion/restart.

Assert: no hang beyond configured deadline; no token committed twice or after terminal status; corrupted/mismatched state becomes miss/recompute; stale worker is fenced; all resources return to baseline; fallback increments epoch and uses an approved manifest.

## Recovery drills

1. Restore from each checkpoint type and compare against prompt replay.
2. Reattach stream from every acknowledged output index, including evicted-window behavior.
3. Exercise one-link operation and return to dual-link only between epochs.
4. Exercise single-node fallback for every model that fits; explicitly verify rejection for those that do not.

**[RECOMMENDATION]** Make this matrix a release gate under sections 78/80/81. Preserve raw logs and environment metadata.
