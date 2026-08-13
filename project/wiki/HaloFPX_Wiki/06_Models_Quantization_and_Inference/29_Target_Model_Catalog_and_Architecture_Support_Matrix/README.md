---
section_id: "29"
title: "Target Model Catalog and Architecture Support Matrix"
status: "needs-machine-validation"
last_verified: "2026-08-12"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "ggml-org/llama.cpp", "JCFrags/HaloFPX"]
  software_versions: ["ROCmFPX a5605a7", "llama.cpp 788e07d", "HaloFPX b77f2bce"]
  hardware_revisions: ["two matched AMD Strix Halo systems - exact revisions pending"]
related_sections: ["30", "31", "33", "34", "35", "36"]
---

# Target model catalog

This section is a candidate catalog, not a compatibility promise. **[VERIFIED]** At the pinned commits, llama.cpp declares architecture identifiers for dense, MoE, Mamba/Mamba2, Jamba, Nemotron-H, DeepSeek, Qwen, Gemma, Mistral, and MTP-related families [S29-01]. **[INFERENCE]** An enum proves recognition only; usable support additionally requires a matching converter, correct GGUF metadata/tensor mapping, graph implementation, and every selected backend operation.

The initial candidates deliberately span dense GQA, MoE, MLA/MTP, hybrid SSM, coding/tool-use, multimodal, and long-context workloads. Detailed fields and the support-stage rubric are in [facts_and_constraints.md](facts_and_constraints.md).

**[VERIFIED]** The catalog now has a small ordinary dense-GQA daily fixture: the
exact Qwen3-0.6B BF16 distribution and pure Q3/Q6/Q8 ROCmFPX identities are
hash-registered. **[MEASURED]** All three pass a bounded off-target pinned-b77
CPU smoke. This closes the artifact-identity gap only; target backend, quality,
and performance stages remain open [S29-07].

**[OPEN]** This is not yet the complete catalog required by the research prompt. Pure-MHA and sliding-window/global targets are unpinned; several rows lack verified total/active counts, actual weight sizes by quant, exact special-state bytes, resolved license terms, converter results, backend results, or machine-validation status. Nominal size arithmetic must not be substituted for artifact hashes and observed file/memory sizes [OQ29-01, OQ29-02, OQ29-09].

## Research split

- Completed now: pinned upstream/fork commits; inspected architecture registry; recorded primary model configurations and licenses where public.
- Required on the two machines: convert or obtain provenance-pinned GGUFs; run metadata, tokenizer, graph, backend-op, quality, memory, and long-context gates on both HIP and Vulkan.
- Contingent decisions: production model set, quant recipe, maximum context, distributed mode, and persistent-state schema.

**[RECOMMENDATION]** Admit a model to the production catalog only after a row-specific evidence bundle records source revision, model hash, converter commit, GGUF hashes, backend, context, cache type, and acceptance results.
