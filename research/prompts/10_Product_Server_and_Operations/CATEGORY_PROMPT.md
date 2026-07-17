# Category Research Agent Prompt — 10: Product, Server, and Operations

The project is a custom local LLM inference stack for two matched AMD Strix Halo systems connected by two tuned, low-latency USB4 host-to-host links. The intended source base is a fork of `charlie12345/ROCmFPX`, selectively incorporating the persistent SSD-backed KV-cache and agent-serving ideas from `fewtarius/CachyLLama` and `fewtarius/llama-ai`, while tracking `ggml-org/llama.cpp`. The product must choose among replication, remote speculative decoding, two-way tensor parallelism, pipeline parallelism, and MoE-aware hybrid execution, with a rank-local persistent cache and a specialized dual-link transport. The wiki will be consumed by local Codex and human engineers before and during implementation.

Build the complete wiki category **10: Product, Server, and Operations**. Research every numbered section below as a separate self-contained folder:

- `66_OpenAI_Compatible_API_Server_Semantics_and_Error_Model/` — 66: OpenAI-Compatible API, Server Semantics, and Error Model
- `67_Configuration_Hardware_Profiles_Model_Manifests_and_Plan_Manifests/` — 67: Configuration, Hardware Profiles, Model Manifests, and Plan Manifests
- `68_Model_Lifecycle_Session_Lifecycle_Admission_Control_and_Routing/` — 68: Model Lifecycle, Session Lifecycle, Admission Control, and Routing
- `69_CLI_Admin_API_Diagnostics_Health_Metrics_Logs_and_Traces/` — 69: CLI, Admin API, Diagnostics, Health, Metrics, Logs, and Traces
- `70_Packaging_systemd_Containers_Deployment_and_Cold_Boot_Procedure/` — 70: Packaging, systemd, Containers, Deployment, and Cold-Boot Procedure
- `71_Security_Trust_Boundaries_Permissions_Local_Network_and_Secrets/` — 71: Security, Trust Boundaries, Permissions, Local Network, and Secrets
- `72_Upgrades_Rollbacks_Protocol_and_Cache_Migration_Backup_and_Runbooks/` — 72: Upgrades, Rollbacks, Protocol and Cache Migration, Backup, and Runbooks

Apply `../OUTPUT_STANDARD.md` to every section. Use current primary sources, exact commits/versions/dates, and explicit claim labels. Separate Internet research from required on-machine investigation. Do not invent measurements or silently resolve conflicting evidence.

Return one downloadable folder named `10_Product_Server_and_Operations/` containing the numbered section folders at the exact paths listed above, plus a category `README.md` that summarizes the category, identifies authoritative pages, links all sections, and highlights cross-category dependencies.
