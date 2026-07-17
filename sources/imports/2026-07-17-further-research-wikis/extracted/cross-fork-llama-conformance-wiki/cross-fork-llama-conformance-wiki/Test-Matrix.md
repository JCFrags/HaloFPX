# Test matrix

The authoritative records are:

- [`matrix/test-matrix.json`](matrix/test-matrix.json)
- [`matrix/test-matrix.yaml`](matrix/test-matrix.yaml)
- [`matrix/test-matrix.csv`](matrix/test-matrix.csv)
- [`matrix/coverage-summary.json`](matrix/coverage-summary.json)

## Applicability legend

The compact `U/R/C/I` column represents upstream, ROCmFPX, CachyLLama, and integration. Use the machine-readable matrix for the full state; first letters are only a visual index.

| Matrix state | Meaning |
|---|---|
| `required` | unconditional gate |
| `required-if-supported` | gate when the capability probe says supported |
| `required-if-feature` | gate when that feature is included |
| `observe` | collect evidence but do not gate |
| `optional` | may run, with an explicit result |
| `not-applicable` | intentionally absent, backed by capability evidence |

## Coverage summary

| Area | Cases |
|---|---|
| Cache rejection | 12 |
| Cache save/restore | 12 |
| Cancellation | 10 |
| Chat templates | 9 |
| Deterministic runs | 10 |
| Expected error behavior | 16 |
| GGUF parsing | 14 |
| Logits | 8 |
| Long context | 10 |
| MTP/speculative decoding | 12 |
| Quantized kernels | 12 |
| RPC | 12 |
| Sampling | 8 |
| Server APIs | 20 |
| Tokenizer | 10 |

> [!NOTE]
> A case title is not a complete test. Inputs, oracle type, comparison mode, upstream reuse, fixtures, CI tier, backends, applicability, expected behavior, prerequisites, and failure-injection status live in the JSON/YAML records.

## Cache rejection (12)

| ID | Case | Layer | Oracle | Comparison | CI tier | U/R/C/I | Fault |
|---|---|---|---|---|---|---|---|
| `CREJ-001` | Different model bytes | server | error-contract | rejection-reason-class | nightly-cache | R/R/R/R | yes |
| `CREJ-002` | Different quantization/layout | integration | error-contract | rejection-reason-class | nightly-cache | R/R/R/R | yes |
| `CREJ-003` | Different context configuration | library | error-contract | case-by-case | nightly-cache | R/R/R/R | yes |
| `CREJ-004` | Corrupt header or checksum | integration | safety-invariant | bounded-rejection | pr-sanitizer | R/R/R/R | yes |
| `CREJ-005` | Truncated checkpoint | integration | safety-invariant | bounded-rejection | pr-sanitizer | R/R/R/R | yes |
| `CREJ-006` | Oversized declared lengths | integration | safety-invariant | bounded-rejection | pr-sanitizer | R/R/R/R | yes |
| `CREJ-007` | Wrong conversation recurrent state | server | semantic-safety | rejection-or-safe-prefix | nightly-cache | N/N/R/R | yes |
| `CREJ-008` | User namespace isolation | server | authorization-invariant | deny | security-nightly | N/N/R/R | yes |
| `CREJ-009` | Expired system-prompt cache | server | policy-invariant | counter-relation | nightly-cache | N/N/R/R | yes |
| `CREJ-010` | Unavailable and read-only cache path | server | error-contract | declared-degradation | nightly-failure | R/R/R/R | yes |
| `CREJ-011` | Interrupted write recovery | integration | filesystem-invariant | recover-or-ignore | weekly-failure | N/N/R/R | yes |
| `CREJ-012` | Unknown future cache version | integration | error-contract | version-rejection | nightly-cache | R/R/R/R | yes |

## Cache save/restore (12)

