# CPU overhead and NUMA locality

## Do not assume a NUMA topology

Strix Halo is a chiplet-based SoC, but Linux may expose one NUMA node. A single NUMA node means `numactl` cannot express chiplet or cache locality as separate memory nodes. CPU/IRQ/cache placement can still matter.

Capture:

```bash
lscpu -e=CPU,NODE,SOCKET,CORE,CACHE,ONLINE
numactl --hardware
for n in /sys/devices/system/node/node*; do
  echo "$n: $(cat "$n/cpulist")"
done
```

## Map the controllers to NUMA nodes

After extracting each NHI BDF from the netdev sysfs path:

```bash
for bdf in 0000:c8:00.5 0000:c8:00.6; do
  [ -e /sys/bus/pci/devices/$bdf ] || continue
  echo "$bdf"
  cat /sys/bus/pci/devices/$bdf/numa_node
  cat /sys/bus/pci/devices/$bdf/local_cpulist 2>/dev/null || true
  cat /sys/bus/pci/devices/$bdf/local_cpus 2>/dev/null || true
done
```

`numa_node=-1` usually means no specific node association. Use empirical IRQ/core placement.

## Find and pin interrupts

```bash
grep -Ei 'thunderbolt|nhi|usb4' /proc/interrupts
# Stronger mapping when the NHI uses MSI/MSI-X:
find /sys/bus/pci/devices/0000:c8:00.5/msi_irqs -maxdepth 1 -mindepth 1 -printf '%f\n' 2>/dev/null
for irq in $(grep -Ei 'thunderbolt|nhi|usb4' /proc/interrupts | cut -d: -f1 | tr -d ' '); do
  echo "IRQ $irq affinity=$(cat /proc/irq/$irq/smp_affinity_list)"
done
```

Set an affinity only after mapping the IRQ to a specific controller/link:

```bash
echo 4-5 | sudo tee /proc/irq/IRQ0/smp_affinity_list
echo 6-7 | sudo tee /proc/irq/IRQ1/smp_affinity_list
```

`irqbalance` may overwrite manual settings. Either configure an IRQ-balance policy or stop it during a controlled benchmark.

## Application placement

```bash
taskset -c 8-15 mptcpize run ./bin/llama-cli ...
numactl --physcpubind=8-15 --membind=0 ./bin/ggml-rpc-server ...
```

Use `--membind` only when the node exists and the model/runtime can tolerate strict allocation. For an integrated GPU/APU, CPU memory policy, GPU memory registration, and UMA behavior can interact; measure model load and inference, not only network microbenchmarks.

## CPU accounting

During a 30-second transfer:

```bash
mpstat -P ALL 1
pidstat -t -p $(pidof ggml-rpc-server) 1
sudo perf stat -a -e cycles,instructions,cache-misses,context-switches,cpu-migrations,irq:irq_handler_entry -- sleep 30
cat /proc/softirqs
cat /proc/net/softnet_stat
```

For attribution:

```bash
sudo perf record -a -g -- sleep 30
sudo perf report
```

Likely stacks include `thunderbolt_net`, NAPI, GRO, TCP/MPTCP, socket copies, and the RPC serialization/deserialization path.

## Per-byte CPU metric

Report CPU cost as:

```text
CPU-seconds / GiB transferred
cycles / byte
instructions / byte
```

This allows fair comparison among MTUs, one versus two links, TCP versus MPTCP, and USB4NET versus USB4STREAM. Wall-clock CPU percentage alone is misleading when throughput changes.

## Expected tradeoffs

- Two links can consume two IRQ/NAPI contexts and more memory bandwidth.
- MPTCP adds meta-socket scheduling, sequence mapping, and possible reinjection work.
- Larger MTU/GSO can lower packet-processing overhead.
- RPS can reduce one-CPU saturation but increase IPIs and cache misses.
- USB4STREAM removes IP/TCP processing but currently performs userspace copies and serializes I/O through a device mutex.
- Soft-RoCE adds software verbs/RDMA processing on top of the netdev path and is unlikely to lower CPU without a specific implementation advantage.

## Locality experiment matrix

| Test | IRQ placement | App placement | RPS | Purpose |
|---|---|---|---|---|
| Baseline | irqbalance | scheduler default | off | Real-world default |
| Same-core | link IRQ and socket thread near one core set | same set | off | Cache locality |
| Split links | cable 0 and cable 1 IRQs on different core groups | app spans both | off | Controller parallelism |
| RPS | IRQ core excluded; nearby cores enabled | app nearby | on | Relieve single RX CPU |
| Remote | intentionally distant CPU group | distant | off | Bound locality penalty |

Change one factor per run and repeat enough times to quantify variance.
