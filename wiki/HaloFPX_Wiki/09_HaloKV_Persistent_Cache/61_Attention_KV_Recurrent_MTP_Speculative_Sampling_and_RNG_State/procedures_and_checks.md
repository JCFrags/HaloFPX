---
section_id: "61"
title: "Continuation state validation"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: ["dual Strix Halo"]
related_sections: ["77", "78", "80"]
---

# Procedures and checks

## Internet/source work completed

Pinned llama.cpp memory/state APIs and CachyLLama target/draft/spec persistence paths were inspected [S61-01, S61-02, S61-03, S61-04]. Re-run this inventory after every upstream rebase.

## M61-01 stream inventory

For each target model/mode, snapshot every mutable object at a token boundary. Record type, size, schema, owner, and whether reconstruction is proven. Acceptance: no required continuation object is unclassified.

## M61-02 exact-continuation matrix

Compare uninterrupted versus save/restart/restore for dense, MLA, sliding-window, hybrid/recurrent, native MTP, external draft, greedy, stochastic, grammar-constrained, adaptive sampler, and logits-processor cases. Record first post-restore logits, tokens, sampler candidates, grammar state, and RNG trace. Exact mode requires declared equality; cache-reuse mode may recompute but must match baseline quality semantics.

## M61-03 fault matrix

Use only a disposable store and disposable service instance satisfying the Section 63 safety gate. Delete, truncate, corrupt, reorder, duplicate, or version-bump each stream independently. No root access is expected for file-fixture mutations; process, cgroup, kernel, or device faults require their declared privileges and Section 80 authorization. Acceptance: required-stream failure rejects the generation; no corrupt state reaches inference; optional stream loss takes its documented rebuild path.

Decisions about optional streams remain contingent on these results.
