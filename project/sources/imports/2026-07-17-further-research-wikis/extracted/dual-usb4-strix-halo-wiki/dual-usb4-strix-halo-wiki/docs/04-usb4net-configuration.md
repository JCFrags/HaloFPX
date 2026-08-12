# USB4NET configuration

## Load and discover

```bash
sudo modprobe thunderbolt_net
ip -br link | grep -E 'thunderbolt|tb[01]'
```

Linux-to-Linux host networking is created through XDomain service discovery. It may take several seconds after cable insertion. Watch both the kernel and udev streams:

```bash
sudo dmesg -wH | grep -Ei 'thunderbolt|usb4|xdomain|nhi'
sudo udevadm monitor --kernel --udev --subsystem-match=thunderbolt --subsystem-match=net
```

## Reference address plan

| Path | Node A | Node B | Subnet |
|---|---|---|---|
| Cable 0 | `10.44.0.1/30` | `10.44.0.2/30` | `10.44.0.0/30` |
| Cable 1 | `10.44.1.1/30` | `10.44.1.2/30` | `10.44.1.0/30` |

Use different subnets. This produces deterministic connected routes and avoids ARP flux.

## Scripted setup

On node A:

```bash
sudo ROLE=A IF0=thunderbolt0 IF1=thunderbolt1 MTU=9000 \
  scripts/configure-links.sh
sudo ROLE=A IF0=thunderbolt0 IF1=thunderbolt1 \
  scripts/configure-policy-routing.sh
```

On node B:

```bash
sudo ROLE=B IF0=thunderbolt0 IF1=thunderbolt1 MTU=9000 \
  scripts/configure-links.sh
sudo ROLE=B IF0=thunderbolt0 IF1=thunderbolt1 \
  scripts/configure-policy-routing.sh
```

The scripts use `ip address replace` and do not flush unrelated addresses. Inspect with `DRY_RUN=1` first.

## Manual setup

Node A:

```bash
sudo ip link set thunderbolt0 up mtu 9000
sudo ip link set thunderbolt1 up mtu 9000
sudo ip address replace 10.44.0.1/30 dev thunderbolt0
sudo ip address replace 10.44.1.1/30 dev thunderbolt1
```

Node B:

```bash
sudo ip link set thunderbolt0 up mtu 9000
sudo ip link set thunderbolt1 up mtu 9000
sudo ip address replace 10.44.0.2/30 dev thunderbolt0
sudo ip address replace 10.44.1.2/30 dev thunderbolt1
```

Test each path explicitly:

```bash
# Node A
ping -I 10.44.0.1 -c 5 10.44.0.2
ping -I 10.44.1.1 -c 5 10.44.1.2
ip route get 10.44.0.2 from 10.44.0.1
ip route get 10.44.1.2 from 10.44.1.1
```

## Network manager ownership

Do not let two management systems race over the same interface. Choose one:

### NetworkManager manual/unmanaged mode

```bash
sudo nmcli device set thunderbolt0 managed no
sudo nmcli device set thunderbolt1 managed no
```

Or create explicit NetworkManager profiles with manual IPv4 addresses and no default route.

### systemd-networkd

Samples for both nodes are in `configs/networkd/`. Copy the files after substituting persistent interface names, then:

```bash
sudo systemctl enable --now systemd-networkd
networkctl status thunderbolt0
```

## Link health

```bash
for i in thunderbolt0 thunderbolt1; do
  ethtool "$i" || true
  ip -s link show dev "$i"
  cat /sys/class/net/$i/carrier
  cat /sys/class/net/$i/operstate
done
```

Capture counters before and after every test. `thunderbolt-net` exposes standard netdev error counters, including receive CRC errors accumulated by the driver.

## MTU selection

Start at 1500 for correctness, then test 9000 and larger values. Both ends of one path must match. The upstream driver permits a maximum MTU of 65522, but large IP packets are still divided into approximately 4 KiB USB4NET transport frames internally.

```bash
sudo ip link set dev thunderbolt0 mtu 65522
sudo ip link set dev thunderbolt1 mtu 65522
ping -I 10.44.0.1 -M do -s 65494 -c 3 10.44.0.2
```

For IPv4 ICMP, payload `MTU - 28` tests a full-MTU packet. Reduce the payload if options or encapsulation are present.

## Do not bridge by default

A Linux bridge merges Layer-2 domains but does not aggregate one end-to-end flow. Bridging both USB4 links can create loops unless STP or another loop-control mechanism is deliberately configured. It is not a substitute for routing, bonding, or MPTCP.
