# 07 — Distributed Runtime

Defines how two Strix Halo machines cooperate and when each parallel execution mode is used.

Research status: source-backed architecture draft complete; protocol implementation, fault behavior, and two-node break-even evidence remain open.

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

All eleven sections remain `needs-machine-validation`. No execution mode is selected until correctness, ownership, recovery, transport, and matched baseline gates pass; the single-node fallback remains mandatory.
