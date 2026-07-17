# Recipe: static USB4/Thunderbolt IP link

**Classification:** upstream Linux kernel component  
**Kernel:** `CONFIG_USB4_NET` / `thunderbolt_net`  
**Sources:** [LINUX-THUNDERBOLT-DOC](../sources.md#linux-thunderbolt-doc), [LINUX-USB4-NET-KCONFIG](../sources.md#linux-usb4-net-kconfig)

## Verify support

```bash
zgrep -E 'CONFIG_USB4=|CONFIG_USB4_NET=|CONFIG_INET=' /proc/config.gz 2>/dev/null \
  || grep -E 'CONFIG_USB4=|CONFIG_USB4_NET=|CONFIG_INET=' "/boot/config-$(uname -r)"
modinfo thunderbolt_net
ls -l /sys/bus/thunderbolt/devices
```

Authorize devices through the distribution’s normal policy (`boltctl`) when required.

## Dry-run helper

Host A:

```bash
../../scripts/usb4-net-setup.sh --interface thunderbolt0 --address 192.168.44.1/30
../../scripts/usb4-net-setup.sh --apply --interface thunderbolt0 --address 192.168.44.1/30
```

Host B:

```bash
../../scripts/usb4-net-setup.sh --apply --interface thunderbolt0 --address 192.168.44.2/30
```

The helper uses `ip address replace`, does not alter the default route, and is dry-run unless `--apply` is supplied.

## Test

```bash
# Host A
iperf3 -s

# Host B
ping -c 20 192.168.44.1
iperf3 -c 192.168.44.1 -P 4 -t 30
iperf3 -c 192.168.44.1 -P 4 -t 30 -R
iperf3 -c 192.168.44.1 -P 4 -t 30 --bidir
```

Record cable model, negotiated USB4/Thunderbolt generation, topology, MTU, CPU power mode, and interface counters.

## Route an application explicitly

```bash
ip route get 192.168.44.1
ss -tnp
```

Bind llama.cpp RPC or another service to the `192.168.44.0/30` address so Ethernet is not selected by a lower metric.

## MTU

Start with MTU 1500. Increase only after both peers and the complete path pass packet-loss and throughput tests:

```bash
sudo ip link set thunderbolt0 mtu 9000
ping -M do -s 8972 -c 5 192.168.44.1
```

Do not assume jumbo frames improve latency-sensitive batch-1 inference.
