# Glossary

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


| Term | Definition |
|---|---|
| TTFT | Time from request send/accept boundary to first non-empty output token, with client/server variant named |
| ITL | Time between consecutive output token events after the first |
| TPOT | Average time per output token excluding the first token |
| Prefill | Prompt ingestion and KV construction before decode |
| Decode | Autoregressive generation after the first token |
| Goodput | Work completed while satisfying latency, correctness, and success SLOs |
| C0/C1/C2/C3 | Power-on cold / OS-cache warm / model-resident warm / exact-prefix cache-hit states |
| Read amplification | Physical block bytes divided by logical unique model bytes loaded |
| USB4NET | Host-to-host IP networking over USB4/Thunderbolt |
| Matched baseline | Single-node result with all non-independent variables equivalent |
| Capacity extension | Dual-node operation that enables a workload not safely runnable on one node; not a speedup claim |
| Evidence gap | Missing required raw samples or provenance over an interval |
| Stable profile | Explicit model/context/load/SLO/trust-boundary combination accepted by G4 |