| ID | Case | Layer | Oracle | Comparison | CI tier | U/R/C/I | Fault |
|---|---|---|---|---|---|---|---|
| `CACHE-001` | Library state file round trip | library | native-test | native-pass-fail | pr-model | R/R/R/R | no |
| `CACHE-002` | Sequence state host copy | library | native-test | native-pass-fail | pr-model | R/R/R/R | no |
| `CACHE-003` | Sequence state device copy | library | native-test | native-pass-fail | gpu-nightly | R/R/R/R | no |
| `CACHE-004` | Sequence removal isolation | library | exact-bytes | exact | pr-model | R/R/R/R | no |
| `CACHE-005` | Server slot save and restore | server | behavioral-contract | exact-and-structural | pr-server | R/R/R/R | no |
| `CACHE-006` | Server slot erase | server | behavioral-contract | counter-relation | pr-server | R/R/R/R | no |
| `CACHE-007` | Persistent SSD restart restore | server | exact-token-sequence | exact | nightly-cache | N/N/R/R | no |
| `CACHE-008` | Hot warm cold tier transitions | integration | exact-bytes | exact-and-invariants | nightly-cache | N/N/R/R | no |
| `CACHE-009` | System-prompt cache cross-conversation reuse | server | exact-token-sequence | exact-and-counter-relation | nightly-cache | O/O/R/R | no |
| `CACHE-010` | Concurrent restore isolation | server | exact-token-sequence | per-conversation-exact | nightly-cache | O/O/R/R | no |
| `CACHE-011` | Atomic checkpoint publication | integration | filesystem-invariant | atomicity | nightly-cache | N/N/R/R | no |
| `CACHE-012` | Cache telemetry truthfulness | server | counter-invariant | exact-relations | nightly-cache | R/R/R/R | no |

## Cancellation (10)

| ID | Case | Layer | Oracle | Comparison | CI tier | U/R/C/I | Fault |
|---|---|---|---|---|---|---|---|
| `CANCEL-001` | Model-load progress callback | library | native-test | native-pass-fail | pr-model | R/R/R/R | no |
| `CANCEL-002` | Client disconnect during streaming decode | server | liveness-and-resource | bounded-cancellation | nightly-cancellation | R/R/R/R | yes |
| `CANCEL-003` | Cancellation during prompt prefill | server | liveness-and-isolation | bounded-cancellation | nightly-cancellation | R/R/R/R | yes |
| `CANCEL-004` | Cancellation during cache restore | server | liveness-and-filesystem | bounded-cancellation | nightly-cancellation | R/R/R/R | yes |
| `CANCEL-005` | Cancellation during cache save | server | filesystem-invariant | atomicity | nightly-cancellation | R/R/R/R | yes |
| `CANCEL-006` | One request cancelled among parallel slots | server | isolation | per-request-exact | nightly-cancellation | R/R/R/R | no |
| `CANCEL-007` | Repeated cancel/reuse stress | server | liveness-and-leak | bounded-resources | weekly-stress | R/R/R/R | yes |
| `CANCEL-008` | Graceful shutdown with active requests | integration | process-contract | bounded-shutdown | nightly-cancellation | R/R/R/R | yes |
| `CANCEL-009` | Cancellation error envelope | server | error-contract | endpoint-specific | nightly-cancellation | R/R/R/R | no |
| `CANCEL-010` | Accelerator fault containment | backend | liveness-and-safety | bounded-failure | hardware-lab | R/R/R/R | yes |

## Chat templates (9)

| ID | Case | Layer | Oracle | Comparison | CI tier | U/R/C/I | Fault |
|---|---|---|---|---|---|---|---|
| `CHAT-001` | Automated upstream template suite | library | native-test | native-pass-fail | pr-cpu | R/R/R/R | no |
| `CHAT-002` | Common versus direct Jinja engine | library | exact-text | normalized-newline-exact | pr-cpu | R/R/R/R | no |
| `CHAT-003` | Tool-call rendering | library | exact-text | exact | pr-cpu | R/R/R/R | no |
| `CHAT-004` | Generation-prompt switch | library | exact-text | exact | pr-cpu | R/R/R/R | no |
| `CHAT-005` | Empty, null, and multimodal content forms | server | schema-contract | case-by-case | pr-server | R/R/R/R | no |
| `CHAT-006` | Template override precedence | server | exact-text | exact | pr-server | R/R/R/R | no |
| `CHAT-007` | Malformed Jinja and missing variables | library | error-contract | error-class-and-location | pr-cpu | R/R/R/R | yes |
| `CHAT-008` | Cross-endpoint prompt parity | server | exact-token-sequence | exact-after-adapter-normalization | nightly-server | R/R/R/R | no |
| `CHAT-009` | Template output size and recursion limits | library | safety-invariant | bounded-failure | pr-sanitizer | R/R/R/R | yes |

