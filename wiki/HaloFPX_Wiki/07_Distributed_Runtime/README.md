# 07 — Distributed Runtime

## Category manifest

- **Purpose:** Define two-node execution modes, ownership, scheduling, failure, and fallback.
- **Authoritative files:** This manifest, the 11 linked section artifact sets, and accepted implementation decisions.
- **Current owner:** Distributed-runtime workers own implementation evidence. Documentation workers own routing.
- **Status:** Source-backed architecture draft complete. Protocol and break-even validation remain open.
- **Last verified date:** 2026-07-29 for routing. Section claims retain their own dates.
- **Source commits:** Exact implementation commits remain in linked decisions. Research uses ROCmFPX `a5605a72768c6562241b248e268e33dc92787394`.
- **Related decisions:** [Decision map](../decision-map.md), including architecture decision records (ADRs) 0048 and 0049, and Decision 0050.
- **Related evidence:** [Evidence map](../evidence-map.md) and [Fabric and Transport](../08_Fabric_and_Transport/README.md).
- **Open work:** Select execution modes only after correctness, ownership, recovery, and matched performance gates pass.
- **Next safe action:** State rank ownership, failure behavior, and single-node fallback before implementation.

This category defines how two Strix Halo machines cooperate.
It also defines when the project can use each parallel execution mode.

Research status: source-backed architecture draft complete; protocol implementation, fault behavior, and two-node break-even evidence remain open.

HIP is the AMD graphics processing unit portability programming model.

- [38 — Distributed Runtime Goals, Cost Model, and Mode Selection](38_Distributed_Runtime_Goals_Cost_Model_and_Mode_Selection/README.md)
- [39 — Coordinator, Rank Worker, Session, and Persistent-Graph Architecture](39_Coordinator_Rank_Worker_Session_and_Persistent_Graph_Architecture/README.md)
- [40 — Full Replication, Request Routing, and Session Affinity](40_Full_Replication_Request_Routing_and_Session_Affinity/README.md)
- [41 — Remote Draft-Node Speculation](41_Remote_Draft_Node_Speculation/README.md)
- [42 — Two-Way Tensor Parallelism and Collective Placement](42_Two_Way_Tensor_Parallelism_and_Collective_Placement/README.md)
- [43 — Contiguous Layer Pipeline Parallelism and Microbatching](43_Contiguous_Layer_Pipeline_Parallelism_and_Microbatching/README.md)
- [44 — MoE-Aware Hybrid Distribution and Hot-Expert Replication](44_MoE_Aware_Hybrid_Distribution_and_Hot_Expert_Replication/README.md)
- [45 — Persistent Rank Protocol, Command Rings, and Graph Reuse](45_Persistent_Rank_Protocol_Command_Rings_and_Graph_Reuse/README.md)
- [46 — Scheduler, Continuous Batching, Backpressure, and Concurrency](46_Scheduler_Continuous_Batching_Backpressure_and_Concurrency/README.md)
- [47 — Topology Planner, Autotuner, and HIP-versus-Vulkan Selection](47_Topology_Planner_Autotuner_and_HIP_versus_Vulkan_Selection/README.md)
- [48 — Distributed Correctness, Determinism, Fault Recovery, and Degraded Mode](48_Distributed_Correctness_Determinism_Fault_Recovery_and_Degraded_Mode/README.md)

All eleven sections remain `needs-machine-validation`.
The project will not select an execution mode until the required gates pass.
Those gates cover correctness, ownership, recovery, transport, and matched baselines.
The single-node fallback remains mandatory.
