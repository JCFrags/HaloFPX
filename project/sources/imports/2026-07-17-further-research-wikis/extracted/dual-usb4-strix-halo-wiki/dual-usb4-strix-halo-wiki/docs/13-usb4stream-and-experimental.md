# USB4STREAM and experimental USB4 transports

## Status

USB4STREAM entered the Linux 7.2 development line. Linux 7.1's upstream Kconfig does not contain `CONFIG_USB4_STREAM`; the current mainline Kconfig does. At this snapshot, evaluate it on a test kernel, not as the only production control path.

## User-facing model

Load the module and create a stream group beneath the XDomain service in ConfigFS:

```bash
sudo modprobe thunderbolt_stream
sudo mount -t configfs none /sys/kernel/config 2>/dev/null || true
cd /sys/kernel/config/thunderbolt/stream
sudo mkdir -p 0-1.0/data
echo -1 | sudo tee 0-1.0/data/in_hopid
echo -1 | sudo tee 0-1.0/data/out_hopid
```

The peer sees advertised XDomain properties and can create a matching stream group. Linux then creates `/dev/tbstreamX`. The supplied `scripts/usb4stream-setup.sh` wraps this procedure.

Multiple named streams may coexist on one XDomain, and USB4STREAM may coexist with `thunderbolt-net`.

## Implementation properties

The current upstream driver source establishes the following:

- DATA packets carry up to 4 KiB; CLOSE packets up to 256 bytes.
- End-to-end flow control is mandatory.
- Ring size defaults to 256 descriptors and is configurable from 32 through 4096.
- Interrupt throttling defaults to 8192 ns and has a configurable upper bound.
- Each stream character device has one TX ring and one RX ring.
- Paths are established on first open and torn down on final close.
- The driver supports blocking and nonblocking I/O plus `poll`/epoll readiness.
- RX and TX buffers are page-backed and DMA-mapped.
- `write_iter` copies from the caller's iov into the TX page; `read_iter` copies from the RX page into the caller's iov.
- The file operations do not expose `mmap`, splice-specific methods, XDP, an AF_XDP queue, or a remote-memory registration API.
- Multiple opens of one stream device share the rings and are serialized by a device mutex.
- CRC error and RX buffer-overrun flags are logged, but no cryptographic integrity is provided.

Consequences:

1. It is a raw byte stream, not shared memory.
2. It is not zero-copy at the current userspace ABI.
3. It does not preserve application message boundaries.
4. A custom runtime needs explicit framing and failure recovery.
5. Creating several streams can separate control and data, but each stream is still a single ring pair.

## Two physical links

Each cable/XDomain needs its own ConfigFS parent and stream device. Example conceptual mapping:

```text
cable 0: /sys/kernel/config/thunderbolt/stream/0-1.0/data0 -> /dev/tbstream0
cable 1: /sys/kernel/config/thunderbolt/stream/1-1.0/data1 -> /dev/tbstream1
```

Names and indices are discovery-dependent. Save the parent/service/device mapping and verify with sysfs/udev before opening devices.

## Minimal benchmark

On receiver:

```bash
sudo dd if=/dev/tbstream0 of=/dev/null bs=4M status=progress
```

On sender:

```bash
dd if=/dev/zero of=/dev/tbstream0 bs=4M count=4096 status=progress
```

This measures one-way bulk transfer but does not validate data integrity. For integrity, send a deterministic file and compare hashes after capture to storage or a hashing sink.

Run two independent processes on the two physical stream devices for the first dual-link test. Do not begin with one process multiplexing both; separate processes simplify CPU and counter attribution.

## Custom llama.cpp adapter implications

The existing RPC transport expects `send_data()` and `recv_data()` with exact-length semantics. A USB4STREAM adapter could implement those calls, but it also needs:

- listener/peer discovery and session ownership;
- bidirectional open ordering;
- connection capability handshake;
- message length validation;
- timeout and cancellation;
- close/error propagation;
- reconnection after cable reset;
- authentication/encryption if required;
- dual-link scheduling if two stream devices are used;
- compatibility fallback to TCP.

Because the current driver already presents an ordered stream, the existing RPC framing may be reusable. However, the RPC handshake assumes a network endpoint model and the transport abstraction should be separated cleanly rather than replacing sockets ad hoc.

## Soft-RoCE over USB4NET

Create an RXE device:

```bash
sudo modprobe rdma_rxe
sudo rdma link add rxe_tb0 type rxe netdev thunderbolt0
rdma link
ibv_devices
```

Repeat for the second interface if desired. Validate with `ib_write_bw`/`ib_read_bw` and compare CPU cost to TCP.

Limitations:

- all RoCE packet processing is software;
- USB4NET remains the lower network device;
- no hardware queue pair offload exists in the USB4 controller;
- no generic GPUDirect/peer-memory path follows from RXE;
- one QP does not automatically use both RXE devices;
- the current llama.cpp RDMA path still stages/copies data.

## Out-of-tree NHI/ibverbs research

Experimental projects have explored direct ibverbs-like access to the Thunderbolt/USB4 NHI. These can illuminate controller behavior and potential queue APIs, but they are not upstream Linux ABIs, not portable across controller revisions, and not suitable as a baseline dependency.

Require source review, IOMMU analysis, reset/unplug testing, and a clear security model before loading such modules.

## AF_XDP, DPDK, and io_uring zero-copy

The current `thunderbolt-net` driver does not expose native XDP/AF_XDP queue operations or a DPDK poll-mode driver. Generic XDP does not create a zero-copy userspace queue. io_uring can submit ordinary socket/file I/O, but it cannot remove the current driver's page-to-user copy by itself.

## Research priorities

1. Measure USB4STREAM CPU-seconds/GiB versus tuned USB4NET TCP at the same payload and affinity.
2. Measure ring-size and throttling sensitivity.
3. Add robust framed integrity tooling.
4. Prototype a transport abstraction in llama.cpp without changing RPC semantics.
5. Evaluate whether registered-buffer or splice/mmap support is feasible upstream.
6. Prove reset, close, and reconnect behavior under cable removal.