## Deterministic runs (10)

| ID | Case | Layer | Oracle | Comparison | CI tier | U/R/C/I | Fault |
|---|---|---|---|---|---|---|---|
| `DET-001` | Greedy CLI repeatability | cli | exact-token-sequence | exact | pr-model | R/R/R/R | no |
| `DET-002` | Fixed-seed server repeatability | server | exact-output | exact | pr-server | R/R/R/R | no |
| `DET-003` | Stream versus non-stream | server | exact-output | exact-after-stream-assembly | pr-server | R/R/R/R | no |
| `DET-004` | Batch-size invariance | integration | exact-token-sequence | exact-or-approved-exception | nightly-model | R/R/R/R | no |
| `DET-005` | Thread-count invariance | cli | exact-token-sequence | exact-or-approved-exception | nightly-model | R/R/R/R | no |
| `DET-006` | Cross-fork upstream-compatible run | integration | exact-token-sequence | exact | nightly-cross-fork | R/R/R/R | no |
| `DET-007` | Backend reproducibility within backend | backend | exact-token-sequence | exact-within-lane | gpu-nightly | R/R/R/R | no |
| `DET-008` | Cache cold/warm deterministic parity | server | exact-token-sequence | exact | nightly-cache | R/R/R/R | no |
| `DET-009` | Speculative versus target-only parity | server | exact-token-sequence | exact | nightly-speculative | R/R/R/R | no |
| `DET-010` | Restart repeatability | integration | exact-token-sequence | exact | nightly-model | R/R/R/R | no |

## Expected error behavior (16)

| ID | Case | Layer | Oracle | Comparison | CI tier | U/R/C/I | Fault |
|---|---|---|---|---|---|---|---|
| `ERR-001` | Missing model argument or file | cli | process-contract | exit-and-error-class | pr-cpu | R/R/R/R | no |
| `ERR-002` | Unsupported device selection | cli | process-contract | exit-and-error-class | pr-cpu | R/R/R/R | yes |
| `ERR-003` | Unsupported quantized tensor type | library | error-contract | explicit-unsupported | nightly-model | R/R/R/R | yes |
| `ERR-004` | Invalid command-line values | cli | process-contract | exit-and-diagnostic | pr-cpu | R/R/R/R | yes |
| `ERR-005` | Malformed endpoint path and HTTP method | server | http-contract | status-and-error-shape | pr-server | R/R/R/R | yes |
| `ERR-006` | Request validation matrix | server | schema-contract | status-and-field-path | pr-server | R/R/R/R | yes |
| `ERR-007` | Capacity and queue saturation | server | resource-contract | status-and-retry-semantics | nightly-server | R/R/R/R | yes |
| `ERR-008` | Host memory allocation failure | integration | safety-invariant | bounded-failure | weekly-failure | R/R/R/R | yes |
| `ERR-009` | Accelerator memory exhaustion | backend | resource-contract | bounded-failure-or-declared-fallback | hardware-lab | R/R/R/R | yes |
| `ERR-010` | Disk full and I/O errors | integration | filesystem-contract | declared-degradation | weekly-failure | R/R/R/R | yes |
| `ERR-011` | Signal and exit-code contract | integration | process-contract | exit-and-cleanup | nightly-cancellation | R/R/R/R | no |
| `ERR-012` | Error envelope normalization | server | compatibility-contract | normalized-error-object | nightly-server | R/R/R/R | no |
| `ERR-013` | Log redaction and bounded diagnostics | integration | security-contract | redaction-and-size-limit | security-nightly | R/R/R/R | yes |
| `ERR-014` | Sanitizer-clean failure corpus | integration | safety-invariant | sanitizer-clean | pr-sanitizer | R/R/R/R | yes |
| `ERR-015` | Unsupported feature declaration | integration | capability-contract | explicit-not-supported | nightly-cross-fork | R/R/R/R | no |
| `ERR-016` | Watchdog and hang detection | integration | harness-invariant | bounded-runtime | all | R/R/R/R | no |

