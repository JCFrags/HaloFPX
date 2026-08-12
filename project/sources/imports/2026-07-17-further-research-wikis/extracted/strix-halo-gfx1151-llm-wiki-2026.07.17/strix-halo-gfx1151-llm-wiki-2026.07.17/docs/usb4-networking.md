# USB4 and Thunderbolt networking

## Standard, upstream IP networking

Linux exposes IP networking over USB4/Thunderbolt through `CONFIG_USB4_NET`; the module name is `thunderbolt_net`. The Kconfig entry depends on `USB4` and `INET` and implements the Apple ThunderboltIP protocol. Sources: [LINUX-USB4-NET-KCONFIG](sources.md#linux-usb4-net-kconfig), [LINUX-THUNDERBOLT-DOC](sources.md#linux-thunderbolt-doc).

Required kernel configuration:

```text
CONFIG_USB4=y or m
CONFIG_INET=y
CONFIG_USB4_NET=m or y
```

Basic two-host setup:

```bash
# Host A
sudo modprobe thunderbolt-net
sudo ip link set thunderbolt0 up
sudo ip address replace 192.168.44.1/30 dev thunderbolt0

# Host B
sudo modprobe thunderbolt-net
sudo ip link set thunderbolt0 up
sudo ip address replace 192.168.44.2/30 dev thunderbolt0
```

Then test `ping` and both iperf directions. The included [`usb4-net-setup.sh`](../scripts/usb4-net-setup.sh) is dry-run by default and requires `--apply` before changing an interface.

A community guide reports roughly 9 Gbit/s per link on specific Strix Halo systems. Cable generation, topology, tunneling, firmware, and CPU placement can change this substantially. Source: [STRIXHALO-WIKI-CLUSTERING](sources.md#strixhalo-wiki-clustering).

### llama.cpp use

Once `thunderbolt0` has IP addresses, llama.cpp RPC traffic is normal TCP/IP traffic. Bind explicitly to the USB4 address or add a route so Ethernet is not selected accidentally. Capture `ss -tnp`, `ip route get PEER`, and per-interface counters during tests.

## Research-only verbs transport

`thunderbolt-ibverbs` creates an InfiniBand verbs device over generic USB4/Thunderbolt DMA rings. The project explicitly warns that it is buggy, insecure, and not for production. Its primary path requires Linux 6.14 or newer, or the project’s patched kernel. Source: [THUNDERBOLT-IBVERBS-76BA39B](sources.md#thunderbolt-ibverbs-76ba39b).

Host module load:

```bash
sudo modprobe thunderbolt_ibverbs   profile=linux_perf   bind_services=1   allocate_rings=1   start_rings=1   negotiate_native=1   enable_tunnels=1   register_verbs=1
```

Container exposure:

```bash
docker run --rm -it   --device=/dev/infiniband   --cap-add=IPC_LOCK   --ulimit memlock=-1   IMAGE
```

The kernel module remains on the host; the container needs the project’s libibverbs provider. Do not use this driver on an untrusted cable, peripheral, host, or physical environment.

## IOMMU warning

A community compute profile reports performance gains from `amd_iommu=off`. USB4 and experimental RDMA are DMA-capable transports, so disabling IOMMU changes the security boundary. This wiki does not combine that tuning with the USB4/RDMA profile. Benchmark `iommu=pt` or default policy before considering isolation-reducing settings.

## Network validation matrix

| Path | Production classification | Kernel component | User space | Test |
|---|---|---|---|---|
| ThunderboltIP/TCP | Upstream kernel component | `thunderbolt_net` | `iproute2`, `iperf3` | ping, iperf3, llama RPC |
| RXE over thunderbolt0 | Standard software RDMA over IP, performance varies | `rdma_rxe` + `thunderbolt_net` | `rdma-core` | `rping`, `ib_write_bw` |
| Native thunderbolt-ibverbs | Research only | `thunderbolt_ibverbs` | custom provider | project smoke and perftest |
