# L10g inclusive world-1 cache-maintenance telemetry

Status: **implemented and locally host-qualified for truthful native response
accounting; target cache benefit remains unmeasured**

The accepted boundary is recorded in
[ADR-0059](decisions/0059-inclusive-world1-cache-maintenance-telemetry.md).
This slice completes the request-local product telemetry needed before a
cache-specific client harness can arbitrate net benefit. It does not complete
GitHub issue #18.

## Implemented

**[VERIFIED]** The native world-1 product response now carries a checked
preprompt cache-maintenance aggregate and its four non-overlapping components:
automatic selected-slot transition, full product lookup preparation/lookup,
state install through token installation or rollback, and synchronous
postlaunch idle-slot saves. Optional selected-slot and idle clocks have
measured bits, so executed-zero and not-run are distinct.

**[VERIFIED]** Deferred tasks clear attempt measurements before every retry.
Failed availability scans and queue time are not credited to the selected-slot
transition. Explicit slot routing remains ineligible for persistent prefix
restore and reports the automatic transition false/zero. The postlaunch phase
writes through the task after ownership has moved into the launched slot.

**[VERIFIED]** The aggregate is a checked `uint64_t` sum. Overflow or a
noncanonical unmeasured/nonzero optional phase returns false/zero without
altering the components. Lookup and install use disjoint outer boundaries, so
the selector's internal validation clock is not added a second time.

**[VERIFIED]** `state_apply_input_bytes` reports only semantic bytes supplied
to a completely accepted state-apply call. Rejected, failed, partial, and
not-run apply report zero/false. The count is retained if a later slot-token
allocation rolls the already-applied state back to cold. No physical-read,
unique-read, storage-transfer, memory-copy, or total-I/O byte count is exposed.

**[VERIFIED]** Compile-time product OFF removes the field literals and product
library from the server build graph. Runtime mode OFF and legacy responses omit
`halofpx_cache`. The existing A/B v1 capture schema is unchanged; client
wall/TTFT remains the required arbiter for a future cache-specific harness.

## Host correctness evidence

**[MEASURED] (off-target WSL2, 2026-08-13):** Fresh Release CPU feature-ON and
feature-OFF builds completed with GCC 15.2.0, CMake 4.2.3, and Ninja 1.13.2.
The ON build passed nine focused exact-session, state-transformer,
catalog/product, selector, golden-vector, and feature contracts. The OFF build
passed its feature contract; four new field markers and the product target were
absent from its binary/build graph.

**[MEASURED] (off-target WSL2, 2026-08-13):** The immutable
Qwen3-0.6B-Q3_0_ROCMFPX pure fixture with SHA-256
`d1404c1afc61ffe49357c14c6d3dbfb252a72e87744fb7e491e7a2e205321fff`
produced identical four-token output `[279, 3409, 429, 374]` in all paths.
The no-idle automatic request reported selected transition true/12,185 ns,
idle false/zero, and aggregate 12,185 ns. Explicit `id_slot=0` reported
selected false/zero and aggregate zero. The idle-enabled automatic request
reported selected true/57,306 ns, idle true/124 ns, and aggregate 57,430 ns.
Runtime mode OFF emitted no cache object. The live-authority fallback kept
lookup/install zero and semantic apply bytes false/zero, as required.

These nanosecond values describe one local WSL correctness smoke while three
CPU servers ran concurrently. They are retained only to prove field wiring and
arithmetic, not to compare performance.

## Remaining gate

**[OPEN]** Issue #18 still needs a cache-specific client schema that retains
the native object, separately justified validation/read observations, and
matched client wall/TTFT on the real Strix Halo Linux machines. Positive
persistent reuse also remains blocked on a trusted world-1 live authority and
model-backed suffix replay. Atomic two-rank reuse remains owned by issue #26.

The retained smoke directory contains the
[run observations](evidence/l10g-inclusive-cache-telemetry-smoke/server-observations.txt)
and normalized JSON responses. No standalone portable telemetry receipt exists
for this slice.

## Rollback

Disable the already-default-OFF product gate or revert the additive telemetry
commit. Cache records and catalog serialization are unchanged.
