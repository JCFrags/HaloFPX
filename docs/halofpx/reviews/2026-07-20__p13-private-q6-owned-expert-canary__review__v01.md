# P13 private Q6 owned-expert canary independent review

Status: **accepted; no blockers**

The review inspected the complete CMake, HIP helper, registry, standalone-test,
documentation, and receipt diff. It independently checked buffer and shape
authority, device-validation ordering, invalid-ID safety, feature-off symbol
absence, arithmetic, scope, provenance, rollback, and the decision not to open
RPC/model integration.

The first review found a blocking oracle defect: compact IDs, gathered
activations, and scatter slots came from the helper under test, while all
expert weights were identical. That could hide incorrect compaction or
placement. Before acceptance, the test was changed to derive oracle IDs,
activations, and slots independently on the host, compare the helper's compact
metadata and gathered values directly, and use distinguishable quantized
patterns for every selected local expert under both ownership bases.

The corrected test was rebuilt and rerun in five fresh processes on nimo-2.
Both bases remained bit-exact; invalid base, out-of-range ID, duplicate ID, and
wrong-owned-count behavior passed. Refreshed test/log hashes, measurements, and
the v2 evidence-bundle hash agree across the synthesis and receipt.

The reviewer recomputed means of 87.7262 and 109.4290 microseconds, ratio
0.801672317210246, saving 21.7028 microseconds, and optimistic three-projection
share 0.108514% of 60 ms. The result remains correctly bounded as a direct
microbenchmark rather than end-to-end, non-inferiority, or greater-than-30
tok/s evidence.

Feature-off compilation contains no private proc string. Invalid device state
cannot address weights because scratch begins zeroed, validation and gather are
stream ordered, and invalid status prevents gather/scatter publication. The
default-off private canary is accepted for retention; closing RPC and model
integration is conservative and justified by its small absolute leverage.
