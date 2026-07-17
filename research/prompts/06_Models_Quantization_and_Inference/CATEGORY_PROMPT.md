# Category Research Agent Prompt — 06: Models, Quantization, and Inference

The project is a custom local LLM inference stack for two matched AMD Strix Halo systems connected by two tuned, low-latency USB4 host-to-host links. The intended source base is a fork of `charlie12345/ROCmFPX`, selectively incorporating the persistent SSD-backed KV-cache and agent-serving ideas from `fewtarius/CachyLLama` and `fewtarius/llama-ai`, while tracking `ggml-org/llama.cpp`. The product must choose among replication, remote speculative decoding, two-way tensor parallelism, pipeline parallelism, and MoE-aware hybrid execution, with a rank-local persistent cache and a specialized dual-link transport. The wiki will be consumed by local Codex and human engineers before and during implementation.

Build the complete wiki category **06: Models, Quantization, and Inference**. Research every numbered section below as a separate self-contained folder:

- `29_Target_Model_Catalog_and_Architecture_Support_Matrix/` — 29: Target Model Catalog and Architecture Support Matrix
- `30_ROCmFPX_Weight_Formats_and_Quantization_Recipes/` — 30: ROCmFPX Weight Formats and Quantization Recipes
- `31_Conversion_Imatrix_Calibration_and_Quality_Validation/` — 31: Conversion, Imatrix, Calibration, and Quality Validation
- `32_llama_cpp_Model_Loading_Graph_Construction_and_Backend_Lifecycle/` — 32: llama.cpp Model Loading, Graph Construction, and Backend Lifecycle
- `33_Attention_Variants_KV_Layouts_FlashAttention_and_TurboQuant/` — 33: Attention Variants, KV Layouts, FlashAttention, and TurboQuant
- `34_MoE_Routing_Expert_Telemetry_and_Expert_Placement_Inputs/` — 34: MoE Routing, Expert Telemetry, and Expert Placement Inputs
- `35_Recurrent_Mamba_SSM_Hybrid_and_State_Shift_Semantics/` — 35: Recurrent, Mamba, SSM, Hybrid, and State-Shift Semantics
- `36_Native_MTP_External_Draft_Models_and_Speculative_Decoding/` — 36: Native MTP, External Draft Models, and Speculative Decoding
- `37_gfx1151_HIP_and_Vulkan_Kernel_Optimization_Opportunities/` — 37: gfx1151 HIP and Vulkan Kernel Optimization Opportunities

Apply `../OUTPUT_STANDARD.md` to every section. Use current primary sources, exact commits/versions/dates, and explicit claim labels. Separate Internet research from required on-machine investigation. Do not invent measurements or silently resolve conflicting evidence.

Return one downloadable folder named `06_Models_Quantization_and_Inference/` containing the numbered section folders at the exact paths listed above, plus a category `README.md` that summarizes the category, identifies authoritative pages, links all sections, and highlights cross-category dependencies.
