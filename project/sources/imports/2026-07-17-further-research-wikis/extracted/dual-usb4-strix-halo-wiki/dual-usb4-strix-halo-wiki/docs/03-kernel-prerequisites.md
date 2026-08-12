# Current kernel prerequisites

## Snapshot versions

As of 2026-07-17, kernel.org listed Linux 7.1.3 as the current stable release, Linux 7.2-rc3 as mainline, and Linux 6.18.38 as a long-term release.

| Kernel line | USB4NET | MPTCP | Bonding status for `thunderbolt-net` | USB4STREAM | Recommendation |
|---|---:|---:|---|---:|---|
| 6.18 LTS | Yes | Yes; includes `laminar` endpoint support | Driver predates the Linux 7.0 MAC/link-setting additions; active-backup may work, aggregation modes need case-by-case validation | No | Viable USB4NET+MPTCP baseline when 7.x is unavailable |
| 7.0 | Yes | Yes | Added live MAC-change and link-setting support intended to allow bonding; 7.0 is EOL at snapshot | No | Feature boundary, not a deployment target |
| 7.1.3 stable | Yes | Yes | Current improved bonding hooks | No | Recommended stable USB4NET+MPTCP baseline |
| 7.2-rc3 mainline | Yes | Yes | Current | Yes, `thunderbolt-stream` | Experimental USB4STREAM evaluation only |

## Same-peer dual-link prerequisite

Kernel version alone cannot create two independent links. Before transport configuration, verify that both cables coexist as separate XDomains/netdevs. Linux 7.1 source shows a material connection-manager distinction: software-CM scanning is route keyed, whereas the firmware/ICM event path replaces an existing same-UUID XDomain at another route within the same USB4 domain. Two distinct `domainX`/NHI instances on both nodes are therefore the preferred hardware prerequisite.

USB4-native ACPI ownership selects the software CM; otherwise Linux probes firmware/ICM first. Treat connection-manager ownership as firmware/platform state, not a tunable promised by `CONFIG_USB4`.

## Required kernel configuration

### USB4NET path

```text
CONFIG_PCI=y
CONFIG_USB4=y or m
CONFIG_INET=y
CONFIG_USB4_NET=y or m
```

The module names are `thunderbolt` and `thunderbolt_net` (`modprobe` also accepts hyphenated spelling).

### MPTCP path

```text
CONFIG_MPTCP=y
CONFIG_INET_MPTCP_DIAG=y       # strongly recommended for ss diagnostics
CONFIG_MPTCP_IPV6=y            # optional; llama.cpp RPC transport is currently IPv4-only
CONFIG_IP_ROUTE_MULTIPATH=y    # only needed for ECMP experiments
```

### Bonding and CPU steering

```text
CONFIG_BONDING=y or m
CONFIG_RPS=y
CONFIG_XPS=y
```

### USB4STREAM path

```text
CONFIG_USB4=y or m
CONFIG_CONFIGFS_FS=y or m
CONFIG_USB4_CONFIGFS=y or m
CONFIG_USB4_STREAM=y or m
```

`CONFIG_USB4_STREAM` builds the `thunderbolt_stream` module. It is absent from Linux 7.1's upstream Kconfig and appears in the Linux 7.2 development tree.

## Verify a running distribution kernel

```bash
cfg=/boot/config-$(uname -r)
if [ ! -r "$cfg" ] && [ -r /proc/config.gz ]; then
    zcat /proc/config.gz > /tmp/kernel.config
    cfg=/tmp/kernel.config
fi

grep -E '^(CONFIG_(USB4|USB4_NET|USB4_CONFIGFS|USB4_STREAM|CONFIGFS_FS|MPTCP|MPTCP_IPV6|INET_MPTCP_DIAG|BONDING|RPS|XPS|IP_ROUTE_MULTIPATH))=' "$cfg"
```

Module checks:

```bash
modinfo thunderbolt
modinfo thunderbolt_net
modprobe thunderbolt_net
lsmod | grep -E '^thunderbolt(_net|_stream)?\b'

# Linux 7.2+ stream experiment
modinfo thunderbolt_stream
mountpoint /sys/kernel/config || sudo mount -t configfs none /sys/kernel/config
```

## Userspace prerequisites

Minimum useful tools:

```text
iproute2 with `ip mptcp`
ethtool
pciutils
udev
iperf3
iputils-ping
nftables
util-linux (lscpu, taskset)
numactl
sysstat (mpstat, pidstat)
perf
tcpdump
jq
mptcpize / mptcpd package where available
```

Check versions:

```bash
ip -Version
ip mptcp help 2>&1 | grep -E 'endpoint|laminar'
ss -V
iperf3 --version
ethtool --version
```

BusyBox `ip` is not sufficient for MPTCP endpoint management. The reference pairing also requires an iproute2 build that recognizes the Linux 6.18-era `laminar` endpoint flag; the setup script fails closed when it is absent.

## Firmware and BIOS prerequisites

- Install current OEM BIOS, USB4 firmware, retimer firmware, and Linux firmware packages.
- Check `fwupdmgr get-devices` and `fwupdmgr get-updates` where supported.
- Prefer an IOMMU-enabled configuration.
- For a dedicated host-to-host data fabric, a BIOS `nopcie` or USB-only security policy is preferable when it still permits XDomain host networking.
- Disable aggressive USB4/PCI runtime power management only as a controlled diagnostic; do not make it the first tuning step.

## Compatibility gate

Before performance work, require all of the following:

```bash
# two netdevs
ls /sys/class/net/thunderbolt0 /sys/class/net/thunderbolt1

# expected modules
lsmod | grep thunderbolt

# MPTCP available
sysctl net.mptcp.enabled 2>/dev/null || true
ip mptcp limits

# no obvious controller faults
dmesg --level=err,warn | grep -Ei 'thunderbolt|usb4|nhi|iommu|pcie' || true
```

A distro backport may make feature availability differ from the nominal upstream version. Source and runtime capability checks take precedence over version-number assumptions.
