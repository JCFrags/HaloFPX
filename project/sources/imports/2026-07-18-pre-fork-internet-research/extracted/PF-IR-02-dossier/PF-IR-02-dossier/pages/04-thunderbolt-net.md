# `thunderbolt-net` / USB4NET audit

## Framing and MTU

`thunderbolt-net` is a netdevice transport, not a raw byte-stream character device. It fragments an Ethernet skb into 4096-byte DMA frames. Each frame carries a 12-byte little-endian header (`frame_size`, `frame_index`, `frame_id`, `frame_count`), leaving 4084 bytes of payload. START and END PDF values are 1 and 2. The protocol-frame ceiling is 64 KiB; Linux exposes MTU 68..65522.

The control protocol uses LOGIN, LOGIN_RESPONSE, LOGOUT and STATUS messages, protocol version 1, with 4500 ms initial login delay, 500 ms login timeout, 1000 ms logout timeout, 60 login retries and 10 logout retries.

## Rings, queues and NAPI

TX and RX rings have 256 slots and 128000 ns interrupt throttling. TX copies skb content into DMA pages. RX allocates page-backed buffers large enough for a frame and skb shared information; completed fragments are reassembled and the final packet is delivered through GRO. NAPI budget accounting follows accepted DMA completions, not necessarily completed Ethernet packets.

The netdevice advertises SG, TSO, GRO, IPv4/IPv6 checksum features and HIGHDMA. These are software/driver interface capabilities, not application-throughput measurements.

## Flow control and lifecycle

A read-only `e2e` module parameter defaults true. End-to-end flow control is used only when both local policy and peer capability permit it. Login success establishes carrier and starts data paths. Disconnect, netdevice close, shutdown and suspend stop login work, carrier/queues, NAPI and rings in the driver’s serialized connection lifecycle; suspend detaches a running netdevice and unregisters the protocol handler.

USB4STREAM differs materially: it requires E2E, exposes a misc character device, and allocates independent rings per configured stream. Both services can exist simultaneously.

## Errors and statistics

CRC, buffer overrun, length, missed-frame and aggregate error paths feed netdevice counters. TX failure paths free/rewind buffers and update TX errors/drops as appropriate. A source-level accounting asymmetry matters for interpretation: TX packet count is skb-oriented; RX packet count advances with accepted USB4NET DMA frames. Raw counter comparison is therefore not an Ethernet packet-rate comparison without normalization.

Some replenish/poll support return values do not have a dedicated user-visible transport-status ABI. The dossier records this as a fault-injection item, not as proof of packet loss or a released defect.

## Interoperability

Microsoft documents USB4NET in the Windows USB4 connection manager, including Thunderbolt 3 compatibility and a two-PC interdomain use case. This establishes a non-Linux implementation class for USB4NET. It does not establish USB4STREAM interoperability.


## Sources

- **S014** — thunderbolt-net implementation source audit capture (v7.2-rc3 blob 02a91650561ab1556d28c4945cae7f62a704b1f3)
- **S023** — Windows USB4 connection-manager and USB4NET documentation (current at access)
