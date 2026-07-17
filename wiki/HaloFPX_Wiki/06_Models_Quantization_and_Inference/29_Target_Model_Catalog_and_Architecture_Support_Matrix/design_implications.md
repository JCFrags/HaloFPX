---
section_id: "29"
title: "Model catalog design implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX"]
  software_versions: ["a5605a7"]
  hardware_revisions: []
related_sections: ["30", "33", "34", "35", "36", "57"]
---

# Design implications

- **[RECOMMENDATION]** Make `model_id` content-derived: publisher/repo revision + source tensor hashes + converter commit + quant recipe ID. A filename is not identity.
- **[INFERENCE]** Ordinary GQA models have token-linear KV growth and are the safest first persistent-cache target. MLA and recurrent models need architecture-specific state descriptors; never coerce their state into the ordinary K/V formula.
- **[RECOMMENDATION]** Start implementation gates with Qwen3-30B-A3B (MoE) and Qwen2.5-Coder-32B (dense) because their nominal low-bit weights fit one 128-GB-class node with room for cache; retain Mistral 24B for multimodal/long-context coverage.
- **[RECOMMENDATION]** Treat DeepSeek-V3 as a schema, transport, and sharding stressor until an actual memory-fit plan is measured. Its nominal low-bit weights alone can exceed two-node aggregate memory.
- **[RECOMMENDATION]** Give recurrent/hybrid models a separate state capability flag and single-node fallback. Rank ownership must name KV layers, recurrent layers, convolution state, and rollback behavior.
- **[INFERENCE]** “Context length” from a config is an upper architectural setting, not an SLO. Actual usable context depends on cache type, batch/slots, backend buffers, and quality under position scaling.

Persistent cache keys must include architecture, tensor layout, RoPE/scaling parameters, cache K/V types, recurrent-state schema, context parameters, tokenizer/chat-template hash, and rank topology. Any mismatch or corrupt payload must be a miss/recompute, never accepted state.

