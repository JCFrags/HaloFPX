# ADR-0042: read-only exact-primary allocation authority

- Status: accepted for the L18 no-production milestone
- Date: 2026-07-21
- Base: `730e96330ae0585719941a93b65c31a6217a7a54`
- Scope: exact-primary, metadata-only allocation planning; no production mutation

## Decision

L18 may open and hash the pinned primary GGUF and read its metadata and tensor
descriptors. It must not map weight data, load tensor bytes, or allocate any
material model, KV, or compute buffer. The probe must use `llama_model_params`
with `no_alloc=true`, `use_mmap=false`, and the exact loader arguments
`--device RPC0,ROCm0 --split-mode layer --tensor-split 1,1`.

The real model loader remains placement and allocation-group authority. After
architecture-specific tensor creation and `done_getting_tensors()`, it records
each real per-buffer-type GGML context, its exact backend allocation-size
query, and every tensor descriptor in that context. The report must bind each
created tensor name to a source GGUF descriptor and refuse unknown or
unaccounted source tensors, source-range overflow, duplicate/ambiguous group
identity, or arithmetic overflow. A zero-byte `no_alloc` backend sentinel is
not a material allocation and must be disclosed; beyond the explicit
full-file identity hash, the loader may not read weight data.

The probe must independently bind exact file size and SHA-256 before loader
admission, require exactly `RPC0` then `ROCm0`, require backends `RPC` then
`ROCm`, bind the expected RPC endpoint, and require exact layer/`1,1` parsing.
It reports exact weight-group requests and totals separately from:

- context/KV bytes estimated by the runtime's `no_alloc` context constructor;
- compute bytes estimated by its graph-reservation path;
- a fixed per-device runtime-reserve policy; and
- a separately stated fragmentation allowance.

For L18, capacity admission uses each backend's reported total capacity, not
the production-loaded free value. It requires the exact weight total plus
simulated context and compute bytes plus a 10% fragmentation allowance plus a
fixed 16 GiB runtime reserve to fit on each device without overflow. It also
requires every single exact weight-group request plus the same required margin
to fit. Current free capacity is evidence only because production must remain
live throughout L18.

## Boundary

The report is an allocation preflight, not a load result. Backend size queries
and context/graph simulations do not prove allocator success, fragmentation,
runtime residency, throughput, correctness, or a future production transition.
P01/P11 are supporting prior exact-model evidence only. L18 authorizes no
primary weight allocation or loading, inference, cache operation, production
stop/start, primary retry, or L19 work.
