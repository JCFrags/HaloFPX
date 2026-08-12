# OPEN-PIN-01 small-model runtime smoke results

Status: **PARTIAL PASS — LOAD/REQUEST/TEARDOWN; QUALITY OPEN**

## Inputs and execution

- **[MEASURED]** Model on both nodes: Qwen3-4B-Q8_0, 4,280,404,704 bytes, SHA-256 `8c2f07f26af9747e41988551106f149b03eb9b5cb6df636027b6bf6278473300`.
- **[MEASURED]** Revisions: control `a5605a72768c6562241b248e268e33dc92787394`; candidate `61f2f2d7bc4955e9bca821095ef69125837133b5`.
- **[MEASURED]** Each revision ran with F16 and Turbo4 K/V cache on nimo-1 and nimo-2: eight total runs, explicit `ROCm0`, flash attention on, 4096-token context, one slot, seed 1234, temperature 0, loopback port 18081.
- **[MEASURED]** All eight servers loaded, returned `/health`, `/props`, `/slots`, and `/completion`, then exited cleanly with no residual listener or process. The deployed services stayed inactive/enabled.

## Reproducibility and output

| Cache | Control vs candidate | nimo-1 vs nimo-2 | Observed decode rate |
|---|---|---|---|
| F16 | **[MEASURED]** content SHA identical: `2b665a310072357ed5e3504d63213b628edb8868939321261aaf9d779ba97560` | **[MEASURED]** identical | 47.86–47.95 tok/s |
| Turbo4 | **[MEASURED]** content SHA identical: `9b9ea063c48080f424a694bd3d8b626a12d361e3e94c3fe4df764d8b6e108191` | **[MEASURED]** identical | 45.50–45.78 tok/s |

**[MEASURED]** F16 and Turbo4 produced different token sequences under the same model, prompt, seed, and sampling controls. The control and candidate agree within each cache mode, so this smoke attributes the divergence to the cache-mode choice rather than the candidate commit. The prompt was intentionally tiny and the response was truncated at 32 tokens; this is a diagnostic observation, not a quality verdict or a representative performance benchmark.

**[MEASURED]** Turbo4 logs on both commits and nodes report: `attention rotation disabled for fp3 ROCmFPX/TurboQuant KV cache (K=turbo4, V=turbo4)`. This warning is deterministic and must be included in subsequent model/position-encoding quality design.

## Resource and safety observations

- **[MEASURED]** Maximum sampled GPU edge temperature was 47 C, well below the 95 C stop rule.
- **[MEASURED]** Swap did not grow during any run and `MemAvailable` remained above 122 GB after each request.
- **[MEASURED]** The runtime reported one AMD Radeon 8060S device as `ROCm0`; Vulkan was enumerated but not selected.
- **[MEASURED]** The server also enabled its existing 8192 MiB RAM prompt cache. This experiment did not exercise SSD persistence, restore, eviction, corruption handling, or cross-process reuse.

## Decision boundary

**[OPEN]** Turbo4 is not admitted. Required follow-up includes matched long-context perplexity/quality, RoPE/attention-rotation analysis, multiple prompts and seeds, memory-at-filled-context measurement, and comparison with F16/Q8 cache modes. Model, MTP, RPC, state, SSD, Vulkan, stress, and large-model gates remain open. The observed single-request rates must not be used as a performance selection.
