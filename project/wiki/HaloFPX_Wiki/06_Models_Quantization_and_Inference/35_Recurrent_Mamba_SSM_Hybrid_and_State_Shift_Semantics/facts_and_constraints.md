---
section_id: "35"
title: "Recurrent state facts and constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: []
  hardware_revisions: []
related_sections: ["32", "61"]
---

# Facts and constraints

## Execution state

**[VERIFIED]** Selective SSM parameters depend on the input; the Mamba paper describes a hardware-aware parallel recurrent algorithm and recurrent inference state rather than an attention KV history [S35-03]. This state is fixed-size per layer with respect to decoded sequence length, but its exact tensor dimensions are model/configuration dependent.

**[VERIFIED]** Jamba is a hybrid architecture mixing Transformer and Mamba layers and may also use MoE [S35-04]. Consequently, "clear cache" for a hybrid model must cover both attention and recurrent memory.

**[VERIFIED]** llama.cpp commit `788e07d` has `llama_memory_recurrent` with per-cell sequence metadata and recurrent tensors; it implements clear, remove, copy, keep, add/div position operations, update, and state read/write paths [S35-01]. The public API exposes generic memory sequence operations, but support/effect is memory-type specific.

**[INFERENCE]** Position arithmetic cannot reconstruct a recurrent state after deleting or changing an earlier token. Because later state is a function of the prior state and intervening inputs, recomputation from the last valid prefix is required.

## Checkpoint validity tuple

**[RECOMMENDATION]** A restorable record must bind at least:

`schema version; model/GGUF hash; tokenizer/chat-template hash; engine commit/build; architecture and hparams; quantization; graph/backend policy; sequence ID; exact token-prefix hash and length; logical position; recurrent tensor names/shapes/types/strides/bytes/checksums; attention KV descriptors/checksums; rank/shard owner map; sampling state if exact generation replay is claimed.`

**[VERIFIED]** llama.cpp source serialization proves that opaque state bytes and sequence metadata are both relevant [S35-01]. It does not establish cross-version portability.

## Known risk evidence

**[OPEN]** Upstream issue 21681 reports plausible but wrong output after prompt-cache restoration with a hybrid DeltaNet model and references a recurrent copy bug [S35-05]. This is issue evidence, not a confirmed universal defect; it justifies a regression test and fail-closed policy.

