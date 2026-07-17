# Topology and interface enumeration

## Expected Linux exposure

The upstream USB4 documentation states that Linux creates one virtual Ethernet interface per Thunderbolt/USB4 port for USB4NET. Typical names are `thunderbolt0`, `thunderbolt1`, and so on. The numeric suffix is assigned by discovery order and must not be treated as a stable physical-port label.

AMD lists two native 40 Gbit/s USB4 ports for Ryzen AI Max+ 395. Some Strix Halo Linux systems have been observed with two AMD USB4 Host Router PCI functions, but a product's board layout, BIOS, firmware, retimers, and connector routing remain decisive. Capture the actual topology on both nodes.

## Same peer over two cables: the connection-manager gate

Linux has two relevant connection-manager paths, and they do not treat a second route to the same peer identically:

- With the Linux software connection manager, XDomain discovery is initiated per route. The XDomain object name is derived from the local domain index and route, and incoming control traffic is matched by route.
- In the firmware/ICM connection-manager path, an XDomain-connect event first searches the **current USB4 domain** by remote UUID. If the same UUID is already present at another route, the old XDomain is removed before the new one is added.
- The lookup is scoped to one `struct tb` / `domainX`. Consequently, two separate NHI/controller domains avoid the same-domain UUID replacement case. They will usually also expose different local/remote domain UUIDs.

This produces a hard preflight rule:

> For a robust two-link deployment between the same two hosts, require two simultaneously present USB4NET interfaces and prefer two distinct `domainX` objects and NHI PCI BDFs on **both** hosts. A board with two connectors under one firmware-managed USB4 domain may expose only one route to the peer at a time.

On USB4 systems where ACPI grants native control, the driver selects the Linux software connection manager. Otherwise it tries the firmware/ICM path first and only falls back to software control. There is no portable userspace knob that turns an arbitrary firmware-managed topology into two independent domains; board firmware and ACPI ownership are part of the topology.

Useful evidence:

```bash
journalctl -k -b | grep -Ei 'thunderbolt|usb4|xdomain|ICM|native|connection manager'
find /sys/bus/thunderbolt/devices -maxdepth 2 -type f \
  \( -name unique_id -o -name security -o -name iommu_dma_protection \) \
  -print -exec cat {} \;
```

If the second cable makes the first XDomain/netdev disappear, stop: the two paths are not concurrently exposed. Do not attempt bonding or MPTCP until this layer passes.

## Capture first

```bash
sudo scripts/capture-topology.sh node-a-before
sudo scripts/capture-topology.sh node-b-before
```

The script records kernel configuration, modules, netdev-to-sysfs mappings, USB4 domains and services, PCI topology, NUMA information, IRQs, routes, MPTCP state, and link counters.

## Manual enumeration

```bash
uname -r
ls -l /sys/bus/thunderbolt/devices
ip -br link
ip -br address
ls -ld /sys/class/net/thunderbolt* 2>/dev/null

for ifc in /sys/class/net/thunderbolt*; do
    [ -e "$ifc" ] || continue
    i=${ifc##*/}
    printf '\n### %s\n' "$i"
    readlink -f "$ifc/device"
    udevadm info -q path -p "$ifc"
    ethtool -i "$i" || true
    ethtool "$i" || true
    ethtool -k "$i" || true
    ethtool -l "$i" || true
    ip -s -d link show dev "$i"
done
```

## Map each netdev to an XDomain, domain, and PCI function

A typical path resembles:

```text
/sys/devices/pci0000:00/.../0000:c8:00.5/domain0/0-0/0-1/0-1.0/net/thunderbolt0
```

Interpretation:

- `0000:c8:00.5` — candidate NHI PCI function.
- `domain0` — USB4 domain.
- `0-1` — remote XDomain/router.
- `0-1.0` — service instance used by `thunderbolt-net`.
- `thunderbolt0` — Ethernet interface.

Use real paths rather than assuming this exact layout:

```bash
for i in thunderbolt0 thunderbolt1; do
    p=$(readlink -f /sys/class/net/$i/device)
    echo "$i -> $p"
    echo "$p" | grep -oE 'domain[0-9]+' | tail -1
    echo "$p" | grep -oE '[0-9a-fA-F]{4}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}\.[0-7]' | tail -1
 done
```

Confirm the PCI functions and hierarchy:

```bash
lspci -Dnnk | grep -A4 -Ei 'USB4|Thunderbolt|Host Router'
lspci -tv
for bdf in 0000:c8:00.5 0000:c8:00.6; do
    [ -e /sys/bus/pci/devices/$bdf ] || continue
    printf '%s numa_node=' "$bdf"
    cat /sys/bus/pci/devices/$bdf/numa_node
    readlink -f /sys/bus/pci/devices/$bdf/iommu_group || true
done
```

## Link speed and lane evidence

The Thunderbolt sysfs ABI can expose `rx_speed`, `tx_speed`, `rx_lanes`, and `tx_lanes` on applicable router/XDomain objects. Record all candidates because exact placement can vary:

```bash
find /sys/bus/thunderbolt/devices -maxdepth 3 -type f \
  \( -name rx_speed -o -name tx_speed -o -name rx_lanes -o -name tx_lanes \
     -o -name link_speed -o -name link_width \) \
  -print -exec cat {} \;
```

`ethtool thunderboltN` on Linux 7.0+ can report the driver's view of aggregate link speed and duplex. Treat this as negotiated-link evidence, not a guarantee of TCP throughput.

## Stable naming

Use a udev rule based on the service device path or a systemd `.link` file. Avoid rules based only on the generated MAC address until it is confirmed stable across kernel/firmware updates.

Example inspection:

```bash
udevadm info --attribute-walk /sys/class/net/thunderbolt0
udevadm test-builtin net_id /sys/class/net/thunderbolt0 2>&1 | less
```

The supplied scripts accept arbitrary interface names, so persistent renaming is optional. At minimum, save the path-to-cable mapping in the topology capture.

## Physical port mapping procedure

1. Disconnect both cables.
2. Capture the empty topology.
3. Connect only cable 0 to the intended connector pair.
4. Record the new XDomain, service, netdev, domain, PCI BDF, and lane/speed attributes.
5. Disconnect cable 0 and verify removal.
6. Repeat for cable 1.
7. Connect both and verify both objects coexist.

This procedure is more reliable than connector labels or interface numbering.

## Signs of shared topology

- Both netdev paths contain the same `domainX` and same PCI BDF.
- Both cables traverse one USB4 hub or dock with one upstream link.
- Unplugging one cable removes or resets both interfaces.
- Simultaneous traffic is capped near one isolated-link result while neither CPU nor memory bandwidth is saturated.
- IRQ and counter activity for both interfaces maps to one shared controller context.

A shared domain does not make dual-link use impossible; it means the correct claim is “two physical links under one host controller,” not “two independent host-controller paths.”
