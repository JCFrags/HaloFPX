# Category Research Agent Prompt — 11: Verification and Performance

The project is a custom local LLM inference stack for two matched AMD Strix Halo systems connected by two tuned, low-latency USB4 host-to-host links. The intended source base is a fork of `charlie12345/ROCmFPX`, selectively incorporating the persistent SSD-backed KV-cache and agent-serving ideas from `fewtarius/CachyLLama` and `fewtarius/llama-ai`, while tracking `ggml-org/llama.cpp`. The product must choose among replication, remote speculative decoding, two-way tensor parallelism, pipeline parallelism, and MoE-aware hybrid execution, with a rank-local persistent cache and a specialized dual-link transport. The wiki will be consumed by local Codex and human engineers before and during implementation.

Build the complete wiki category **11: Verification and Performance**. Research every numbered section below as a separate self-contained folder:

- `73_Benchmark_Methodology_Terminology_Experimental_Controls_and_Data_Schema/` — 73: Benchmark Methodology, Terminology, Experimental Controls, and Data Schema
- `74_Single_Node_HIP_and_Vulkan_Baseline_Matrix/` — 74: Single-Node HIP and Vulkan Baseline Matrix
- `75_Fabric_Microbenchmarks_and_GPU_to_Peer_GPU_End_to_End_Tests/` — 75: Fabric Microbenchmarks and GPU-to-Peer-GPU End-to-End Tests
- `76_Distributed_Mode_Benchmark_Matrix_and_Break_Even_Analysis/` — 76: Distributed Mode Benchmark Matrix and Break-Even Analysis
- `77_HaloKV_Restore_Writeback_Hit_Rate_and_Endurance_Benchmarks/` — 77: HaloKV Restore, Writeback, Hit-Rate, and Endurance Benchmarks
- `78_Correctness_Regression_Determinism_and_Model_Quality_Evaluation/` — 78: Correctness, Regression, Determinism, and Model Quality Evaluation
- `79_Stress_Soak_Long_Context_Multi_Session_Power_and_Thermal_Testing/` — 79: Stress, Soak, Long-Context, Multi-Session, Power, and Thermal Testing
- `80_Fault_Injection_Cable_Pulls_Restarts_OOM_Disk_Full_and_Corruption/` — 80: Fault Injection: Cable Pulls, Restarts, OOM, Disk-Full, and Corruption
- `81_CI_Matrix_Release_Gates_Reproducibility_and_Performance_Regression_Policy/` — 81: CI Matrix, Release Gates, Reproducibility, and Performance Regression Policy

Apply `../OUTPUT_STANDARD.md` to every section. Use current primary sources, exact commits/versions/dates, and explicit claim labels. Separate Internet research from required on-machine investigation. Do not invent measurements or silently resolve conflicting evidence.

Return one downloadable folder named `11_Verification_and_Performance/` containing the numbered section folders at the exact paths listed above, plus a category `README.md` that summarizes the category, identifies authoritative pages, links all sections, and highlights cross-category dependencies.
