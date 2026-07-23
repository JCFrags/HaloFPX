# ADR-0046: Fresh RPC model residency precedes restore staging

- Status: accepted for the default-off diagnostic harness
- Date: 2026-07-23
- Base: `5616abb2c19c1611c3852575270ad41b43085921`

## Decision

A true RPC-worker-restart restore uses two material coordinator model
residencies. The executable order is:

1. coordinator/model residency A captures state while bound to worker epoch A;
2. coordinator A terminates;
3. worker A stops;
4. worker B starts with a different PID and InvocationID and passes exact CAPS;
5. coordinator/model residency B loads all model allocations against worker B;
6. coordinator B publishes `model-ready`;
7. the controller re-reads worker B PID/InvocationID, validates the complete
   A-to-B authority tuple, and only then publishes `restore-authorized`;
8. coordinator B may stage and apply state.

The capture epoch is retained in a durable audit receipt. It is intentionally
not required to equal the restore epoch. A missing or ambiguous epoch, reused
worker/coordinator identity, wrong stop order, staging before model readiness,
or worker change after model load refuses restore.

## Implementation boundary

The restore test process loads the distributed model and context before
publishing `model-ready`. Its restore code cannot read coordinator or worker
artifacts until the controller-owned authorization marker exists. The runner
calls the epoch/model-residency validator before creating that marker.

This does not rehydrate old remote handles or reconnect an existing model
residency. It changes diagnostic lifecycle authority only; cache behavior
remains compile- and runtime-default-off.

