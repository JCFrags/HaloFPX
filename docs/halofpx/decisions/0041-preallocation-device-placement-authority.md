# ADR-0041: pre-allocation device and layer-placement authority

- Status: accepted for the L17 no-production milestone
- Date: 2026-07-21
- Base: `20f19a2d2460bff76f381a43ef8010c0e5f7ff08`
- Scope: disposable two-device HaloFPX coordinator admission only

## Decision

The primary canary command must explicitly contain `--device RPC0,ROCm0` in
that order. After `--rpc 10.44.0.1:50180` registers the disposable worker, but
before any model is opened or allocated, a pinned probe must pass the current
common argument parser and require all of the following:

- exactly two selected non-null devices followed by the parser's terminal null;
- exact selected names and order `RPC0`, then `ROCm0`;
- exact backend registrations `RPC`, then `ROCm`;
- RPC0 description equal to the expected endpoint;
- sane nonzero free and total memory for both devices;
- split mode `layer`, exact tensor split `1,1`, and no additional split value;
- 62 repeating layers plus output resolved by the same helper used by
  `llama_model_base::load_tensors`; and
- repeating ownership 32 RPC / 30 ROCm with output on ROCm, for 32/31 total.

Omitted device selection, reversed or additional devices, a missing local
device, wrong name/backend/endpoint, CPU/one-device resolution, or zero
ownership on either selected device refuses admission. The probe emits one
bounded JSON record containing only endpoint, device/backend names, byte
capacity, split mode, split value, and ownership counts.

The loader's existing layer resolver is extracted without changing its
formula. Both the loader and probe call that helper. The probe supplies a
`/dev/null` parser sentinel because the common parser requires a model
argument; it never opens the sentinel or any model.

## Boundary

This decision prevents the exact L16 omission and catches monolithic predicted
ownership before allocation. It does not prove that the 159.9 GB MiniMax
artifact will load, that its byte distribution will be balanced, or that its
allocator will not make another oversized request. P01/P11 remain the only
successful exact-model evidence and used the same explicit `RPC0,ROCm0` order.

L17 authorizes no production transition, primary artifact access, cache state
operation, performance claim, or subsequent primary attempt.
