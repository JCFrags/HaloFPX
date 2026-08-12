# Validation Receipt

Date: 2026-07-10
Result: PASS

## Acceptance Results

- Both USB4 links are up at MTU 9000.
- A single MPTCP RPC socket has two subflows and transferred 121.65 GB during model load.
- Primary per-cable transmit totals were approximately 91.75 GB and 89.31 GB.
- Both ROCm `gfx1151` devices hold model allocations.
- Worker TCP 50052 is reachable from `10.44.0.0/29` and blocked on the management LAN.
- Kimi-K2-Thinking `UD-TQ1_0` loaded in about six minutes with zero service restarts.
- `/health` returned `{"status":"ok"}`.
- Chat completion returned `READY` with `finish_reason=stop`.

## SHA-256

| File | Bytes | SHA-256 |
|---|---:|---|
| `api-chat-response.json` | 1123 | `516f12f59956890c26f86a4ed9e381bbb365ac3448420e003c6eef89ec220ad1` |
| `api-health.json` | 15 | `a29ee2b15c494311c52521766e44af56a3ad2248e7a8ab465e5206463c13d288` |
| `final-report.txt` | 18268 | `ceefb3e3cc1dd67ea9a947b01c9139ed7c30f36bf9f9510815d12833c37e475a` |
| `iperf3-links-after.log` | 6030 | `129396b6ff0d1830d4d70d57969d01757f7e760f6a87d941657fc2772f1d915a` |
| `iperf3-mptcp-socket.log` | 2570 | `d6f041dacc1f3ba85104fb1376020119ce73b81bbd905499390bee97fd470728` |
| `iperf3-primary.log` | 2028 | `47724f37de31323cebac3bcd7afd43fd6cb4649fcaf4a1005b773d8d476701ec` |