## GGUF parsing (14)

| ID | Case | Layer | Oracle | Comparison | CI tier | U/R/C/I | Fault |
|---|---|---|---|---|---|---|---|
| `GGUF-001` | Known-good single-file load | library | exact-structure | exact-normalized | pr-cpu | R/R/R/R | no |
| `GGUF-002` | Split-model discovery and load | library | exact-structure | exact-normalized | nightly-model | R/R/R/R | no |
| `GGUF-003` | Bad magic rejection | library | error-contract | exact-error-class | pr-cpu | R/R/R/R | yes |
| `GGUF-004` | Unsupported version rejection | library | error-contract | set-membership | pr-cpu | R/R/R/R | yes |
| `GGUF-005` | Impossible counts rejection | library | safety-invariant | exact-error-class | pr-sanitizer | R/R/R/R | yes |
| `GGUF-006` | Invalid KV entries | library | error-contract | case-by-case | pr-cpu | R/R/R/R | yes |
| `GGUF-007` | Invalid tensor descriptors | library | safety-invariant | case-by-case | pr-sanitizer | R/R/R/R | yes |
| `GGUF-008` | Truncated tensor data | library | error-contract | exact-error-class | pr-cpu | R/R/R/R | yes |
| `GGUF-009` | Custom alignment round trip | library | metamorphic | round-trip | pr-cpu | R/R/R/R | no |
| `GGUF-010` | Special-token ID bounds | library | safety-invariant | error-or-safe-fallback | pr-sanitizer | R/R/R/R | yes |
| `GGUF-011` | Unknown optional metadata preservation | library | metamorphic | invariant | nightly-model | R/R/R/R | no |
| `GGUF-012` | ROCmFPX type dispatch | library | exact-structure | exact-normalized | rocm-gpu | N/R/N/R | no |
| `GGUF-013` | Model identity fingerprint | integration | exact-structure | exact-fields | nightly-model | O/O/R/R | no |
| `GGUF-014` | MTP/NextN metadata detection | library | exact-structure | exact-boolean | nightly-model | R/R/R/R | no |

## Logits (8)

| ID | Case | Layer | Oracle | Comparison | CI tier | U/R/C/I | Fault |
|---|---|---|---|---|---|---|---|
| `LOGIT-001` | CPU reference next-token logits | library | numeric-reference | calibrated-abs-rel | reference-promotion | R/R/R/R | no |
| `LOGIT-002` | Cross-fork CPU logits | library | numeric-reference | calibrated-abs-rel-and-rank | nightly-model | R/R/R/R | no |
| `LOGIT-003` | Backend logits parity | backend | numeric-reference | calibrated-abs-rel-and-rank | gpu-nightly | R/R/R/R | no |
| `LOGIT-004` | Batch and microbatch invariance | library | metamorphic-numeric | calibrated-abs-rel-and-rank | nightly-model | R/R/R/R | no |
| `LOGIT-005` | Cache versus replay logits | library | numeric-reference | calibrated-abs-rel-and-rank | nightly-model | R/R/R/R | no |
| `LOGIT-006` | Long-position logits checkpoints | library | numeric-reference | position-indexed-calibrated | long-context-nightly | R/R/R/R | no |
| `LOGIT-007` | NaN and infinity rejection | backend | safety-invariant | finite-only | gpu-nightly | R/R/R/R | no |
| `LOGIT-008` | Embedding API parity | server | numeric-reference | calibrated-vector-metrics | nightly-server | R/R/R/R | no |

## Long context (10)

