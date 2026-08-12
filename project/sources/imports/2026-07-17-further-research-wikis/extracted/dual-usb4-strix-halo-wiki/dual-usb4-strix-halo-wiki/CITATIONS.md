# Citation map

**Snapshot:** 2026-07-17. Claims are atomic retrieval units. Source-code claims are pinned to the revisions and blob IDs in [SOURCE-SNAPSHOT.md](SOURCE-SNAPSHOT.md).

## Claims

| ID | Kind | Confidence | Claim | Sources |
|---|---|---|---|---|
| C001 | upstream-fact | high | AMD Ryzen AI Max+ 395 advertises two native USB4 40 Gbit/s ports | [amd-395](https://www.amd.com/en/products/processors/laptop/ryzen/ai-300-series/amd-ryzen-ai-max-plus-395.html) |
| C002 | upstream-fact | high | Linux creates one USB4NET virtual Ethernet interface per Thunderbolt port | [linux-usb4-guide](https://docs.kernel.org/admin-guide/thunderbolt.html) |
| C003 | upstream-fact | high | Linux typically exposes one domain per USB4/Thunderbolt host controller | [linux-usb4-guide](https://docs.kernel.org/admin-guide/thunderbolt.html) |
| C004 | source-code-fact | high | Linux 7.1 thunderbolt-net has one NAPI object and one TX/RX ring pair per netdev | [usb4net-7.1](https://github.com/torvalds/linux/blob/v7.1/drivers/net/thunderbolt/main.c) |
| C005 | source-code-fact | high | Linux 7.1 thunderbolt-net advertises maximum MTU 65522 and splits into roughly 4 KiB frames | [usb4net-7.1](https://github.com/torvalds/linux/blob/v7.1/drivers/net/thunderbolt/main.c) |
| C006 | source-code-fact | high | USB4STREAM is in the Linux 7.2 development line and absent from Linux 7.1 Kconfig | [usb4stream-mainline](https://github.com/torvalds/linux/blob/master/drivers/thunderbolt/stream.c), [usb4-kconfig-mainline](https://github.com/torvalds/linux/blob/master/drivers/thunderbolt/Kconfig) |
| C007 | source-code-fact | high | USB4STREAM currently exposes copy-based read/write character devices, not remote shared memory | [usb4stream-mainline](https://github.com/torvalds/linux/blob/master/drivers/thunderbolt/stream.c) |
| C008 | upstream-fact | high | Flow-hash bonding does not spread one TCP connection; balance-rr can but risks reordering | [bonding](https://docs.kernel.org/networking/bonding.html) |
| C009 | upstream-fact | high | MPTCP can aggregate interfaces under one logical connection and provide failover | [mptcp-doc](https://docs.kernel.org/networking/mptcp.html) |
| C010 | engineering-inference | medium | The reference laminar endpoint design pairs the second local address with the second announced address | [mptcp-pm](https://www.mptcp.dev/pm.html) |
| C011 | source-code-fact | high | Current llama.cpp RPC uses IPv4 TCP sockets and has no authentication/TLS | [llama-rpc-readme](https://github.com/ggml-org/llama.cpp/blob/master/tools/rpc/README.md), [llama-rpc-transport](https://github.com/ggml-org/llama.cpp/blob/master/ggml/src/ggml-rpc/transport.cpp), [llama-security](https://github.com/ggml-org/llama.cpp/blob/master/SECURITY.md) |
| C012 | engineering-inference | high | Two --rpc endpoints to the same remote device are not a transparent multipath abstraction | [llama-rpc-readme](https://github.com/ggml-org/llama.cpp/blob/master/tools/rpc/README.md) |
| C013 | upstream-fact | high | Soft-RoCE can be created over USB4NET but is software processing, not USB4 hardware RDMA | [rxe](https://github.com/linux-rdma/rdma-core/blob/master/Documentation/rxe.md) |
| C014 | engineering-inference | high | Distinct domains and NHI BDFs are strong topology evidence but concurrent throughput is needed for end-to-end capacity independence | [linux-usb4-guide](https://docs.kernel.org/admin-guide/thunderbolt.html) |
| C015 | heuristic | n/a | Aggregate >=1.7x best isolated and each path >=80% retained is a project heuristic, not a standard | Project-defined |
| C016 | upstream-fact | high | MPTCP authenticates additional subflow joins and ADD_ADDR signaling to the existing connection with key-derived HMACs, but does not provide application peer identity or payload encryption | [rfc8684](https://www.rfc-editor.org/rfc/rfc8684.html) |
| C017 | source-code-fact | high | Linux 7.1 software-CM XDomain scanning and control matching are route keyed, allowing distinct route objects within a domain at the object-model level | [software-cm-7.1](https://github.com/torvalds/linux/blob/v7.1/drivers/thunderbolt/tb.c), [xdomain-7.1](https://github.com/torvalds/linux/blob/v7.1/drivers/thunderbolt/xdomain.c) |
| C018 | source-code-fact | high | Linux 7.1 firmware/ICM XDomain connect handling removes an existing same-remote-UUID XDomain when the UUID appears at a different route within the same USB4 domain | [icm-7.1](https://github.com/torvalds/linux/blob/v7.1/drivers/thunderbolt/icm.c) |
| C019 | source-code-fact | high | Linux selects software CM when ACPI grants USB4 native control; otherwise it tries firmware/ICM first and falls back to software CM | [nhi-cm-select-7.1](https://github.com/torvalds/linux/blob/v7.1/drivers/thunderbolt/nhi.c) |
| C020 | engineering-inference | high | Same-UUID lookup in the ICM path is scoped to one USB4 domain, so separate domainX/NHI instances avoid that same-domain replacement scope | [icm-7.1](https://github.com/torvalds/linux/blob/v7.1/drivers/thunderbolt/icm.c) |
| C021 | upstream-fact | high | At the 2026-07-17 snapshot kernel.org lists Linux 7.1.3 as stable, 7.2-rc3 as mainline, and 6.18.38 as longterm | [kernel-releases](https://www.kernel.org/) |

## Source catalog

| Source ID | Type | Title |
|---|---|---|
| `kernel-releases` | primary | [Linux Kernel Archives](https://www.kernel.org/) |
| `linux-usb4-guide` | primary | [USB4 and Thunderbolt — Linux documentation](https://docs.kernel.org/admin-guide/thunderbolt.html) |
| `usb4net-7.1` | primary-source | [Linux 7.1 thunderbolt-net source](https://github.com/torvalds/linux/blob/v7.1/drivers/net/thunderbolt/main.c) |
| `usb4net-kconfig` | primary-source | [Linux 7.1 USB4NET Kconfig](https://github.com/torvalds/linux/blob/v7.1/drivers/net/thunderbolt/Kconfig) |
| `usb4stream-mainline` | primary-source | [Linux mainline USB4STREAM source](https://github.com/torvalds/linux/blob/master/drivers/thunderbolt/stream.c) |
| `usb4-kconfig-mainline` | primary-source | [Linux mainline USB4 Kconfig](https://github.com/torvalds/linux/blob/master/drivers/thunderbolt/Kconfig) |
| `mptcp-doc` | primary | [Linux MPTCP documentation](https://docs.kernel.org/networking/mptcp.html) |
| `mptcp-pm` | primary-project | [MPTCP path-manager guide](https://www.mptcp.dev/pm.html) |
| `mptcp-setup` | primary-project | [MPTCP application setup](https://www.mptcp.dev/setup.html) |
| `rfc8684` | primary-standard | [RFC 8684 — Multipath TCP v1](https://www.rfc-editor.org/rfc/rfc8684.html) |
| `bonding` | primary | [Linux Ethernet Bonding Driver HOWTO](https://docs.kernel.org/networking/bonding.html) |
| `network-scaling` | primary | [Scaling in the Linux Networking Stack](https://docs.kernel.org/networking/scaling.html) |
| `llama-rpc-readme` | primary | [llama.cpp RPC README](https://github.com/ggml-org/llama.cpp/blob/master/tools/rpc/README.md) |
| `llama-rpc-transport` | primary-source | [llama.cpp RPC transport source](https://github.com/ggml-org/llama.cpp/blob/master/ggml/src/ggml-rpc/transport.cpp) |
| `llama-security` | primary | [llama.cpp security policy](https://github.com/ggml-org/llama.cpp/blob/master/SECURITY.md) |
| `amd-395` | primary-vendor | [AMD Ryzen AI Max+ 395](https://www.amd.com/en/products/processors/laptop/ryzen/ai-300-series/amd-ryzen-ai-max-plus-395.html) |
| `rxe` | primary-project | [rdma-core RXE documentation](https://github.com/linux-rdma/rdma-core/blob/master/Documentation/rxe.md) |
| `hellas-nhi` | field-research | [thunderbolt-ibverbs research series](https://blog.hellas.ai/blog/thunderbolt-ibverbs/4-thunderbolt-ibverbs/) |
| `xdomain-7.1` | primary-source | [Linux 7.1 XDomain core source](https://github.com/torvalds/linux/blob/v7.1/drivers/thunderbolt/xdomain.c) |
| `icm-7.1` | primary-source | [Linux 7.1 firmware/ICM connection-manager source](https://github.com/torvalds/linux/blob/v7.1/drivers/thunderbolt/icm.c) |
| `software-cm-7.1` | primary-source | [Linux 7.1 software connection-manager source](https://github.com/torvalds/linux/blob/v7.1/drivers/thunderbolt/tb.c) |
| `nhi-cm-select-7.1` | primary-source | [Linux 7.1 NHI connection-manager selection source](https://github.com/torvalds/linux/blob/v7.1/drivers/thunderbolt/nhi.c) |

## Interpretation

- **Upstream fact:** documented by a project, vendor, or standard body.
- **Source-code fact:** observed in the pinned implementation and subject to later change.
- **Engineering inference:** a reasoned conclusion that still requires target-platform validation.
- **Heuristic:** a project acceptance threshold, not a protocol guarantee.

See also: [source snapshot](SOURCE-SNAPSHOT.md) and [full references](docs/16-references.md).
