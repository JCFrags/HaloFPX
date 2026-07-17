# 11 — Verification and Performance

Defines the evidence model, experiments, correctness checks, and proposed release gates used to decide whether HaloFPX is correct, faster, stable, and releasable.

Research status: source-backed planning complete for Sections 73-81; target-machine measurements and policy decisions remain open. No page in this category currently establishes a measured HaloFPX performance result.

## Authority map

- [73 — Benchmark Methodology, Terminology, Experimental Controls, and Data Schema](73_Benchmark_Methodology_Terminology_Experimental_Controls_and_Data_Schema/README.md) is authoritative for metric definitions and experiment controls. Schema 1.0.0 has executable structural validation; metric thresholds and machine-derived controls remain unapproved.
- [78 — Correctness, Regression, Determinism, and Model Quality Evaluation](78_Correctness_Regression_Determinism_and_Model_Quality_Evaluation/README.md) is authoritative for correctness and quality evidence.
- [81 — CI Matrix, Release Gates, Reproducibility, and Performance Regression Policy](81_CI_Matrix_Release_Gates_Reproducibility_and_Performance_Regression_Policy/README.md) is the proposed promotion gate. It remains contingent on validated methodology, target runners, variance, thresholds, and the experiments below.

## Sections

- [73 — Benchmark Methodology, Terminology, Experimental Controls, and Data Schema](73_Benchmark_Methodology_Terminology_Experimental_Controls_and_Data_Schema/README.md)
- [74 — Single-Node HIP and Vulkan Baseline Matrix](74_Single_Node_HIP_and_Vulkan_Baseline_Matrix/README.md)
- [75 — Fabric Microbenchmarks and GPU-to-Peer-GPU End-to-End Tests](75_Fabric_Microbenchmarks_and_GPU_to_Peer_GPU_End_to_End_Tests/README.md)
- [76 — Distributed Mode Benchmark Matrix and Break-Even Analysis](76_Distributed_Mode_Benchmark_Matrix_and_Break_Even_Analysis/README.md)
- [77 — HaloKV Restore, Writeback, Hit-Rate, and Endurance Benchmarks](77_HaloKV_Restore_Writeback_Hit_Rate_and_Endurance_Benchmarks/README.md)
- [78 — Correctness, Regression, Determinism, and Model Quality Evaluation](78_Correctness_Regression_Determinism_and_Model_Quality_Evaluation/README.md)
- [79 — Stress, Soak, Long-Context, Multi-Session, Power, and Thermal Testing](79_Stress_Soak_Long_Context_Multi_Session_Power_and_Thermal_Testing/README.md)
- [80 — Fault Injection: Cable Pulls, Restarts, OOM, Disk Full, and Corruption](80_Fault_Injection_Cable_Pulls_Restarts_OOM_Disk_Full_and_Corruption/README.md)
- [81 — CI Matrix, Release Gates, Reproducibility, and Performance Regression Policy](81_CI_Matrix_Release_Gates_Reproducibility_and_Performance_Regression_Policy/README.md)

## Cross-category dependencies

- Product requirements and candidate SLOs: [Section 09](../02_Project_Definition/09_Functional_Requirements_SLOs_and_Acceptance_Criteria/README.md)
- Model conversion and quality policy: [Section 31](../06_Models_Quantization_and_Inference/31_Conversion_Imatrix_Calibration_and_Quality_Validation/README.md)
- Distributed execution plans and fallback: [Sections 38-48](../07_Distributed_Runtime/README.md)
- Fabric requirements, topology, and transport: [Sections 49-55](../08_Fabric_and_Transport/README.md)
- HaloKV identity, durability, and recovery: [Sections 56-65](../09_HaloKV_Persistent_Cache/README.md)
- Server, security, deployment, and recovery behavior: [Sections 66-72](../10_Product_Server_and_Operations/README.md)

All machine results must route to `experiments/` with raw data and environment metadata before promotion into `[MEASURED]` wiki claims.
