# 10 — Product, Server, and Operations

Defines the user-facing service and how it is installed, secured, observed, and maintained.

Research status: populated and under evidence-backed revision; Sections 66-72 remain `needs-machine-validation` until their required machine experiments and open policy decisions are resolved.

- 66 — OpenAI-Compatible API, Server Semantics, and Error Model
- 67 — Configuration, Hardware Profiles, Model Manifests, and Plan Manifests
- 68 — Model Lifecycle, Session Lifecycle, Admission Control, and Routing
- 69 — CLI, Admin API, Diagnostics, Health, Metrics, Logs, and Traces
- 70 — Packaging, systemd, Containers, Deployment, and Cold-Boot Procedure
- 71 — Security, Trust Boundaries, Permissions, Local Network, and Secrets
- 72 — Upgrades, Rollbacks, Protocol and Cache Migration, Backup, and Runbooks

Cross-section contracts in this category use the canonical health routes `GET /health/live`, `GET /health/ready`, and `GET /health/startup`. Deployment and rollback are complete only after the separately recorded install, activation-pointer, process, readiness, traffic, durable-state, and rollback phases have succeeded; changing a pointer alone is not a runtime cutover.

> This category is maintained working guidance, not permanent truth. Recheck exact source pins, machine evidence, unresolved decisions, and cross-section contracts before promoting an operational baseline.
