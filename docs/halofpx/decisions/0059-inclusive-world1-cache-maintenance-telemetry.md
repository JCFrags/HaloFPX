# ADR-0059: inclusive world-1 cache-maintenance telemetry

Status: accepted for the default-off world-size-1 native response path and
hosted correctness evidence only. Positive cache reuse and Strix Halo benefit
qualification remain open.

Date: 2026-08-13

## Context

ADR-0054 exposes narrow `lookup_total_ns` and
`state_install_cleanup_ns` phase clocks, but server `prompt_ms` begins after
slot selection, persistent-cache transition, and synchronous idle-slot cache
work. Those clocks therefore cannot attribute the cache-related portion of
client-observed time to first token. GitHub issue #18 requires truthful
request-local cache attribution before any selection-policy or speed claim.

The current product has no trusted world-1 live-loader authority, so a
positive server hit remains structurally dormant. The existing A/B v1 adapter
also drops `halofpx_cache` and requires an uncached prompt. This decision can
make the native product object ready for later evidence without pretending
that host-only cold execution qualifies cache benefit.

## Decision

Extend only the compile-gated, native, nonstreaming world-1 product response.
Feature-OFF binaries contain neither the new field literals nor the product
library. Runtime-off, legacy exact paths, OpenAI-compatible responses, and the
A/B v1 schema do not gain the object or any of these fields.

The product object reports four non-overlapping request-local clocks:

1. `selected_slot_transition_ns` covers one successful automatic
   `get_available_slot()` transition, including synchronous run-local prompt
   cache save/load/update work performed by that transition. Its measured bit
   becomes true only when a concrete slot is returned. A failed scan followed
   by deferral and queue wait is not committed. An explicit `id_slot` does not
   execute automatic selection and reports false/zero.
2. `lookup_total_ns` starts after the product-active authority/catalog gate and
   covers cache-only slot/request/profile preparation, exact-session
   derivation, and authenticated catalog/selector lookup, including early cold
   outcomes. It stops before installation begins.
3. `state_install_cleanup_ns` starts after lookup has stopped and covers
   install validation, complete or failed state apply, snapshot cleanup,
   destination token/checkpoint installation, and every rollback path.
4. `postlaunch_idle_slot_saves_ns` covers the synchronous configured
   idle-slot save/update/clear block after launch. Entering that block sets the
   measured bit even when the steady-clock result is zero. A disabled or
   otherwise unentered block reports false/zero.

Every queued attempt clears the attempt-scoped clocks and semantic apply-byte
fields before starting a new clock. This prevents a deferred task from
carrying a prior scan or transition into its eventual response. The lookup and
install phase timers use single-owner finish-on-scope-exit recording, and the
lookup timer is explicitly stopped before the install timer begins, so nested
validation clocks are not summed twice.

`preprompt_cache_maintenance_ns` is the checked sum of those four components.
Canonical not-run phases contribute zero. An unsigned overflow, or a nonzero
duration paired with an unmeasured optional phase, makes the aggregate
false/zero while preserving all component values. A valid aggregate can be
zero; the measured bits distinguish executed-zero from not-run.

`state_apply_input_bytes` is the semantic byte count supplied to a completely
accepted state-apply API call. Its validity bit is false and the value is zero
when apply was not attempted, rejected, failed, or partial. If the state API
accepted the complete input but later slot-token installation fails and rolls
the live slot back to cold, the successful semantic apply observation remains
valid. This is not a physical-read, unique-read, storage-transfer, memory-copy,
or total-I/O byte count. The response adds no `physical_bytes`, `read_bytes`,
or `total_io_bytes` claim.

The aggregate is inclusive only for the four defined cache-maintenance phases.
It excludes HTTP handling, tokenization, queue wait, task-launch overhead,
ordinary prompt evaluation, and generation. Client-observed wall time and
time-to-first-token remain the net-benefit arbiters. Consumers must retain the
component clocks, measured/valid bits, final server prompt accounting, and
client clock together rather than subtracting unmatched server clocks.

Explicit-slot native product requests are observable but remain ineligible for
persistent prefix restoration. They emit the cache object with the automatic
selection component false/zero and retain the fail-closed cold path.

## Qualification boundary

Four focused arithmetic rows cover canonical not-run, measured zero, an exact
nonzero sum, and overflow with unchanged components. The same product test
covers successful semantic apply bytes plus rejected, not-run, and partial
apply as zero/invalid. The feature contract checks the full field set and
forbidden ambiguous byte names in source, then checks field-literal presence in
the product-ON server binary and absence in the product-OFF binary.

Fresh Release CPU builds on Ubuntu WSL2 qualify product ON with nine focused
contracts and product OFF with its feature contract. A real Qwen3-0.6B
ROCmFPX fixture smoke exercises the server scheduler and serializer: automatic
selection, explicit-slot no-selection, idle block enabled/disabled, exact
aggregate equality, successful mutation through the moved slot task, and
runtime-off object absence. This is host correctness evidence, not target
performance evidence.

No trusted authority, positive persistent hit, physical/read byte accounting,
cache-specific A/B v2 harness, automatic system-prefix capture, two-rank
composition, or matched Strix Halo result is added. Issue #18 therefore remains
open. The issue #41 target-maintenance boundary remains controlling.

## Relationship and rollback

This decision supersedes only ADR-0054's narrow timing-boundary statement. It
does not change cache identity, selection, stored bytes, catalog serialization,
authority, publication, or corruption behavior. ADR-0054 remains the product
shell authority.

Rollback removes the additive native response fields, attempt clocks, semantic
apply-byte accounting, focused tests, L10g evidence, and this decision. The
product remains default OFF and no stored artifact requires migration.