| ID | Case | Layer | Oracle | Comparison | CI tier | U/R/C/I | Fault |
|---|---|---|---|---|---|---|---|
| `CTX-001` | Exact context boundary | integration | behavioral-contract | status-and-token-count | long-context-nightly | R/R/R/R | no |
| `CTX-002` | One token over boundary | server | error-contract | status-and-error-shape | pr-server | R/R/R/R | yes |
| `CTX-003` | Context shift continuation | server | behavioral-contract | invariants | long-context-nightly | R/R/R/R | no |
| `CTX-004` | No-shift rejection | server | error-contract | status-and-error-shape | pr-server | R/R/R/R | yes |
| `CTX-005` | Rope-scaling configuration isolation | integration | numeric-reference | configuration-scoped | long-context-nightly | R/R/R/R | no |
| `CTX-006` | Passkey retrieval correctness | integration | exact-answer | exact-normalized | long-context-nightly | R/R/R/R | no |
| `CTX-007` | Long-prefix cache equivalence | integration | exact-token-sequence | exact | nightly-cache | R/R/R/R | no |
| `CTX-008` | Fragmented KV restore | library | native-test | native-pass-fail | pr-model | R/R/R/R | no |
| `CTX-009` | Recurrent-state rollback | library | native-test | native-pass-fail | pr-model | R/R/R/R | no |
| `CTX-010` | Cancellation during long prefill | server | liveness-and-isolation | bounded-cancellation | nightly-cancellation | R/R/R/R | yes |

## MTP/speculative decoding (12)

| ID | Case | Layer | Oracle | Comparison | CI tier | U/R/C/I | Fault |
|---|---|---|---|---|---|---|---|
| `SPEC-001` | Draft-model greedy parity | server | exact-token-sequence | exact | nightly-speculative | R/R/R/R | no |
| `SPEC-002` | Draft window parameter matrix | server | exact-token-sequence | exact | nightly-speculative | R/R/R/R | no |
| `SPEC-003` | Embedded MTP greedy parity | server | exact-token-sequence | exact | mtp-gpu | R/R/R/R | no |
| `SPEC-004` | MTP capability rejection | server | error-contract | status-and-error-class | pr-server | R/R/R/R | yes |
| `SPEC-005` | Draft tokenizer/model mismatch | server | error-contract | reject-before-generation | nightly-speculative | R/R/R/R | yes |
| `SPEC-006` | Context boundary and shift | server | behavioral-contract | exact-output-and-capacity | long-context-nightly | R/R/R/R | no |
| `SPEC-007` | Parallel slot isolation | server | exact-output | per-request-exact | nightly-speculative | R/R/R/R | no |
| `SPEC-008` | Acceptance accounting | server | counter-invariant | exact-relations | nightly-speculative | R/R/R/R | no |
| `SPEC-009` | Speculative cache save/restore | integration | exact-token-sequence | exact | nightly-speculative | R/R/R/R | no |
| `SPEC-010` | Cancellation during drafting | server | liveness-and-isolation | bounded-cancellation | nightly-cancellation | R/R/R/R | yes |
| `SPEC-011` | Nondeterministic target distribution | integration | distributional | approved-paired-statistical-profile | statistical-weekly | R/R/R/R | no |
| `SPEC-012` | Performance telemetry without correctness waiver | integration | telemetry | record-only-until-approved | performance-weekly | R/R/R/R | no |

## Quantized kernels (12)

| ID | Case | Layer | Oracle | Comparison | CI tier | U/R/C/I | Fault |
|---|---|---|---|---|---|---|---|
| `KERN-001` | Native backend operation suite | backend | native-test | native-pass-fail | gpu-pr | R/R/R/R | no |
| `KERN-002` | Native quantize/dequantize functions | backend | native-test | native-pass-fail | pr-cpu | R/R/R/R | no |
| `KERN-003` | CPU dequant reference bytes | backend | numeric-reference | calibrated-abs-rel | reference-promotion | R/R/R/R | no |
| `KERN-004` | Accelerator versus CPU reference | backend | numeric-reference | calibrated-abs-rel | gpu-nightly | R/R/R/R | no |
| `KERN-005` | Ragged and boundary shapes | backend | numeric-reference | calibrated-abs-rel | gpu-nightly | R/R/R/R | no |
| `KERN-006` | Tail and non-contiguous tensors | backend | numeric-reference | calibrated-abs-rel | gpu-nightly | R/R/R/R | no |
| `KERN-007` | ROCmFPX format matrix | backend | numeric-reference | calibrated-type-specific | rocm-gpu | N/R/N/R | no |
| `KERN-008` | TurboQuant reference tests | backend | native-test | native-pass-fail | pr-cpu | N/R/N/R | no |
| `KERN-009` | Kernel fallback semantics | backend | control-flow-invariant | declared-path | gpu-nightly | R/R/R/R | yes |
| `KERN-010` | Quantization quality characterization | integration | quality-reference | approved-quality-profile | quality-weekly | R/R/R/R | no |
| `KERN-011` | Non-finite and sentinel checks | backend | safety-invariant | sentinel-and-finite | gpu-nightly | R/R/R/R | no |
| `KERN-012` | Concurrent kernel isolation | backend | numeric-reference | calibrated-abs-rel | gpu-nightly | R/R/R/R | no |

