# 10 — Product, Server, and Operations

## Category manifest

- **Purpose:** Define service behavior, deployment, security, observation, and recovery.
- **Authoritative files:** This manifest, the seven linked section artifact sets, and current Project Lead production authority.
- **Current owner:** Project Lead owns production transitions. Documentation workers own routing.
- **Status:** Populated and under evidence-backed revision. Machine validation remains open.
- **Last verified date:** 2026-07-29 for routing. Read current status before operations.
- **Source commits:** The [current Project Lead status](../../../project-management/lead/CURRENT_STATUS.md) routes exact deployment commits and binaries.
- **Related decisions:** [Project Lead decisions](../../../project-management/lead/DECISIONS.md) and [decision map](../decision-map.md).
- **Related evidence:** [Current state](../../../CURRENT_STATE.md) and [evidence map](../evidence-map.md).
- **Open work:** Resolve section policy questions. Do not infer production authority from Wiki research.
- **Next safe action:** Verify exact units, processes, binaries, listeners, health, and rollback before an authorized transition.

Defines the user-facing service and how it is installed, secured, observed, and maintained.

Research status: populated and under evidence-backed revision.
Sections 66–72 remain `needs-machine-validation`.
Required machine experiments and policy decisions remain open.

- [66 — OpenAI-Compatible API, Server Semantics, and Error Model](66_OpenAI_Compatible_API_Server_Semantics_and_Error_Model/README.md)
- [67 — Configuration, Hardware Profiles, Model Manifests, and Plan Manifests](67_Configuration_Hardware_Profiles_Model_Manifests_and_Plan_Manifests/README.md)
- [68 — Model Lifecycle, Session Lifecycle, Admission Control, and Routing](68_Model_Lifecycle_Session_Lifecycle_Admission_Control_and_Routing/README.md)
- [69 — CLI, Admin API, Diagnostics, Health, Metrics, Logs, and Traces](69_CLI_Admin_API_Diagnostics_Health_Metrics_Logs_and_Traces/README.md)
- [70 — Packaging, systemd, Containers, Deployment, and Cold-Boot Procedure](70_Packaging_systemd_Containers_Deployment_and_Cold_Boot_Procedure/README.md)
- [71 — Security, Trust Boundaries, Permissions, Local Network, and Secrets](71_Security_Trust_Boundaries_Permissions_Local_Network_and_Secrets/README.md)
- [72 — Upgrades, Rollbacks, Protocol and Cache Migration, Backup, and Runbooks](72_Upgrades_Rollbacks_Protocol_and_Cache_Migration_Backup_and_Runbooks/README.md)

Cross-section contracts use three canonical health routes:

- `GET /health/live`;
- `GET /health/ready`; and
- `GET /health/startup`.

Deployment and rollback require separate records for these phases:

- installation;
- activation pointer;
- process;
- readiness;
- traffic;
- durable state; and
- rollback.

All required phases must succeed.
An activation-pointer change is not a runtime cutover.

> This category contains maintained working guidance.
> The guidance is not permanent truth.
> Recheck exact source pins and machine evidence before promotion.
> Recheck unresolved decisions and cross-section contracts before promotion.
