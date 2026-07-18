# Network-plugin ABI and custom transport boundary

## Versioned ABI

| RCCL line | Current Net struct | Loader compatibility set | Current symbol |
|---|---|---|---|
| 2.27.7 | v10 | v10, v9, v8, v7, v6 | `ncclNetPlugin_v10` |
| active 2.30.4 | v12 | v12, v11, v10, v9, v8, v7, v6 | `ncclNetPlugin_v12` |

A plugin must be built and tested against the exact RCCL header/runtime pair. Loader compatibility means older symbols may be accepted; it does not mean a newer plugin can be dropped into an older runtime.

## Selection and fallback

The loader can obtain an external library from `NCCL_NET_PLUGIN`, probes versioned exported symbols, and appends internal IB and Socket providers as fallbacks. `NCCL_NET`/communicator `netName` selects the provider by its declared name. The harness must capture logs proving both the library and provider selected; silent fallback would invalidate a custom-transport result.

## v12 responsibilities relevant to USB4STREAM

A custom provider is responsible for:

* device enumeration and topology properties;
* nonblocking listener/connect/accept progress;
* host/device/DMA-BUF pointer capability declarations;
* memory registration and deregistration;
* asynchronous send, receive, and completion polling;
* visibility/flush semantics for device receives;
* connection and listener teardown;
* error propagation and optional device-offload hooks.

A USB4STREAM proof of concept therefore needs ABI conformance tests before any collective benchmark. A fast byte channel without correct progress, ordering, visibility, and teardown is not an RCCL transport.
