---
section_id: "35"
title: "Recurrent state procedures and checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo"]
related_sections: ["61", "77", "78", "80"]
---

# Procedures and checks

## Internet/source work completed

The current recurrent memory implementation, public state APIs, Mamba/Jamba papers, and one upstream hybrid-cache failure report were reviewed [S35-01, S35-02, S35-03, S35-04, S35-05]. Follow up each upstream rebase by diffing the recurrent memory and state-serialization paths.

## M35-01 state inventory

For every target GGUF, dump architecture metadata and instrument creation of each memory object. Record tensor name, layer, kind, shape, type, stride, byte size, owning rank/backend, and serialization participation. Acceptance: every mutable continuation tensor maps to one manifest entry or is explicitly proved derivable.

## M35-02 restore equivalence matrix

Use fixed seeds and tokenized prompts. For each model and backend, compare uninterrupted generation with save/restart/restore at multiple boundaries:

| Case | Mutation |
|---|---|
| exact prefix | none |
| suffix truncate | remove last N tokens |
| middle edit | change one earlier token/message |
| sequence fork | copy then diverge both branches |
| clear | reuse slot after full clear |
| context pressure | boundary near configured context limit |

Record logits for at least the first post-restore step and generated tokens. Acceptance: exact-prefix restore meets the declared numeric tolerance; unsafe mutations miss/recompute rather than silently reuse stale state.

## M35-03 distributed fault test

Checkpoint both ranks, interrupt one rank between shard writes, and attempt restore. Acceptance: incomplete/mixed-generation manifests are rejected; verified single-node fallback or recomputation succeeds.

## Non-destructive rule

Keep raw checkpoint and logs until checksum and restore tests finish. Corruption, missing shard, schema mismatch, or unknown state kind must produce a cache miss.
