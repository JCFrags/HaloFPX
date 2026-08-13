# HaloFPX Decision Map

Status: authoritative routing index

Last verified: 2026-08-13

This page routes workers to decision authority.
This page does not replace any decision record.

## Project decisions

- The [Project Lead decisions](../../project-management/lead/DECISIONS.md) control active boundaries.
- The [current Project Lead status](../../project-management/lead/CURRENT_STATUS.md) identifies current work.
- The [project goal](../../PROJECT_GOAL.md) defines the ordered end state.
- The [Project Lead objectives](../../project-management/lead/OBJECTIVES.md) define product and performance gates.

## Implementation decisions

- [HaloFPX implementation decision index](../../../docs/halofpx/decisions/README.md)

Current accepted architecture decision records (ADRs) include:

- [ADR-0048](../../../docs/halofpx/decisions/0048-composed-scheduler-rpc-execution-authority.md):
  composed scheduler and remote procedure call execution authority.
- [ADR-0049](../../../docs/halofpx/decisions/0049-l63-real-lifecycle-preexecute-authority.md):
  real-lifecycle pre-execute authority.
- [Decision 0050](../../../docs/halofpx/decisions/0050-l67-retained-adr0049-foundation.md):
  retain the L67 ADR-0049 default-off foundation.
- [ADR-0051](../../../docs/halofpx/decisions/0051-default-off-exact-longest-prefix-selector.md):
  default-off exact longest-prefix selector.
- [ADR-0052](../../../docs/halofpx/decisions/0052-standalone-live-cache-authority.md):
  standalone live-derived cache authority.
- [ADR-0054](../../../docs/halofpx/decisions/0054-default-off-world1-prefix-product-shell.md):
  default-off world-1 authenticated-prefix product shell.
- [ADR-0056](../../../docs/halofpx/decisions/0056-versioned-strix-ab-runtime-comparison.md):
  versioned Strix A/B runtime comparison.
- [ADR-0057](../../../docs/halofpx/decisions/0057-offline-strix-maintenance-admission-controller.md):
  offline-only issue-#41 Strix maintenance admission controller.
- [ADR-0058](../../../docs/halofpx/decisions/0058-default-off-world1-live-authority-install-boundary.md):
  default-off world-1 live-authority install boundary; no positive source.
- [ADR-0059](../../../docs/halofpx/decisions/0059-inclusive-world1-cache-maintenance-telemetry.md):
  inclusive world-1 cache-maintenance telemetry.
- [ADR-0065](../../../docs/halofpx/decisions/0065-offline-independent-two-node-recovery-watchdog.md):
  offline-only independent two-node recovery-watchdog model.

Current proposed architecture decision records include:

- [ADR-0064 (proposed)](../../../docs/halofpx/decisions/0064-offline-strix-hmm-admission-snapshot.md):
  offline closed-world Strix HMM admission snapshot.
- [ADR-0066 (proposed)](../../../docs/halofpx/decisions/0066-offline-two-node-maintenance-nonce-protocol.md):
  offline two-node maintenance nonce protocol and explicit distributed
  uncertainty boundary.

Read each complete decision in the HaloFPX implementation repository.
Do not copy a decision into the Wiki as a replacement authority.

## Decision use

1. Confirm that the decision status is accepted.
2. Confirm that the decision applies to the current commit and task.
3. Read linked evidence and independent review.
4. Preserve rejected alternatives and negative results.
5. Request Project Lead authority when the required choice is absent.

## Immutable history

Do not rewrite accepted or rejected decision history.
Add a new decision when an approved choice changes.
Link the new decision to the superseded record.
