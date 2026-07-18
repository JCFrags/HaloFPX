# Upstream tests and maintainer statements

## Tests that merit reuse

* `rccl-tests` with MPI: multi-node correctness and performance sweeps for standard collectives.
* RCCL MPI framework: hostfile/network-interface controlled distributed tests.
* `NetSocketTests.cpp`: local Socket request/progress unit coverage.
* Active `RevokeMPITests.cpp` and `GrowMPITests.cpp`: version-specific recovery API coverage.
* Active timeout tests: specific `ncclTimeout` producers and async-state handling.

## Tests that do not establish the target

The Socket unit test runs locally. The generic MPI suite does not encode cable or NIC topology. The active timeout tests do not establish a universal Socket deadline. No audited test fixture names two gfx1151 hosts connected by Ethernet-over-USB4.

## Maintainer/report chronology

1. A 2025 standalone issue received an initial “no plans” maintainer response.
2. The same maintainer later redirected tracking to monorepo issue #2788.
3. PR #3415 merged gfx1151 enablement with explicit single-GPU-only validation.
4. PR #4875 merged Strix Halo tuning and reported 1–4 nodes over a 10 Gbps Ethernet switch.

This chronology prevents an obsolete issue comment from being mistaken for the current source state while preserving its historical context.
