# Scope and principles

## In scope

The suite covers GGUF structure and metadata; tokenization; detokenization; chat/Jinja templates; logits and embeddings; sampling; deterministic runs; standard and ROCmFPX quantized kernels; long context and context shift; state and persistent cache save/restore; cache rejection and isolation; native and compatibility server APIs; draft-model and embedded MTP speculative decoding; RPC; cancellation; and expected failure behavior.

## Out of scope by default

- universal quality claims across unrelated models or tasks;
- universal performance thresholds across hardware;
- comparisons between different model bytes presented as engine conformance;
- public-network RPC security certification;
- exact stochastic text equality;
- treating a missing capability or missing test fixture as a pass;
- automatic promotion of a candidate result into its own baseline.

## Principles

### 1. Provenance is part of the output

A token vector or logit array without source commit, model SHA-256, binary SHA-256, build flags, backend, device, driver/runtime, fixture digest, and control parameters is incomplete evidence.

### 2. Reuse before rewrite

Run upstream native tests unchanged wherever possible. New differential tests should cover cross-fork contracts, fork-specific features, failure modes, or evidence capture that upstream tests do not address.

### 3. Exact where semantics are discrete

Bytes, token IDs, rendered prompts, deterministic output tokens, JSON shape, event order, stop reason, error class, and cache identity should be exact after narrowly defined normalization.

### 4. Calibrate numeric comparisons

Floating-point logits, embeddings, dequantized values, quality metrics, and stochastic statistics require scoped profiles. The suite ships those fields as `null`.

### 5. Separate correctness from speed

A faster result is not correct by implication. Performance data is record-only until a lane-specific baseline is approved, and it never waives a semantic failure.

### 6. Failure is a first-class contract

Malformed files, cancellation, OOM, full disks, transport loss, incompatible caches, unsupported backends, and invalid API input must fail within a watchdog and leave the process or persistent state in a declared condition.

### 7. Capability evidence replaces assumptions

Each binary emits or is probed for capabilities. Conditional tests either run or produce an evidence-backed skip. Fork names alone do not establish support.
