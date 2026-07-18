# Security-relevant changes after the known b3561 RPC fix

[VERIFIED] This ledger includes published advisories and non-advisory hardening that materially changes an input boundary, validation behavior, authentication/default exposure, or failure containment.

| Label | Date | Exact commit | Change | Class | Candidate status | Disposition |
|---|---|---|---|---|---|---|
| [VERIFIED] | 2024-08-12 | `1f67436c5ee6f4c99e71a8518bdfc214c27ce934` | Enable RPC in released builds | attack-surface expansion | Downstream release workflow retains equivalent explicit enablement | Reverse in standard release; isolate RPC artifact. |
| [VERIFIED] | 2024-08-19 | `18eaf29f4c5f7c8e89412f3a9a0392a9e4e01d75` | Reject invalid commands and contain allocation failures | malformed-input/DoS hardening | Semantic checks present | Keep sentinel coverage. |
| [VERIFIED] | 2024-10-08 | `458367a90606448a9c0262b276947c9e536086e0` | Server public-deployment security controls | HTTP authorization/defaults | Middleware present; fork diverges with slots enabled by default | Backport policy/default portion: slots off, protected-by-default. |
| [VERIFIED] | 2025-02-06 | `1d20e53c40c3cc848ba2b95f5bf7c075eeec8b19` | COPY_TENSOR destination bounds check | known RCE-class RPC hardening | Measured present | No source backport. |
| [VERIFIED] | 2025-04 | `43ddab6eeeaab5a04fe5a364af0bafb0e4d35065` | Broader RPC malformed-input and failure handling | RPC validation/DoS hardening | Measured present | Keep; review integer sums in local static analysis. |
| [VERIFIED] | 2025-04-22 | `ab47dec3d37aa1927c2ec590e166b76141374ed3` | Explicit untrusted-network prohibition in SECURITY.md | security policy | Policy is upstream documentation, not enforced by fork build | Adopt as deployment policy and CI gate. |
| [VERIFIED] | 2025-05-01 | `8efbdadc616fa717c369059b9b388160958d886c` | Zero RPC tensor name/padding before serialization | information-leak hygiene | Included by ancestry/semantics | No source backport. |
| [VERIFIED] | 2025-07-25 | `64bf1c3744053cf7def10aeed21ff48883ee755b` | RPC null-buffer checks for get/set/copy | RPC validation | Measured present | No source backport. |
| [VERIFIED] | 2025-10-04 | `f39283960b58a92ecc0c72567711318b20e22b55` | COPY_TENSOR source-buffer validation | RPC validation | Measured present | No source backport. |
| [VERIFIED] | 2025-12-23 | `12ee1763a6f6130ce820a366d220bbadff54b818` | RPC buffer-type validation | RPC validation | Included | No source backport. |
| [VERIFIED] | 2025-12-28 | `60f17f56da78d8fabc658279fd25e109700122da` | Invalid endpoint crash handling | RPC robustness | Included | No source backport. |
| [VERIFIED] | 2026-01-11 | `28068af78996485eaa5c34557eae56dc1f996f92` | Narrow and clarify supported security scope | security policy | External policy evidence | Mirror scope in HaloFPX release documentation. |
| [VERIFIED] | 2026-01-11 | `4b060bf240daaeb4fc83386a628b9dfedeb33342` | Clarify server and DoS exclusions | security policy | External policy evidence | Do not infer network hardening from advisory status. |
| [VERIFIED] | 2026-03-21 | `2bcdddd5e3ade6b1e8c9437a652f9fbcf2ad2512` | Reject deprecated zero-block-size tensor types | RPC divide-by-zero/security hardening | Measured present | No source backport. |
| [VERIFIED] | 2026-03-23 | `39bf0d3c6a95803e0f41aaba069ffbee26721042` | Emergency GRAPH_COMPUTE null-buffer rejection | RPC RCE mitigation | Superseded by corrected behavior | Do not cherry-pick alone. |
| [VERIFIED] | 2026-03-27 | `ba38f3becce7d1283585c73d796eb47d72bbbd30` | Correct CPU-buffer data-pointer serialization/validation | RPC RCE mitigation | Measured semantic equivalent | Required minimum if maintaining RPC. |
| [VERIFIED] | 2026-04-23 | `c78fb909b23758f5e418cf98a69bc8a0ef142fb8` | Clamp negative n_discard | HTTP memory-safety fix | Measured present | No source backport. |

[OPEN] The sweep is bounded by public commit metadata, repository advisories, and the reviewed paths. It is not a formal whole-program proof that no security-relevant change was omitted.

[RECOMMENDATION] Re-run this delta review from the frozen upstream comparison commit to the next release candidate; do not rely on a static CVE list.
