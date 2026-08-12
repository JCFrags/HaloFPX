# Candidate RPC smoke results

Status: **PARTIAL PASS — REACHABILITY/OFFLOAD/REQUEST/TEARDOWN**

## Build and identity

- **[MEASURED]** Candidate commit/tree: `61f2f2d7bc4955e9bca821095ef69125837133b5` / `0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd`.
- **[MEASURED]** A separate `GGML_RPC=ON` build completed on nimo-1 and nimo-2. Cross-host hashes match: `rpc-server` `3327b1d7165d6084aeff8694163b1a69a1110dd7538a89389794bba9d78d5868`; RPC-enabled `llama-server` `d752b7f327b51d50f3a868fda537ffec90f999ae2144ad61416203ccc12d4b4c`.
- **[MEASURED]** The original non-RPC qualification build and its recorded hashes were not overwritten.

## Distributed smoke

- **[MEASURED]** nimo-1 worker bound only `10.44.0.1:50053` on USB4 rail A. nimo-2 coordinator bound only `127.0.0.1:18081`.
- **[MEASURED]** Coordinator argv explicitly ordered `--device RPC0,ROCm0 --tensor-split 1,1 --split-mode layer` and used model SHA-256 `8c2f07f26af9747e41988551106f149b03eb9b5cb6df636027b6bf6278473300`.
- **[MEASURED]** Coordinator enumerated `RPC0` as `10.44.0.1:50053` with 126,976 MiB and local `ROCm0` with 126,976 MiB. The remote worker's available-memory reports fell by about 2.4 GiB during model/graph setup and its ROCm graph warmup completed, proving remote allocation and graph execution rather than mere TCP reachability.
- **[MEASURED]** The 18-token prompt plus 32-token generation completed at 45.18 decode tok/s in this single diagnostic run. This unmatched, tiny-model observation is not a performance baseline.
- **[MEASURED]** Both experiment processes exited, ports 50053/18081 closed, source stayed clean, swap did not grow, sampled GPU edge stayed at or below 43 C, kernel-watch files were empty, and deployed services remained inactive/enabled.

## Boundaries and open gates

**[OPEN]** This listener was briefly exposed only on the directly addressed private USB4 rail, but RPC is explicitly unauthenticated/experimental and its broader security-admission gate is not closed. No external or management-network listener was created.

**[OPEN]** This does not qualify the control revision, dual rails, MPTCP path choice, row/tensor split, tensor parallelism, large-model fit, quality, throughput, latency variance, worker loss, malformed input, restart/reconnect, cache persistence, or single-node fallback. The source described `layer` mode; no claim of tensor parallel execution is made.
