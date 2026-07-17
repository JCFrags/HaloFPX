# Status

| Item | State |
|---|---|
| Upstream source | Pinned to `86d86ed4396b4130922f7b9af26e3d9fc11a591b` for the research snapshot |
| ROCmFPX source | Pinned to `a5605a72768c6562241b248e268e33dc92787394` for the research snapshot |
| Requested CachyLLama identity | **Unresolved**: `llama-ai/CachyLlama` was not available during research |
| Provisional CachyLLama candidate | `fewtarius/CachyLLama` at `6be745998f568e379ea197fcf827baec73ff9940` |
| Future integration fork | **Unpinned** |
| Test cases | 175 |
| Fixture IDs | 161 |
| Approved numeric references | 0 |
| Approved tolerance profiles | 0 |
| Bundled large model binaries | 0 |

> [!CAUTION]
> The CachyLLama candidate is useful for suite design because its public README and sources expose persistent SSD cache, compatibility hashing, user isolation, and page-manager behavior. Confirm repository ownership and the intended integration source before treating it as a release gate.

## Readiness checklist

- [x] Requested domains represented in the matrix.
- [x] Upstream test reuse mapped.
- [x] Structural, exact, numeric, distributional, and metamorphic oracle types separated.
- [x] Failure-injection and cancellation cases specified.
- [x] CI lanes and reference-promotion workflow specified.
- [x] Input fixtures and mutation recipes included.
- [ ] Integration fork repository and commit supplied.
- [ ] CachyLLama identity confirmed.
- [ ] External model fixtures licensed, downloaded, and hash-locked.
- [ ] Hardware/driver lanes named and provisioned.
- [ ] Numeric and distributional profiles calibrated and approved.
- [ ] End-to-end observations collected.
