# Required local preflights

## Gate order

1. Legal scope and gating signoff.
2. Fetch only the pinned publisher and artifact revisions; never `main`, `latest`, a floating tag, or an unpinned conversion alias.
3. Verify ordered shard names, exact byte counts, and SHA-256. Incomplete manifests must be refreshed before qualification.
4. Hash tokenizer, tokenizer config, chat template, processor/projector assets, config, imatrix, and all conversion metadata.
5. Build exact llama.cpp and ROCmFPX commits with recorded compiler, flags, ROCm, driver, and GPU architecture.
6. Parse GGUF metadata and compare architecture, tensor count/types, split metadata, tokenizer, and template with the publisher identity.
7. Run CPU mmap/no-mmap and GPU-offload load tests; capture resident memory and failures.
8. Force backend tracing. Reject silent CPU fallback when the acceptance plan requires ROCm execution.
9. Run deterministic 4K, 32K, 128K, then publisher-maximum context probes with telemetry.
10. Run architecture-specific state tests and separate speculative/MTP tests.
11. Run workload quality tests against an approved reference and human acceptance thresholds.

## Architecture-specific gates

- DeepSeek V3.1/R1: MLA latent-state memory, routed/shared expert trace, long-context stability, MTP treated as absent until separately proven.
- GLM-4.7: base decoding first. The pinned source skips NextN execution; a later runtime requires a new immutable review.
- Qwen3.5: recurrent DeltaNet sequence/reset/copy semantics, full-attention transitions, MTP draft path, and independent vision/projector qualification.
- Nemotron: Mamba-2 convolution/recurrent state, attention-layer KV state, LatentMoE routing, and independent MTP qualification.
- MiniMax: legal authorization/scope, custom-code/template review, MoE execution, and MTP mismatch review.
