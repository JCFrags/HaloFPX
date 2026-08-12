# MiniMax M2.7 UD-Q6_K_XL dual-node LAN deployment

Status: active and LAN-verified on 2026-07-17.

This experiment reactivated the previously measured fast Q6 deployment after
cleanly stopping the MiniMax ROCmFP4 coordinator and worker. It did not modify
the model shards or runtime binaries.

## Effective plan

| Field | Effective value |
|---|---|
| Coordinator | `nimo-1` (`192.168.40.11`) |
| RPC worker | `nimo-2` (`10.44.0.2:50052`) |
| Model | MiniMax M2.7 UD-Q6_K_XL, six shards |
| API alias | `minimax-m2.7-ud-q6-k-xl` |
| Runtime | llama.cpp fingerprint `b1-8f114a9b` |
| `llama-server` SHA-256 | `d62ab220a4743a347461c958ce99a701e7ed21a938d9ab033334d9fb77fabbdb` |
| Placement | layer split, tensor split `1,1`, local ROCm plus RPC ROCm |
| Transport | MPTCP over both USB4 Thunderbolt-net rails |
| Context | 131,072 total; two independent 65,536-token slots |
| Batch / microbatch | 4096 / 4096 |
| KV cache | q8_0 K and V |
| I/O | no mmap, Direct I/O |
| Scheduling | 16 threads, poll 100, priority 2 |
| Reuse | prompt cache, reuse 256, RAM cache 12,288 MiB, idle-slot cache, context checkpoints |
| Speculation | disabled; prior local tests found no accepted n-gram draft tokens |
| API listener | `0.0.0.0:8081` |

## Live validation

- **[MEASURED]** `/health` returned HTTP 200 with `{"status":"ok"}` from the
  Windows LAN client.
- **[MEASURED]** `/v1/models` reported the expected alias, Q6_K, 228,689,764,864
  parameters, 207,437,154,304 model bytes, and runtime fingerprint `b1-8f114a9b`.
- **[MEASURED]** `/slots` reported two idle 65,536-token slots.
- **[MEASURED]** `ss -Mti` reported one MPTCP session with `subflows_total:2`.
  The subflows used `10.44.0.1 -> 10.44.0.2` and
  `10.44.0.5 -> 10.44.0.6`, proving both USB4 rails were active.
- **[MEASURED]** A LAN OpenAI-compatible chat request completed in 4.848 s.
  Server timings were 45.30 prompt tokens/s for 54 tokens and 18.00 generated
  tokens/s for 64 tokens. This is a smoke measurement, not a controlled
  benchmark.
- **[MEASURED]** A second request reached `finish_reason: stop` and returned the
  requested final answer, `MiniMax M2.7 Q6 LAN deployment verified.` Generation
  measured 18.08 tokens/s for 180 tokens.
- **[MEASURED]** Both systemd services and both MPTCP transport services were
  active; the Q6 coordinator and worker services were enabled for reboot.

## Endpoint

- OpenAI-compatible base URL: `http://192.168.40.11:8081/v1`
- Health URL: `http://192.168.40.11:8081/health`
- Model: `minimax-m2.7-ud-q6-k-xl`

The endpoint is an unauthenticated llama.cpp LAN listener. Do not expose port
8081 to the public internet without an authenticated TLS reverse proxy.
