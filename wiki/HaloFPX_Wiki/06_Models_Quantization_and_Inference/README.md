# 06 — Models, Quantization, and Inference

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
