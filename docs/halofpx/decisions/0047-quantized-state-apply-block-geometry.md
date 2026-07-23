# ADR-0047: Quantized state application uses scalar block geometry

- Status: accepted for the default-off HaloFPX local-state path
- Date: 2026-07-23
- Base: `8b54091efe456c8222528ec455316afbca8c8562`

## Context

L29 proved that the authenticated capture object and validated staging tensors
agreed while the immediate live-post-apply aggregate differed. Source audit
localized a concrete error in `rpc_server::hfx_state_commit_apply`: the code
divided a component byte count by `ggml_element_size()` and passed that value
to `ggml_view_1d()` as a scalar element count.

For block-quantized types those units differ. A 1,024-element Q8_0 component is
1,088 bytes (`32` blocks of `34` bytes). The old calculation produced a
32-element Q8_0 view, which spans only one 34-byte block, rather than the full
1,088-byte component.

## Decision

The commit-live path derives the scalar element count as:

`(component_bytes / ggml_type_size(type)) * ggml_blck_size(type)`

It refuses zero sizes, a byte count not divisible by the type size, checked
multiplication failure, a zero result, or a result greater than `INT64_MAX`.
This is the same scalar-count authority expected by `ggml_view_1d()`.

Default-off diagnostics may emit authenticated per-component identity,
content digest, normalized buffer-relative range, deterministic leaf/Merkle
hashes, and a summary tag. Diagnostics never emit state bytes or require raw
pointer equality across model residencies. Overlapping normalized ranges,
incomplete ordinals, malformed summaries, or failed authentication are
refused by the offline analyzer.

## Boundary

This decision changes only the compile- and runtime-default-off HaloFPX
commit-live path. It does not change RPC reconnection semantics, cache
promotion policy, or production configuration. CUDA/ROCm copy and readback
paths were audited and synchronize before returning; no asynchronous-copy,
aliasing, or padding defect was established.

