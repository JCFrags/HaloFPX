# L05p registry-lab read-only operations 1-4 review v01

- Date: 2026-07-18
- Scope: fake-only portable execution of operations 1-4 under ADR-0019,
  ADR-0020, and ADR-0021
- Final verdict: **ACCEPT**

## Delivered boundary

The milestone adds an internal, fixed-capacity, allocation-free step engine for
guard acquisition, writer-lock acquisition, under-lock preflight, and snapshot
load. It exhaustively freezes the 600-product algebra while admitting exactly
43 operation 1-4 products. The all-`ok` path exposes only private trace event
200 and no ordinary result. Operation 5, decoding, mutation, Linux I/O,
persistent writes, product linkage, and authority remain unavailable.

The fake state separates live and durable namespace, bytes, length,
completeness, envelope metadata, unexpected-entry metadata, and directory
projections. Restart images are fieldwise, durable-only, canonical, and
validated in full before any restore mutation. Invalid, oversized,
non-canonical, or inconsistent images reject atomically. A valid modeled
process restart permanently kills, wipes, and audits every live invocation in
that process without disturbing another process's ownership.

## Adversarial review and repairs

Three independent review rounds were required. Promotion was held while the
implementation repaired:

1. durable-only envelope and unexpected-entry projections;
2. process-wide restart teardown for paused invocations;
3. non-elidable credential wiping and computed secret-exclusion evidence;
4. exact restart invariant rejection rather than length clamping;
5. the repository's registered non-NUL ASCII `0x01..0x7f` credential profile;
6. Windows and Linux read-only-archive allocator-import rejection;
7. lossless per-dead-slot teardown evidence and exhaustion behavior;
8. hostile enum, move/overwrite, differential secret, restart, boundary, and
   capacity tests;
9. removal of a temporary stack secret copy; and
10. a computed operation-90 zeroization gate before operation 91 can release
    the writer lock.

The final independent verdict was ACCEPT. The review also confirmed the
seven-archive read-only closure, preserved six-archive wire closure, product
reverse-edge exclusion, donor/provenance scans, and test-only result boundary.

## Verification

| Gate | Result |
|---|---|
| Windows Release registry-lab matrix | Pass, 7/7 |
| Windows Debug registry-lab matrix | Pass, 7/7 |
| Windows full configured Release suite | Pass, 84/84 |
| Windows Release repeated focused executions | Pass, 1,000/1,000 in 77.30 s |
| Windows Debug repeated focused executions | Pass, 200/200 in 24.68 s |
| nimo-2 ASan/UBSan registry-lab matrix | Pass, 6/6 |
| nimo-2 ASan/UBSan repeated focused executions | Pass, 1,000/1,000 in 61.41 s |
| Independent adversarial rereview | ACCEPT |
| Immutable reference repositories | Clean at locked commits and trees, 4/4 |
| Configured implementation remote | None |

An earlier sanitizer repeat passed 68 processes before the kernel killed
process 69 under global OOM with the approximately 200 GB distributed model
loaded. Kernel evidence attributes the kill to global memory pressure; the
test unit peaked at 329.6 MiB. This failure is retained separately. After the
coordinator and worker were stopped in dependency order, available nimo-2
memory rose from 13 GiB to 122 GiB and the 1,000-process run completed. This is
an operational qualification lesson, not a suppressed result.

The exact hashes, commands, environments, failure evidence, and raw repeat logs
are recorded in
`evidence/l05p-registry-lab-read-only-ops14-repeat-receipt.json`.

## Rollback-control verification

The nimo-2 RPC worker was restored before the nimo-1 model server. Both
services are active, ports 50052 and 8081 listen, `/health` returns HTTP 200,
and a real chat completion returned exact content `HALOFPX_ROLLBACK_OK`. The
observed generation rate near 18.47 tokens/s is only an operational smoke
signal and is not a matched performance baseline.

## Wiki, provenance, and promotion boundary

The implementation matches ADR-0019's internal-final fake boundary,
ADR-0020's closed no-mutation algebra and process-death rule, and ADR-0021's
execution, cleanup, restart, allocation, and graph contracts. No donor
implementation was used. No GPL llama-ai implementation or separately
licensed documentation entered the MIT engine; no CachyLLama code was
admitted; the direct-cherry-pick roster remains empty.

Promotion proves the fake-only operations 1-4 execution contract. It does not
prove operation-5 decoding/classification, a Linux provider, filesystem crash
durability, persistence, cache reuse, runtime compatibility, or performance
non-regression. All such gates remain closed.