## RPC (12)

| ID | Case | Layer | Oracle | Comparison | CI tier | U/R/C/I | Fault |
|---|---|---|---|---|---|---|---|
| `RPC-001` | Loopback server discovery | rpc | capability-contract | exact-normalized | rpc-nightly | R/R/R/R | no |
| `RPC-002` | Remote operation parity | rpc | numeric-reference | calibrated-abs-rel | rpc-nightly | R/R/R/R | no |
| `RPC-003` | Remote model inference parity | rpc | numeric-and-exact | calibrated-logits-exact-greedy | rpc-nightly | R/R/R/R | no |
| `RPC-004` | Multiple RPC endpoints | rpc | numeric-reference | calibrated-abs-rel | rpc-weekly | R/R/R/R | no |
| `RPC-005` | Tensor cache identity | rpc | cache-invariant | exact-identity-and-output | rpc-nightly | R/R/R/R | no |
| `RPC-006` | Disconnect during transfer | rpc | liveness-and-safety | bounded-failure | rpc-nightly | R/R/R/R | yes |
| `RPC-007` | Protocol or build mismatch | rpc | error-contract | reject-or-capability-negotiate | rpc-nightly | R/R/R/R | yes |
| `RPC-008` | Unsupported remote operation | rpc | error-contract | explicit-unsupported-or-fallback | rpc-nightly | R/R/R/R | yes |
| `RPC-009` | Invalid address and port | rpc | error-contract | bounded-transport-error | pr-cpu | R/R/R/R | yes |
| `RPC-010` | Untrusted-network guardrail | rpc | security-policy | loopback-only | security-nightly | R/R/R/R | no |
| `RPC-011` | RPC cancellation propagation | rpc | liveness-and-accounting | bounded-cancellation | rpc-nightly | R/R/R/R | yes |
| `RPC-012` | RDMA optional lane | rpc | numeric-reference | same-profile-as-rpc | rpc-rdma-weekly | R/R/R/R | no |

## Sampling (8)

| ID | Case | Layer | Oracle | Comparison | CI tier | U/R/C/I | Fault |
|---|---|---|---|---|---|---|---|
| `SAMP-001` | Native sampler primitive tests | library | native-test | native-pass-fail | pr-cpu | R/R/R/R | no |
| `SAMP-002` | Greedy argmax | library | exact-token | exact | pr-cpu | R/R/R/R | no |
| `SAMP-003` | Seeded sampler replay | library | exact-token-sequence | exact | pr-cpu | R/R/R/R | no |
| `SAMP-004` | Sampler order semantics | library | exact-structure | exact | pr-cpu | R/R/R/R | no |
| `SAMP-005` | Grammar-constrained sampling | library | language-membership | validator | pr-cpu | R/R/R/R | no |
| `SAMP-006` | Nondeterministic distribution comparison | integration | distributional | approved-statistical-profile | statistical-nightly | R/R/R/R | no |
| `SAMP-007` | Invalid sampler parameters | server | error-contract | status-and-error-shape | pr-server | R/R/R/R | yes |
| `SAMP-008` | Sampler state across cache restore | integration | exact-token-sequence | case-by-case | nightly-model | R/R/R/R | no |

## Server APIs (20)

