# RPC and distributed execution

> **Audit snapshot:** fork `a5605a72768c6562241b248e268e33dc92787394`; upstream `86d86ed4396b4130922f7b9af26e3d9fc11a591b`; inventory date `2026-07-17`.  
> **Decision vocabulary:** **RETAIN** = capability/ABI must survive; **REFRESH** = preserve capability but re-port onto current upstream; **RETIRE** = remove the fork patch and use upstream or archive it. [S-FORK-HEAD](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394) [S-UP-HEAD](https://github.com/ggml-org/llama.cpp/tree/86d86ed4396b4130922f7b9af26e3d9fc11a591b)

## Inventory finding

No changed path under `ggml/src/ggml-rpc/` appears in the exact file lists for PR #27, PR #31, or PR #32. The audited recent ROCmFPX series therefore does **not** contain an intentional RPC implementation patch. [S-PR27-FILES](https://github.com/charlie12345/ROCmFPX/pull/27/files) [S-PR31-FILES](https://github.com/charlie12345/ROCmFPX/pull/31/files) [S-PR32-FILES](https://github.com/charlie12345/ROCmFPX/pull/32/files)

Both the fork and pinned upstream RPC implementations serialize tensor metadata with a numeric tensor-type field. ROCmFPX’s custom type IDs can therefore be transported by the generic structure, but a peer that does not understand IDs `100`–`107` cannot execute or reconstruct those tensors correctly. The same-build compatibility requirement is an engineering inference from the RPC tensor metadata and the fork-only type registry. [S-RPC-FORK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-rpc/ggml-rpc.cpp) [S-RPC-UP](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/ggml/src/ggml-rpc/ggml-rpc.cpp) [S-QT-ENUM-FORK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/include/ggml.h) [S-QT-ENUM-UP](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/ggml/include/ggml.h)

## Why upstream RPC must replace the fork copy

| Upstream change | Relevance to ROCmFPX | Primary source |
|---|---|---|
| Move `last_graph_uid` to the RPC device context | Native MTP can switch multiple compute contexts on one RPC device; stale graph IDs are a correctness risk. | [S-RPC-MTP-FIX](https://github.com/ggml-org/llama.cpp/commit/c3e9ade6dd3ff2a1ceafd2d59062634715b472c4) |
| Keep local iGPU when RPC devices are present | The upstream fix explicitly cites Strix Halo unified-memory systems where the local iGPU is the main compute device. | [S-RPC-IGPU-FIX](https://github.com/ggml-org/llama.cpp/commit/1738129bee5c81b06fa1850daf3f958813c76f5f) |
| Add Lightning Indexer and bump RPC version | Demonstrates that the current upstream wire/operation surface continues to evolve. | [S-RPC-LIGHTNING](https://github.com/ggml-org/llama.cpp/commit/00f5442cc4e805293280c8f85d21d8f9d4aad206) |
| Current RPC refactors and hardening | The pinned upstream file is the authoritative base, not the older fork snapshot. | [S-RPC-UP](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/ggml/src/ggml-rpc/ggml-rpc.cpp) [S-UP-HEAD](https://github.com/ggml-org/llama.cpp/tree/86d86ed4396b4130922f7b9af26e3d9fc11a591b) |

## Classification

**RETIRE the fork RPC implementation as a maintained patch.** Replace `ggml/src/ggml-rpc/**` from current upstream and avoid adding ROCmFPX-only protocol commands unless a format cannot be represented by ordinary tensor metadata. [S-RPC-UP](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/ggml/src/ggml-rpc/ggml-rpc.cpp) [S-RPC-MTP-FIX](https://github.com/ggml-org/llama.cpp/commit/c3e9ade6dd3ff2a1ceafd2d59062634715b472c4) [S-RPC-IGPU-FIX](https://github.com/ggml-org/llama.cpp/commit/1738129bee5c81b06fa1850daf3f958813c76f5f)

**RETAIN compatibility tests.** At minimum, test:

- client/server build and protocol version match;
- custom type ID round trip for metadata;
- remote allocation/copy/compute for each advertised ROCmFPX format;
- target plus draft-MTP context switching;
- local Strix iGPU plus one or more RPC devices;
- explicit rejection when a peer lacks a custom type.

These tests are required by the interaction of fork-only type IDs with upstream’s evolving RPC implementation. [S-QT-ENUM-FORK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/include/ggml.h) [S-RPC-UP](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/ggml/src/ggml-rpc/ggml-rpc.cpp) [S-RPC-MTP-FIX](https://github.com/ggml-org/llama.cpp/commit/c3e9ade6dd3ff2a1ceafd2d59062634715b472c4) [S-RPC-IGPU-FIX](https://github.com/ggml-org/llama.cpp/commit/1738129bee5c81b06fa1850daf3f958813c76f5f) [S-RPC-LIGHTNING](https://github.com/ggml-org/llama.cpp/commit/00f5442cc4e805293280c8f85d21d8f9d4aad206)

## Recommended handshake extension

A protocol fork is unnecessary, but a higher-level compatibility check should exchange or derive:

`llama.cpp upstream SHA` + `ROCmFPX extension SHA` + `GGML RPC version` + `supported custom type IDs`.

This is a proposed operational guard, not an existing code claim. It makes an incompatible peer fail before model allocation or graph execution.
