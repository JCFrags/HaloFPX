# L100 KV physical-range coverage diagnosis

Status: **SOURCE-PROVEN UNCOVERED READABLE RANGE; STOP WITHOUT CORRECTION**

The machine-readable audit is
[`l98-kv-physical-range-audit.json`](evidence/l100-offline/l98-kv-physical-range-audit.json).
It covers all 124 retained replay KV tensors and every retained RPC capture,
stage, and apply component.

## Exact defect

The KV state writer requests one occupied row per cell. For L98 q8_0 K and V,
that is 1,128 rows × 1,088 bytes = 1,227,264 bytes per tensor
([llama-kv-cache.cpp](../../src/llama-kv-cache.cpp)).

The device-capture constructor converts that byte count to a 1-D element count
with:

`n = winfo.size / ggml_element_size(winfo.tensor)`

and then creates a quantized tensor with `ggml_new_tensor_1d(type, n)`
([llama-context.cpp](../../src/llama-context.cpp)). For q8_0,
`ggml_element_size()` is the 34-byte block size, while the new tensor again
applies the 32-element q8_0 block ratio. The requested 1,227,264 bytes therefore
become:

`(1,227,264 / 34) / 32 × 34 = 38,352 bytes`

This exactly matches every one of the 64 retained RPC capture/stage/apply
components. The capture and restore path copies only that reduced tensor.

## Coverage result

- RPC KV tensors: 64 (layers 0–31, K and V).
- Local ROCm KV tensors: 60 (layers 32–61, K and V).
- Provably read occupied RPC bytes: 78,544,896.
- Serialized and restored RPC bytes: 2,454,528.
- Unrepresented but provably read occupied RPC bytes: **76,090,368**.
- Unrepresented but provably read occupied local ROCm bytes:
  **71,334,720**.
- Total unrepresented but provably read occupied bytes across all 124 tensors:
  **147,425,088**.
- Additional padded rows 1,128–1,279: 10,584,064 bytes classified only as
  conservatively possibly read; they are not needed to establish the defect.

For each RPC tensor, the exact uncovered occupied interval is allocation offset
`+38,352` through `+1,227,264`. The q8_0 K dot-product and V dequantization
load complete 34-byte blocks from the attention views
([fattn-common.cuh](../../ggml/src/ggml-cuda/fattn-common.cuh)); the HIP
quantized tile path uses the same block loads
([fattn-tile.cuh](../../ggml/src/ggml-cuda/fattn-tile.cuh)). The final replay
attends over the 1,128 occupied prefix, so these ranges are not merely
allocation padding.

The coordinator-local blob is exactly 2,301,688 bytes: 568 bytes of its
source-defined header/length framing plus 60 × 38,352-byte tensor payloads.
Thus the same constructor defect covers all 60 local ROCm tensors as well.
Their stable identities and live allocation ranges come from replay authority;
their canonical blob offsets come from the source K-then-V writer order.

## Smallest safe correction

Do not implement in L100. A future correction should derive the quantized copy
element count using `block_size / type_size`, or use a byte-exact
allocation/view contract, and must refuse capture unless
`ggml_nbytes(copy) == winfo.size`. Restore must retain the same exact byte
contract. Focused q8_0/q4 block, multi-range, and nonquantized cases should
precede any primary retry.

Because an uncovered provably readable range is source-proven, a per-layer
primary instrumentation run is not yet justified.
