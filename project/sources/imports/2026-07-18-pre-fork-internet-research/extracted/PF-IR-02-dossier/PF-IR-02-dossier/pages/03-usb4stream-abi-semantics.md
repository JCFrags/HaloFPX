# USB4STREAM ABI and `/dev/tbstreamX` semantics

## ABI surface

The public surface is a testing ConfigFS ABI plus a misc character device. A group named `<xdomain>.<service>` may be created before the peer exists. A child `$name` creates a stream and a dynamically indexed `/dev/tbstreamX` device.

| Attribute | Meaning | Write constraints |
|---|---|---|
| `index` | maps ConfigFS object to `/dev/tbstreamX` | read-only |
| `in_hopid` | local read/input tunnel HopID | `-1` automatic; `0` release; explicit IDs start at 8; busy while opened |
| `out_hopid` | local write/output tunnel HopID | same; peer-advertised in/out values are reversed locally |
| `ring_size` | TX and RX descriptor count | 32..4096; default 256; `-EBUSY` while opened |
| `throttling` | ring interrupt throttling in ns | 0..16776960; default 8192; `-EBUSY` while opened |

The ABI document says `Date: Sep 2026` and `KernelVersion: v7.2`, although this dossier’s access date is 2026-07-18. The future date is retained as an upstream metadata anomaly.

## Data and ownership model

Each ring slot owns a kernel page. Pages are DMA-mapped by the driver. `write_iter` copies from the caller into those pages; `read_iter` copies from the pages into the caller. DATA frames are at most 4096 bytes. The transport’s frame boundaries are not a record-oriented userspace ABI: reads can split a frame or combine available frames, and writes can be split into multiple frames.

Multiple opens do not create independent streams. They increment one device user count, share one ring pair, and use one lock. Concurrent readers consume from the same byte stream; concurrent writers can interleave at write-call/chunk scheduling boundaries.

## Blocking and errors

- Blocking `open()` waits for a matching service when the only failure is no attached stream. Nonblocking `open()` returns `-ENXIO`. Missing/invalid HopIDs return `-EINVAL`.
- Blocking `read()` waits for a completed RX frame. Nonblocking empty read returns `-EAGAIN`. CLOSE at the front of the queue returns EOF. Copy failure returns `-EFAULT`.
- Blocking `write()` waits for at least one TX slot. Nonblocking full-ring write returns `-EAGAIN`. A large write can return a positive short count after filling available slots.
- Ring/API errors can terminate a read/write with a negative errno. CRC and overrun descriptor flags are only logged in the RX callback; no status record is returned to userspace.
- `poll()` reports ring readability/writability and HUP|ERR only when device validity fails. It does not directly test the driver’s `closed` or `removed` flags. Persistent readiness after CLOSE/removal is therefore a machine-qualification item.

## Lifecycle and teardown

The first open allocates both rings and enables both paths. Subsequent opens share them. The last release calls the CLOSE submission once, ignores that result, flushes TX and RX with 500 ms bounds, stops rings, disables paths and frees pages/rings. The source comment says CLOSE would be sent twice if the first failed, but the implementation calls it once.

Peer removal detaches the service and wakes waiters. ConfigFS objects can remain and attach to a future matching peer. Suspend stops open streams; resume restarts them. Source presence does not prove lossless reconnection, ordering preservation, or clean poll behavior across these transitions.

## Acceleration and interoperability status

| Capability | Status | Basis |
|---|---|---|
| registered userspace buffers | `[UNSUPPORTED]` | no registration UAPI |
| driver `mmap` | `[UNSUPPORTED]` | no `.mmap` fop |
| driver `splice_read` / `splice_write` | `[UNSUPPORTED]` | no splice fops |
| dma-buf / GPU-direct / peer memory | `[UNSUPPORTED]` | no import/export/registration path |
| native zero-copy userspace ABI | `[UNSUPPORTED]` | kernel-page copies are explicit |
| directional stream preference | `[UNSUPPORTED]` | TX and RX are provisioned together |
| non-Linux USB4STREAM peer | `[EVIDENCE-GAP]` | no public implementation/conformance evidence found |

Generic VFS helpers may internally optimize unrelated file operations, but they do not change the driver’s explicit copy-based ABI and are not evidence of zero-copy transport.


## Sources

- **S005** — USB4STREAM implementation source (v7.2-rc3 blob c1f5c55583d069c811d25df95f4e90136255d585)
- **S006** — USB4/USB4STREAM Kconfig (v7.2-rc3-equivalent blob 294b3227a54582536433cbb391057b99bb9df352)
- **S007** — Thunderbolt module Makefile (v7.2-rc3-equivalent blob beb054c3126b1445d26b4c14de039d23367a5100)
- **S008** — Thunderbolt ConfigFS support (v7.2-rc3-equivalent blob dc6bc363dfe8052d7f94c6e3dca3efe9fe9771be)
- **S009** — USB4STREAM ConfigFS ABI documentation (v7.2 source; new-file blob 7abc6b7…)
- **S010** — Thunderbolt administrator guide — USB4STREAM section (v7.2-rc3)
- **S013** — USB4STREAM DMA unmap-size fix proposal and acceptance (blob delta c1f5c55583d0..4cc86d8d6491; fixes branch head observed db79679595326fd3f6bd1e6fd0cefc3ba016039a)
