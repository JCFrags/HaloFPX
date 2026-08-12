# 13. Gaps, stop conditions, and re-entry

## Open evidence gaps

[MISSING] Actual target distro and kernel build.

[MISSING] Kernel configuration and IOMMU/PASID state.

[MISSING] Exact loaded module provenance and UAPI compatibility.

[MISSING] Firmware package, symlink targets, and hashes.

[MISSING] AMD package hashes and EULA/third-party notice records.

[MISSING] Exact target model compile and operator assignment.

[MISSING] Correctness, latency, throughput, energy, thermal, memory, and stability measurements.

[MISSING] Recovery evidence after ordinary process failure, runtime error, or device timeout.

[MISSING] Exact Linux reranker support.

[UNKNOWN] Linux qualification of the public embedding sample.

[UNKNOWN] Draft-model integration interface and economics.

## Re-entry trigger

[DECISION] Re-entry requires a concrete artifact set, not a new marketing claim:

1. target read-only probe;
2. matched package and firmware manifest;
3. exact classifier model manifest;
4. compiler/cache manifest;
5. provider assignment report;
6. CPU/NPU correctness report;
7. raw performance and energy data;
8. stability logs;
9. teardown proof;
10. written confirmation that HIP/Vulkan scope is unchanged.

## Stop conditions

[DECISION] Retain `keep excluded` when any of the following remains true:

- target distro is outside the supported package baseline without a reproducible alternative;
- packages cannot be independently hashed and archived under acceptable terms;
- kernel/firmware/plugin versions must be mixed;
- IOMMU or native-host constraints cannot be met;
- NPU partition is small or mostly CPU;
- memory movement erases the expected benefit;
- accuracy or policy behavior regresses;
- stability requires module reload or reboot;
- the experiment consumes primary-path engineering capacity.

## Review date semantics

The decision is dated 2026-07-18. It is not a permanent statement about future AMD or Linux releases.

[DECISION] Until a complete re-entry artifact set supersedes this bundle, the operative state is `keep excluded`.
