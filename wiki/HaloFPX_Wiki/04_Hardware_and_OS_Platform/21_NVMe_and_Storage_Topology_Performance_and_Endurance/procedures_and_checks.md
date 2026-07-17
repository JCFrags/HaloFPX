---
section_id: "21"
title: "Storage procedures and checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: ["nvme-cli", "smartmontools", "fio"]
  hardware_revisions: ["two project Strix Halo nodes"]
related_sections: ["18", "22", "73", "77", "80"]
---

# Storage procedures and checks

## Prerequisites and safety

Run on each node into a timestamped evidence directory. Inventory is read-only; some NVMe log commands require root. **Never point fio at `/dev/nvme*` or an existing model/cache file.** File tests consume capacity, generate writes, and may affect service latency. Stop if the target path, free space, or workload ownership is uncertain.

## 1. Read-only inventory

```bash
date --iso-8601=seconds
uname -a
cat /etc/os-release
lsblk -e7 -o NAME,PATH,MODEL,SERIAL,REV,TRAN,SIZE,LOG-SEC,PHY-SEC,ROTA,FSTYPE,UUID,MOUNTPOINTS
findmnt --json
sudo nvme list -v -o json
sudo nvme list-subsys -o json
sudo smartctl --scan-open
```

For every controller/namespace, capture without eliding output:

```bash
sudo nvme id-ctrl /dev/nvme0 -o json
sudo nvme id-ns /dev/nvme0n1 -o json
sudo nvme smart-log /dev/nvme0 -o json
sudo nvme error-log /dev/nvme0 -o json
sudo nvme fw-log /dev/nvme0 -o json
sudo smartctl -x -j /dev/nvme0
sudo lspci -Dnnvv -s <PCI_ADDRESS>
grep . /sys/block/nvme0n1/queue/* 2>/dev/null
find /sys/block/nvme0n1/mq -maxdepth 2 -type f -readable -print -exec cat {} \; 2>/dev/null
```

Record the exact SSD datasheet and warranty after the model/part number is known. Hash evidence files with `sha256sum`.

## 2. Capacity allocation

```bash
df -B1 --output=source,fstype,size,used,avail,pcent,target <CACHE_MOUNT>
sudo du -x -B1 --max-depth=2 <MODEL_ROOT> <CACHE_ROOT>
sudo find <MODEL_ROOT> <CACHE_ROOT> -xdev -type f -printf '%s\t%p\n' | sort -n
```

Record reserved blocks/quotas, swap placement, snapshots, compression, encryption, and mount options. Do not infer usable headroom from nominal drive capacity.

## 3. Bounded file-based baseline

Create a dedicated test directory on the target filesystem. Choose `size` that preserves the approved free-space floor. Example read/write job (destructive only to `halofpx-fio.bin`):

```ini
[global]
filename=halofpx-fio.bin
size=16G
direct=1
ioengine=io_uring
time_based=1
runtime=1800
ramp_time=120
group_reporting=1
output-format=json+

[seq-read-1m]
rw=read
bs=1M
iodepth=16

[rand-read-4k]
stonewall
rw=randread
bs=4k
iodepth=32
```

Precondition the test file explicitly before a read test. Record fio version and job SHA-256. Run separate aligned sequential read/write, random read, mixed metadata, and sustained writeback jobs; do not average them together. Capture SMART and temperature before, during, and after.

## 4. Concurrent-load matrix

Repeat the identical storage job under: idle; model resident/no decode; steady single-node decode; two-node decode plus fabric traffic; cache writeback/GC. Record inference tokens/s and latency alongside fio latency percentiles, CPU, interrupts, I/O pressure (`iostat -x 1`, `pidstat -d 1`, PSI), temperature, and clocks.

## 5. Power-loss behavior

**[RECOMMENDATION]** First test process kill, then clean reboot, then controlled external power interruption only with user approval and disposable cache data. Never interrupt firmware update or raw-device write. Assertions after reboot:

1. filesystem mounts without uncorrected error;
2. committed generation checksum and manifest validate;
3. incomplete generation is ignored;
4. previous generation remains readable or the entry is a safe miss;
5. SMART/error logs and journal are preserved.

No result is `[MEASURED]` until raw data, environment, exact command/job, timestamps, and pass/fail assertions are linked.
