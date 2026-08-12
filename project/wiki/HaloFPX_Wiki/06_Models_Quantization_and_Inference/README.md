# 06 — Models, Quantization, and Inference

## Category manifest

- **Purpose:** Record model support, numerical formats, inference behavior, and kernel opportunities.
- **Authoritative files:** This manifest and the nine linked section artifact sets.
- **Current owner:** Model and implementation workers own qualification. Documentation workers own routing.
- **Status:** Source-backed draft complete. All section metadata passes the Wiki validator. Qualification remains open.
- **Last verified date:** 2026-07-29 for routing. Section claims retain their own dates.
- **Source commits:** ROCmFPX `a5605a72768c6562241b248e268e33dc92787394`; llama.cpp `788e07dc91d266ad3162a1ce9037665656269689`; CachyLLama `6be745998f568e379ea197fcf827baec73ff9940`.
- **Related decisions:** [Decision map](../decision-map.md) and the linked implementation decision index.
- **Related evidence:** [Evidence map](../evidence-map.md) and [Verification and Performance](../11_Verification_and_Performance/README.md).
- **Open work:** Qualify each supported model, quantization, backend, and feature combination.
- **Next safe action:** Name the exact model hash, source commit, binary, and runtime tuple before a claim.

Captures the supported model families, numerical formats, engine behavior, and kernel optimization surface.

Research status: source-backed draft complete; model-specific correctness, quality, and target-machine performance remain open.

- [29 — Target Model Catalog and Architecture Support Matrix](29_Target_Model_Catalog_and_Architecture_Support_Matrix/README.md)
- [30 — ROCmFPX Weight Formats and Quantization Recipes](30_ROCmFPX_Weight_Formats_and_Quantization_Recipes/README.md)
- [31 — Conversion, Imatrix, Calibration, and Quality Validation](31_Conversion_Imatrix_Calibration_and_Quality_Validation/README.md)
- [32 — llama.cpp Model Loading, Graph Construction, and Backend Lifecycle](32_llama_cpp_Model_Loading_Graph_Construction_and_Backend_Lifecycle/README.md)
- [33 — Attention Variants, KV Layouts, FlashAttention, and TurboQuant](33_Attention_Variants_KV_Layouts_FlashAttention_and_TurboQuant/README.md)
- [34 — MoE Routing, Expert Telemetry, and Expert Placement Inputs](34_MoE_Routing_Expert_Telemetry_and_Expert_Placement_Inputs/README.md)
- [35 — Recurrent, Mamba, SSM, Hybrid, and State-Shift Semantics](35_Recurrent_Mamba_SSM_Hybrid_and_State_Shift_Semantics/README.md)
- [36 — Native MTP, External Draft Models, and Speculative Decoding](36_Native_MTP_External_Draft_Models_and_Speculative_Decoding/README.md)
- [37 — gfx1151 HIP and Vulkan Kernel Optimization Opportunities](37_gfx1151_HIP_and_Vulkan_Kernel_Optimization_Opportunities/README.md)

All nine sections remain `needs-machine-validation`. Static source support does not establish a model/quantization release lane, numerical parity, or performance on either target node.
