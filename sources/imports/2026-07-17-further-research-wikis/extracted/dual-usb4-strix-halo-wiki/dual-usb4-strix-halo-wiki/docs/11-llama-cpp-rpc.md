# llama.cpp RPC integration

## Current upstream behavior

The current llama.cpp RPC backend is a proof-of-concept transport that exposes one or more remote ggml devices. The upstream documentation identifies TCP as the normal data path and warns that the RPC service is fragile and insecure.

The current Linux transport implementation:

- creates IPv4 `AF_INET`, `SOCK_STREAM` sockets with protocol 0, meaning ordinary TCP;
- sets `TCP_NODELAY` on connected/accepted sockets;
- uses blocking send/receive loops over one ordered byte stream;
- binds the RPC server to `127.0.0.1:50052` by default;
- accepts `-H/--host` and `-p/--port` for listener configuration;
- can negotiate a verbs/RDMA data path when built with RDMA and when a matching RoCE-capable verbs device/GID is available;
- retains the TCP socket for control/liveness when RDMA is activated;
- does not provide TLS or authentication.

The implementation is therefore compatible with transparent MPTCP conversion: the RPC framing still sees a reliable ordered stream.

## Build

On both nodes:

```bash
git clone https://github.com/ggml-org/llama.cpp.git
cd llama.cpp
cmake -S . -B build -DGGML_RPC=ON -DGGML_HIP=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Choose backends appropriate to the target distribution and current ROCm support. The network research does not depend on HIP; CPU-only RPC is sufficient for transport validation.

## Secure listener scope

The server defaults to loopback. To make it reachable on the dedicated fabric:

```bash
./build/bin/ggml-rpc-server -H 10.44.0.2 -p 50052 -c
```

Binding to only `10.44.0.2` is narrow, but an MPTCP additional subflow targets `10.44.1.2` at the same port. For MPTCP, use one of:

- bind `0.0.0.0` and restrict ingress with nftables;
- place both USB4 addresses in an isolated network namespace and bind `0.0.0.0` there;
- patch the listener for multiple bind addresses or a wildcard listener with explicit device filtering.

Reference command:

```bash
mptcpize run ./build/bin/ggml-rpc-server -H 0.0.0.0 -p 50052 -c
```

Run the client through `mptcpize` as well:

```bash
mptcpize run ./build/bin/llama-cli \
  -m /models/model.gguf \
  -ngl 99 \
  --rpc 10.44.0.2:50052
```

The initial connection uses cable 0; the MPTCP path manager adds cable 1.

## Verify conversion

```bash
GGML_RPC_DEBUG=1 mptcpize run ./build/bin/ggml-rpc-server -H 0.0.0.0 -p 50052

# During a client request
ss -Mani '( sport = :50052 or dport = :50052 )'
sudo ss -tani '( sport = :50052 or dport = :50052 )'
ip -ts mptcp monitor
```

If `ss -M` shows no MPTCP socket, check:

```bash
ldd ./build/bin/ggml-rpc-server
LD_DEBUG=libs mptcpize run ./build/bin/ggml-rpc-server --help 2>&1 | grep -i mptcp
strace -f -e socket,bind,listen,connect,accept ./your-command
```

## Native MPTCP option

`patches/0001-ggml-rpc-optional-native-mptcp.patch` changes both listener and client socket creation to use `IPPROTO_MPTCP` when `GGML_RPC_MPTCP=1` is set. Native selection is preferable to an implicit preload wrapper for a controlled deployment because it is visible in the application configuration and fails clearly when MPTCP is unavailable.

Example after applying the patch:

```bash
GGML_RPC_MPTCP=1 ./build/bin/ggml-rpc-server -H 0.0.0.0 -p 50052 -c
GGML_RPC_MPTCP=1 ./build/bin/llama-cli -m model.gguf -ngl 99 --rpc 10.44.0.2:50052
```

The patch is pinned to the source shape observed at this wiki's snapshot and may need rebasing.

## Why two `--rpc` addresses are not transparent multipath

The comma-separated `--rpc` list represents multiple RPC server endpoints/devices. It does not tell the backend that two addresses lead to the same remote device through two paths. Running two server instances on the same remote APU/GPU can:

- expose duplicate logical devices;
- duplicate model state or cache;
- oversubscribe compute and memory bandwidth;
- alter tensor splitting;
- fail independently in ways the client does not interpret as path failover.

Use two endpoints only when they represent intentionally separate RPC devices or an explicitly designed multi-server partition. Use MPTCP for two paths to one RPC server.

## RPC cache and network measurement

The `-c/--cache` option stores large tensors locally on the remote host. This can substantially change link utilization during model loading. Measure both:

1. cold cache / first load;
2. warm cache / repeated load;
3. prompt processing;
4. token generation;
5. graph/tensor transfer bytes per phase.

Record `GGML_RPC_DEBUG=1` logs together with link and MPTCP counters.

## Current RDMA path

When built with `libibverbs`, llama.cpp probes verbs devices and matches a GID to the local TCP socket address. If both peers advertise compatible capabilities, it activates a reliable-connected QP. The current code uses 256 KiB staging chunks and copies into/out of registered buffers. It is not a zero-copy mapping of ggml tensor memory.

A hardware RoCE NIC is the intended case. Soft-RoCE over a USB4NET interface may satisfy the verbs/GID discovery path, but it is software RDMA over the same netdev and must be treated as experimental. It may increase CPU work and does not make USB4 memory-coherent.

## RPC benchmark metrics

Collect:

```text
model-load wall time
remote tensor bytes
prompt tokens/s and prompt latency
generation tokens/s and inter-token latency
RPC server CPU-seconds
client CPU-seconds
per-link bytes and errors
MPTCP subflow bytes/retransmissions
GPU/APU utilization and memory occupancy
```

A transport that wins iperf3 can still lose end-to-end inference if RPC serialization, compute partitioning, or synchronization dominates.