| ID | Case | Layer | Oracle | Comparison | CI tier | U/R/C/I | Fault |
|---|---|---|---|---|---|---|---|
| `API-001` | Health readiness and liveness | server | http-contract | status-and-schema | pr-server | R/R/R/R | no |
| `API-002` | Properties and model inventory | server | schema-contract | normalized-json | pr-server | R/R/R/R | no |
| `API-003` | Native completion | server | schema-and-output | exact-for-deterministic | pr-server | R/R/R/R | no |
| `API-004` | Native completion streaming | server | stream-contract | exact-after-stream-assembly | pr-server | R/R/R/R | no |
| `API-005` | OpenAI legacy completions | server | compatibility-contract | normalized-json | pr-server | R/R/R/R | no |
| `API-006` | OpenAI chat completions | server | compatibility-contract | normalized-json-and-exact-output | pr-server | R/R/R/R | no |
| `API-007` | OpenAI Responses API | server | compatibility-contract | normalized-json-and-event-sequence | nightly-server | R/R/R/R | no |
| `API-008` | Anthropic Messages adapter | server | compatibility-contract | normalized-json-and-event-sequence | nightly-server | R/R/R/R | no |
| `API-009` | Embeddings endpoints | server | schema-and-numeric | exact-shape-calibrated-values | nightly-server | R/R/R/R | no |
| `API-010` | Rerank endpoint | server | schema-and-order | exact-order-calibrated-scores | nightly-server | R/R/R/R | no |
| `API-011` | Tokenize and detokenize endpoints | server | exact-token-sequence | exact | pr-server | R/R/R/R | no |
| `API-012` | Slots administration | server | http-contract | status-and-schema | pr-server | R/R/R/R | no |
| `API-013` | Structured output and JSON schema | server | language-membership | schema-validator | nightly-server | R/R/R/R | no |
| `API-014` | Tool-call contract | server | schema-contract | normalized-json | nightly-server | R/R/R/R | no |
| `API-015` | Metrics and usage accounting | server | counter-invariant | exact-relations | nightly-server | R/R/R/R | no |
| `API-016` | User concurrency cap | server | authorization-and-rate-contract | status-and-error-shape | nightly-cache | N/N/R/R | no |
| `API-017` | Expert tracking API | server | schema-and-noninterference | exact-relations | nightly-fork | N/N/R/R | no |
| `API-018` | Unknown fields and compatibility policy | server | error-contract | case-by-case | pr-server | R/R/R/R | yes |
| `API-019` | Malformed JSON and content type | server | safety-invariant | bounded-http-error | pr-sanitizer | R/R/R/R | yes |
| `API-020` | CORS and authentication surface | server | security-contract | header-and-redaction | security-nightly | R/R/R/R | no |

## Tokenizer (10)

| ID | Case | Layer | Oracle | Comparison | CI tier | U/R/C/I | Fault |
|---|---|---|---|---|---|---|---|
| `TOK-001` | Pinned vocabulary vectors | library | exact-token-sequence | exact | pr-cpu | R/R/R/R | no |
| `TOK-002` | Detokenization vectors | library | exact-bytes | exact | pr-cpu | R/R/R/R | no |
| `TOK-003` | Unicode and normalization corpus | library | exact-token-sequence | exact | pr-cpu | R/R/R/R | no |
| `TOK-004` | Whitespace and control bytes | library | exact-token-sequence | exact | pr-cpu | R/R/R/R | no |
| `TOK-005` | BOS/EOS and special-token switches | library | exact-token-sequence | exact | pr-cpu | R/R/R/R | no |
| `TOK-006` | Round-trip property | library | metamorphic | round-trip | nightly-fuzz | R/R/R/R | no |
| `TOK-007` | Thread-safety and repeatability | library | exact-token-sequence | exact | pr-cpu | R/R/R/R | no |
| `TOK-008` | Server tokenize/detokenize parity | server | exact-token-sequence | exact | pr-server | R/R/R/R | no |
| `TOK-009` | Prompt string versus token-array generation | server | exact-output | exact | pr-server | R/R/R/R | no |
| `TOK-010` | Malformed token input | server | error-contract | status-and-error-shape | pr-server | R/R/R/R | yes |
