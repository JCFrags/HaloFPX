# Validation

**Validation date:** 2026-07-17

## Suite self-validation

| Check | Result |
|---|---|
| Matrix integrity | PASS — 175 unique cases across all 15 requested areas |
| Failure-injection inventory | PASS — 55 cases marked for bounded fault injection |
| Fixture coverage | PASS — 161 logical fixture IDs, all referenced IDs present in the manifest |
| Included fixture digests | PASS — included/recipe files match their manifest SHA-256 |
| JSON Schema validation | PASS for matrix cases and fixture manifest |
| JSON / JSONL / YAML parsing | PASS |
| Python compilation | PASS |
| Shell syntax (`bash -n`) | PASS |
| Harness unit tests | PASS — 18 tests |
| CLI case selection | PASS |
| Example observation validation | PASS |
| Utility smoke tests | PASS for fixture locking, passkey/template generation, GGUF byte mutation, proposed tolerance evidence, proposed reference records, and report aggregation |
| Editable harness installation | PASS in the validation environment using the available setuptools backend |

## Expected warnings

Five operator-supplied model classes remain without SHA-256 values:

- embedding model;
- long-context model;
- MTP-capable model;
- F16/BF16 numeric reference model;
- target/draft speculative pair.

This is intentional. Their exact model selections, licenses, and digests must be approved before the corresponding cases can execute. Additional generated model/quant variants are locked after materialization.

## Not executed

This artifact has not run the four source forks, model inference, GPU kernels, persistent cache, server endpoints, MTP, or RPC. Those require the program's exact integration repository, confirmation of the intended CachyLLama source, external model locks, builds, and named hardware lanes.

Accordingly, this validation establishes that the **suite design and harness scaffold are internally consistent**. It does not claim cross-fork conformance results.
