# 7. Scheduling, telemetry, power management, and reset boundary

## Scheduling

[UPSTREAM] The architecture supports mixed spatial and temporal scheduling of array columns.

[UPSTREAM] The driver resource solver allocates columns using workload metadata and QoS hints; firmware enforces context-to-column binding.

[UPSTREAM] The pinned npu5 definition allows 16 hardware contexts.

[UPSTREAM] Each hardware context uses a DRM scheduler/entity and fence/sync-object based completion.

[UPSTREAM] The captured implementation defines a default firmware/resource time quantum of 30,000 microseconds and a scheduler timeout path. These implementation values are not product service-level guarantees.

[UNKNOWN] Cross-process fairness, tail latency under contention, QoS enforcement, and interaction with desktop AI workloads are not measured.

## Telemetry

[UPSTREAM] The pinned UAPI includes:

- firmware version;
- AIE metadata and status;
- clocks;
- power;
- per-column utilization;
- hardware contexts;
- command submissions/completions;
- migrations, preemptions, suspensions, and errors;
- resource information;
- firmware telemetry maps;
- buffer usage and DRM client memory statistics.

[VENDOR-ONLY] AMD warns that telemetry and array queries may fail when userspace is newer than the loaded in-tree driver.

[INFERENCE] Telemetry surface availability is not proof of accuracy, sampling stability, or compatibility with an arbitrary monitoring stack.

## Suspend and resume

[UPSTREAM] System suspend stops contexts and hardware.

[UPSTREAM] Resume starts the hardware, reloads/initializes firmware, and attempts to resume contexts.

[UNKNOWN] Transparent application behavior across suspend/resume is not validated on the target.

[DECISION] The initial bounded experiment must not claim suspend/resume support. It may record naturally occurring events but must not initiate system suspend.

## Error and reset behavior

[UPSTREAM] Firmware can report asynchronous errors and capture register state for a faulting context.

[UPSTREAM] The UAPI exposes context errors and fatal-error fields.

[UNKNOWN] A deterministic, production-safe user-triggered reset/recovery contract was not established from the captured boundary.

[VENDOR-ONLY] Firmware/driver mismatch is a known source of command aborts and mailbox timeouts.

[DECISION] The read-only probe never resets, reloads, unbinds, suspends, or writes device state. Any experiment encountering a device disappearance, mailbox timeout, command abort, or required module reload fails the stability gate.
