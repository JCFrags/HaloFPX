# Troubleshooting

## No `thunderboltN` interface

```bash
sudo modprobe thunderbolt
sudo modprobe thunderbolt_net
ls -l /sys/bus/thunderbolt/devices
dmesg | grep -Ei 'thunderbolt|usb4|xdomain|nhi'
```

Check cable capability, connector choice, BIOS USB4 enablement, host-to-host support, firmware, and whether the peer loaded the driver. Some passive USB-C cables support charging/USB 2 only; use a certified 40 Gbit/s cable.

## Only one interface when both cables are connected

- Connect each cable alone and map the physical connectors.
- Check whether both ports route through one hub or one upstream path.
- Inspect `lspci` for one versus two NHI functions.
- Watch for firmware resets or bandwidth warnings.
- Update BIOS/firmware.
- Check whether the product exposes one native port plus one non-USB4 USB-C port.

AMD's SoC port count is not a board guarantee.

## Interface numbering swaps

Use sysfs/udev physical paths and persistent naming. Never hard-code `thunderbolt0` as “left port” without a recorded mapping.

## Ping works on one link but not the other

```bash
ip addr show
ip route show table all
ip rule show
ip neigh show
sysctl net.ipv4.conf.all.rp_filter
ip route get REMOTE from LOCAL
sudo tcpdump -ni IFACE arp or icmp
```

Distinct subnets should create direct routes automatically. Check address reversal, mask `/30`, and manager conflicts.

## MPTCP remains one subflow

```bash
ip mptcp endpoint show
ip mptcp limits
ss -Mani
sudo ss -tani
ip -ts mptcp monitor
sysctl net.mptcp.path_manager
```

Common causes:

- application still created a plain TCP socket;
- only one side ran through `mptcpize`;
- `add_addr_accepted` is zero;
- endpoint has the wrong `signal`/`subflow` role;
- strict `rp_filter` drops MP_JOIN traffic;
- policy routing selects the wrong interface;
- firewall blocks the additional address/port;
- NetworkManager and `mptcpd` overwrite endpoints;
- no application data has yet made the MPTCP connection fully established enough for path-manager actions.

## MPTCP creates cross-paired subflows

Remove `fullmesh`, use `laminar`, and verify source-specific routes. Filter invalid cross-pairs if necessary:

```bash
sudo nft add rule inet dual_usb4 output ip saddr 10.44.0.1 ip daddr 10.44.1.2 reject
sudo nft add rule inet dual_usb4 output ip saddr 10.44.1.1 ip daddr 10.44.0.2 reject
```

Use the provided nftables file as a starting point and adapt addresses/roles.

## Bond refuses a member or LACP stays down

- Require Linux 7.0+ or a distro backport of the `thunderbolt-net` MAC/link-setting changes.
- Ensure no IP address remains on a member.
- Ensure member MTUs match.
- Inspect `ethtool`, `ip -d link`, and `/proc/net/bonding/bond0`.
- For 802.3ad, both peers must run compatible LACP and show one aggregator.
- Begin with active-backup to isolate capability from aggregation behavior.

## `balance-rr` underperforms

Look for reordering and retransmission:

```bash
nstat -az | grep -Ei 'Retrans|Reorder|DSACK'
ss -ti dst PEER
```

Return to separate interfaces plus MPTCP. Do not tune `tcp_reordering` until the baseline and packet traces establish that reordering is the cause.

## Large MTU fails

- Set exactly the same MTU at both ends.
- Check bond master and members if bonding.
- Remove VPN/encapsulation or account for its overhead.
- Use `ping -M do` with payload `MTU-28` for direct IPv4.
- Check route cache and PMTU diagnostics.
- Fall back to 9000 or 1500 and compare.

## Throughput plateaus near one link

Determine which ceiling is active:

```bash
mpstat -P ALL 1
perf stat -a -- sleep 30
cat /proc/net/softnet_stat
ip -s link show
ss -ti
```

Possible causes:

- both ports share one controller/domain;
- one CPU handles both NAPI/IRQ paths;
- one application thread is saturated;
- memory bandwidth or SoC fabric is saturated;
- MPTCP scheduler uses only the lower-RTT path;
- one link negotiates fewer lanes/lower speed;
- one route/subflow is not carrying payload;
- thermal or power policy throttles the platform.

## USB4STREAM ConfigFS path missing

```bash
uname -r
modinfo thunderbolt_stream
zgrep CONFIG_USB4_STREAM /proc/config.gz 2>/dev/null || true
mount | grep configfs
ls /sys/kernel/config/thunderbolt/stream
```

Linux 7.1 does not contain the upstream driver. A distro kernel may backport it, but verify the Kconfig/module and ABI.

## USB4STREAM blocks on open

The driver can wait for the peer stream/service to appear. Configure both ends, open the read side first, and use `O_NONBLOCK` while debugging. Verify in/out HopIDs and XDomain service names.

## llama.cpp RPC reachable on cable 0 but MPTCP join fails on cable 1

A listener bound only to `10.44.0.2` may not accept joins targeting `10.44.1.2`. Use a wildcard listener inside a restricted namespace/firewall, or modify the listener architecture. Confirm the server socket itself is MPTCP.

## Soft-RoCE device exists but llama.cpp remains TCP

Enable RPC debug, verify the verbs GID matches the local TCP address, and inspect `ibv_devices`, `rdma link`, and GID tables. Even if negotiation succeeds, compare end-to-end CPU and throughput; RXE is not hardware offload.

## Connecting cable 1 removes cable 0

This is below IP and MPTCP. Capture `journalctl -k -b`, the Thunderbolt sysfs tree, and the netdev-to-domain map while connecting one cable at a time. Linux's firmware/ICM path de-duplicates a remote UUID within one USB4 domain and can replace the old route. A robust fix is a board/port pairing that gives each cable a different NHI and `domainX` on both hosts. A newer kernel cannot manufacture controller independence that the firmware/ACPI topology does not expose.

Do not hide the symptom with interface renaming or NetworkManager profiles. The pass condition is two XDomains/services/netdevs present at the same time.
