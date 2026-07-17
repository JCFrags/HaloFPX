# Deployed RPC tensor-cache audit

## Scope and identity

- Deployed repository: `https://github.com/charlie12345/rocmfp4-llama.git`.
- Exact commit on both nodes: `4860505ee322091f0f61eba77d6ad49be88cf4ea`.
- Relevant source: `tools/rpc/rpc-server.cpp`, `tools/rpc/README.md`, and `ggml/src/ggml-rpc/ggml-rpc.cpp` at that commit.
- Live nimo-1 process enabled `rpc-server --cache` with `LLAMA_CACHE` routed under `/opt/llm-usb4-cluster/rocmfp4-rpc-cache`.

## Verified implementation behavior

- **[VERIFIED]** The RPC README describes this cache as storage for large tensors to avoid retransmitting them during model loading. It is not attention KV, session, prefix, or checkpoint persistence.
- **[VERIFIED]** For tensors over the implementation threshold, the server computes a 64-bit FNV hash, uses the hexadecimal hash as the filename, and writes bytes directly to that final path with `std::ofstream`.
- **[VERIFIED]** The observed write path does not use a temporary file plus atomic rename, does not call or check `fsync`, and does not check the stream's final status before the tensor is installed.
- **[VERIFIED]** On reuse, `get_cached_file()` reads the file selected by the requested hash but does not recompute the content hash or compare an independent digest.
- **[VERIFIED]** The bounds check rejects a cached file that is too large for the target region. A shorter or same-sized corrupted file is not cryptographically rejected by the inspected path before `ggml_backend_tensor_set()` and a success response.

## Measured live state

- **[MEASURED]** nimo-1 held 187 regular files consuming about 112 GiB in the RPC cache.
- **[MEASURED]** Many observed files were 641,728,512 bytes each.
- **[MEASURED]** The root filesystem had only about 43 GiB free at the same capture.
- **[MEASURED]** nimo-2 had no equivalent RPC cache directory because it was the coordinator/local-device host.

## Project disposition

- **[RECOMMENDATION]** Classify the deployed cache as a useful weight-transfer optimization and a design donor, not an admissible HaloKV implementation.
- **[RECOMMENDATION]** Before carrying it into HaloFPX, require a cryptographic content digest bound to tensor/model/runtime identity; atomic temp-write/flush/rename publication; read-time digest and exact-length verification; cache quota, reserve, eviction, and inspection; and corruption-as-miss behavior.
- **[RECOMMENDATION]** Preserve RPC tensor cache and persistent prompt/session/KV cache as separate namespaces, schemas, quotas, and threat boundaries.
- **[OPEN]** Cache hit/miss logs, load-time benefit, write amplification, eviction needs, and behavior after partial/corrupt files require controlled experiments on disposable cache copies.

