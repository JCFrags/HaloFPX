# Research sources

**Research date:** 2026-07-17. Pins are a snapshot; verify again before execution.

## Source repositories

- Upstream: `https://github.com/ggml-org/llama.cpp` at `86d86ed4396b4130922f7b9af26e3d9fc11a591b`
- ROCmFPX: `https://github.com/charlie12345/ROCmFPX` at `a5605a72768c6562241b248e268e33dc92787394`
- Requested CachyLLama path: `https://github.com/llama-ai/CachyLlama` — not resolved during research
- Provisional candidate: `https://github.com/fewtarius/CachyLLama` at `6be745998f568e379ea197fcf827baec73ff9940`

The machine-readable inventory is `references/source-inventory.yaml`.

## Upstream test sources reviewed

- `tests/CMakeLists.txt`
- `tests/test-gguf.cpp`
- `tests/test-tokenizer-0.cpp`
- `tests/test-tokenizer-1-bpe.cpp`
- `tests/test-tokenizer-1-spm.cpp`
- `tests/test-tokenizer-random.py`
- `tests/test-chat-template.cpp`
- `tests/test-jinja.cpp`
- `tests/test-sampling.cpp`
- `tests/test-backend-sampler.cpp`
- `tests/test-backend-ops.cpp`
- `tests/test-quantize-fns.cpp`
- `tests/test-quantize-stats.cpp`
- `tests/test-save-load-state.cpp`
- `tests/test-state-restore-fragmented.cpp`
- `tests/test-recurrent-state-rollback.cpp`
- `tests/test-model-load-cancel.cpp`
- `tests/test-thread-safety.cpp`
- `tools/server/tests/unit/test_basic.py`
- `tools/server/tests/unit/test_completion.py`
- `tools/server/tests/unit/test_slot_save.py`
- `tools/server/tests/unit/test_speculative.py`
- server chat, Responses, Anthropic, embedding, rerank, schema, tool-call, metrics, tokenize, context-shift, and security tests.

## Documentation and security sources reviewed

- `tools/server/README.md`
- `examples/speculative/README.md`
- `examples/rpc/README.md`
- `examples/quantize/README.md`
- upstream `SECURITY.md`
- upstream GitHub Security Advisory `GHSA-5vm9-p64x-gqw9`

## Fork-specific sources reviewed

ROCmFPX:

- `README.md`
- `tests/CMakeLists.txt`
- `tests/test-turboquant.cpp`
- `tools/server/tests/unit/test_speculative.py`

Provisional CachyLLama:

- `README.md`
- server/cache source tree
- `test_kv_page_manager.cpp`

## Research limits

The suite does not claim that every current branch-head feature was exhaustively enumerated. It records the paths and capabilities needed for this design. Before a release run, repeat the inventory against the exact selected commits and regenerate the upstream drift report.
