# MTU, queues, and offloads

## Driver architecture relevant to performance

At Linux 7.1, the upstream `thunderbolt-net` source defines:

- one 256-entry TX ring and one 256-entry RX ring per netdev;
- one NAPI instance per netdev;
- 4 KiB USB4NET frames with a maximum data payload of approximately 4084 bytes;
- a maximum network packet size of 64 KiB and netdev MTU range 68–65522;
- SG, TSO, GRO, IPv4 checksum, IPv6 checksum, and high-DMA feature flags.

The netdev is therefore a single-queue device in the Linux networking sense. Two USB4 links create two driver instances, each with its own rings/NAPI context, but one interface does not expose RSS-style multiple queue pairs.

## Inspect the running driver

```bash
for i in thunderbolt0 thunderbolt1; do
  echo "### $i"
  ethtool -i "$i" || true
  ethtool -k "$i" || true
  ethtool -l "$i" || true
  find /sys/class/net/$i/queues -maxdepth 1 -mindepth 1 -printf '%f\n'
done
```

Expected queue layout is typically `rx-0` and `tx-0`. A distro backport or future kernel can differ; runtime output wins.

## MTU test matrix

Test at least:

```text
1500, 9000, 16384, 32768, 65522
```

For each MTU:

1. Set both ends of both links.
2. Verify with `ping -M do`.
3. Run isolated and simultaneous throughput.
4. Record CPU, softirq, retransmission, and error counters.
5. Repeat the application workload.

Large MTUs can reduce packets per second and protocol overhead, but USB4NET still fragments data into 4 KiB tunneled frames. Benefits therefore come mostly from fewer skbs/IP/TCP headers and better segmentation/coalescing behavior, not from one USB4 frame carrying a 64 KiB packet.

## TSO/GSO/GRO interpretation

The driver accepts large TSO/GSO skbs and splits them into USB4NET frames itself. On receive it uses GRO/NAPI to present coalesced traffic to the stack. Do not infer hardware checksum or segmentation offload equivalent to a conventional NIC; some work is implemented in the driver and CPU.

Controlled offload comparison:

```bash
sudo ethtool -K thunderbolt0 gro off gso off tso off
# benchmark, then restore
sudo ethtool -K thunderbolt0 gro on gso on tso on
```

Only change features reported as supported. Offload-off tests help attribute CPU cost but are not normal production settings.

## RPS

Receive Packet Steering can move protocol processing above the interrupt handler to other CPUs. It works on single-queue devices, but adds inter-processor interrupts and cache movement.

```bash
cat /sys/class/net/thunderbolt0/queues/rx-0/rps_cpus
# Example bitmap only; calculate for the intended CPU set
printf '0000000e\n' | sudo tee /sys/class/net/thunderbolt0/queues/rx-0/rps_cpus
```

Use RPS only when the interrupting CPU or softirq context is saturated. Keep one flow on one CPU to avoid reordering; RPS flow hashing does this by design.

## XPS

XPS selects among transmit queues. With one TX queue, there is no queue choice, so XPS configuration has no material queue-selection effect. CPU pinning can still affect where the application builds skbs and where completions are processed.

## qdisc and backlog

```bash
tc -s qdisc show dev thunderbolt0
tc -s qdisc show dev thunderbolt1
sysctl net.core.netdev_max_backlog
cat /proc/net/softnet_stat
```

Queue drops or `time_squeeze` growth indicate host processing pressure rather than link-layer loss.

## Busy polling and threaded NAPI

Modern kernels expose NAPI controls, including threaded NAPI on supporting netdevs. These are advanced experiments, not baseline requirements:

```bash
cat /sys/class/net/thunderbolt0/threaded 2>/dev/null || true
# sudo sh -c 'echo 1 > /sys/class/net/thunderbolt0/threaded'
```

If enabled, pin the NAPI thread near the corresponding interrupt and application CPU. Re-run latency and CPU measurements; threaded processing can help isolation but also add scheduling overhead.

## Queue proof

Record the following before claiming per-link queue independence:

```bash
find /sys/class/net/thunderbolt0/queues -maxdepth 2 -type f -print -exec cat {} \; 2>/dev/null
find /sys/class/net/thunderbolt1/queues -maxdepth 2 -type f -print -exec cat {} \; 2>/dev/null
cat /proc/interrupts | grep -Ei 'thunderbolt|nhi|usb4'
```

Source inspection proves one ring/NAPI object per netdev in the recorded kernel; target captures prove how a distribution kernel and the actual controller expose interrupts.
