# Upstream test reuse

## Strategy

Run the pinned upstream test implementations in every fork before adding cross-fork wrappers. Reuse has three forms:

1. **Unchanged native test:** build and run the same CTest target or server pytest.
2. **Parameterized native test:** supply the same model/fixture and backend lane.
3. **Differential wrapper:** retain the native test, then capture comparable outputs across forks.

Do not copy assertions into a new harness merely to rename them. Copying creates drift and often loses upstream setup, sanitizer, fixture, or platform behavior.

## High-value inventory

| Area | Upstream path | Type | Reuse | Coverage | Added suite gap |
|---|---|---|---|---|---|
| GGUF | `tests/test-gguf.cpp` | C++ native | run unchanged; retain handcrafted generator | bad magic/version/counts, KV and tensor descriptors, alignment, truncation, overflow | cross-fork normalized diagnostics and fork-specific tensor types |
| Tokenizer | `tests/test-tokenizer-0.cpp` | C++ native | run unchanged for every pinned vocab fixture | exact vectors, detokenization, multithread repeatability | cross-fork API parity and added Unicode corpus |
| Tokenizer | `tests/test-tokenizer-1-bpe.cpp` | C++ native | run supported cases | BPE behavior | promote exact corpus per vocabulary digest |
| Tokenizer | `tests/test-tokenizer-1-spm.cpp` | C++ native | run supported cases | SentencePiece behavior | promote exact corpus per vocabulary digest |
| Tokenizer | `tests/test-tokenizer-random.py` | Python | nightly/property lane | random tokenization differential coverage | seed locking and artifact capture |
| Tokenizer | `tests/test-tokenizers-repo.sh` | shell | scheduled upstream vocab sweep | repository vocab fixtures | network isolation and mirror pinning |
| Chat | `tests/test-chat-template.cpp` | C++ native | run automated suite and custom fixture directory | Jinja rendering, tools, common/direct engine | adapter prompt parity |
| Chat | `tests/test-jinja.cpp` | C++ native | run unchanged | Jinja parser/runtime | resource-limit failure cases |
| Chat | `tests/test-chat.cpp` | C++ native | run unchanged | server/common chat behavior | cross-endpoint normalized input tokens |
| Sampling | `tests/test-sampling.cpp` | C++ native | run unchanged | sampler primitives and queues | cross-fork seeded and distributional runs |
| Sampling | `tests/test-backend-sampler.cpp` | C++ native | run with tiny pinned model | sampler integration and multi-sequence behavior | fork/backend matrix |
| Grammar | `tests/test-grammar-integration.cpp` | C++ native | run unchanged | grammar-constrained decode | server adapter schemas |
| Grammar | `tests/test-json-schema-to-grammar.cpp` | C++ native | run unchanged | JSON schema conversion | endpoint validation and event streams |
| Backends | `tests/test-backend-ops.cpp` | C++ native | run on every compiled backend | operation correctness and shape matrix | ROCmFPX types, lane-scoped references |
| Quantization | `tests/test-quantize-fns.cpp` | C++ native | run unchanged plus fork types | quantize/dequantize functions | approved cross-backend numeric envelopes |
| Quantization | `tests/test-quantize-stats.cpp` | C++ utility | quality characterization only | quant statistics | independent quality-profile approval |
| Quantization | `tests/test-quantize-perf.cpp` | C++ utility | telemetry lane | kernel performance | hardware-specific baseline policy |
| State | `tests/test-save-load-state.cpp` | C++ native | run unchanged | state file, host/device sequence copy, sequence isolation | cross-fork state rejection and persistent cache |
| State | `tests/test-state-restore-fragmented.cpp` | C++ native | run unchanged | fragmented KV restoration | long-context and fork persistence |
| State | `tests/test-recurrent-state-rollback.cpp` | C++ native | run with generated model | recurrent rollback | hybrid cross-conversation cache safety |
| Cancellation | `tests/test-model-load-cancel.cpp` | C++ native | run unchanged | progress-callback load cancellation | HTTP, cache I/O, RPC, speculative cancellation |
| Concurrency | `tests/test-thread-safety.cpp` | C++ native | run unchanged | parallel generation/thread safety | cross-fork and cache isolation |
| Server core | `tools/server/tests/unit/test_basic.py` | pytest | run unchanged | health, props, models, slots, split model, UI | fork capabilities and normalized schemas |
| Server completion | `tools/server/tests/unit/test_completion.py` | pytest | run unchanged | stream/nonstream, SDK, seeds, batching, token input, errors | cross-fork references and cancellation |
| Server chat | `tools/server/tests/unit/test_chat_completion.py` | pytest | run unchanged | OpenAI chat endpoint | input-token capture and cross-adapter parity |
| Server state | `tools/server/tests/unit/test_slot_save.py` | pytest | run unchanged | slot save/restore/erase | corruption, mismatch, persistent SSD tiers |
| Speculative | `tools/server/tests/unit/test_speculative.py` | pytest | run unchanged | draft parity, parameter sweep, context shift, parallel slots | embedded MTP, cancellation, distributional lane |
| Responses | `tools/server/tests/unit/test_responses.py` | pytest | run supported subset | OpenAI Responses compatibility | cross-fork schema normalization |
| Anthropic | `tools/server/tests/unit/test_anthropic.py` | pytest | run supported subset | Anthropic Messages compatibility | cross-fork schema normalization |
| Embeddings | `tools/server/tests/unit/test_embedding.py` | pytest | run unchanged | embedding endpoint | approved vector tolerances |
| Rerank | `tools/server/tests/unit/test_rerank.py` | pytest | run unchanged | rerank endpoint | approved score tolerances |
| ROCmFPX | `tests/test-turboquant.cpp` | C++ native | run on ROCmFPX and integration fork | TurboQuant FWHT, MSE, bitpack reference behavior | accelerated backend comparisons |
| CachyLLama | `test_kv_page_manager.cpp` | C++ native | adopt after build integration review | page allocation, hot/warm/cold, reload, flush, stats | server-level persistence, rejection, user isolation |

The CSV form is at [`references/upstream-test-map.csv`](references/upstream-test-map.csv).

## Execution recommendations

### CTest

Build with tests enabled and preserve `ctest --show-only=json-v1` as an artifact. Select native labels/targets rather than maintaining a separate hardcoded list when possible. Record tests that are absent from a fork as capability drift, not as passing.

### Server pytest

Use the fork's own `tools/server/tests` utilities for process startup and endpoint behavior, but pin Python dependencies and model downloads. Add an outer collector that records raw request/response data and normalized observations.

### Fork-specific reuse

ROCmFPX retains much of the upstream tree and adds TurboQuant coverage. The provisional CachyLLama candidate has a page-manager test outside the standard upstream test directory; integrate it into the fork's build/CTest flow before treating it as a stable gate.

### Drift review

On every upstream rebase, diff:

- `tests/CMakeLists.txt`;
- `tests/`;
- `tools/server/tests/`;
- `examples/speculative/`;
- `examples/rpc/`;
- server API documentation;
- GGUF, state, tokenizer, sampling, and backend public headers.

New upstream regressions should enter the native lane immediately and be mapped into this matrix only when a cross-fork or feature-specific contract is needed.
