# Capture commands

All commands were invoked from the repository root on the Windows control PC
through the saved `nimo-1` and `nimo-2` OpenSSH aliases with:

```text
-o BatchMode=yes
-o StrictHostKeyChecking=yes
-o UpdateHostKeys=no
-o ConnectTimeout=10
-o ConnectionAttempts=1
```

The normalized records retain selected non-secret output from:

```bash
date -u +%Y-%m-%dT%H:%M:%SZ
hostname
uname -srm
grep -E '^(NAME|PRETTY_NAME|ID|BUILD_ID|VERSION_ID)=' /etc/os-release
cat /sys/class/dmi/id/sys_vendor
cat /sys/class/dmi/id/product_name
cat /sys/class/dmi/id/bios_vendor
cat /sys/class/dmi/id/bios_version
cat /sys/class/dmi/id/bios_date
grep -m1 '^model name' /proc/cpuinfo
awk '/MemTotal/ { print $2 * 1024 }' /proc/meminfo
pacman -Q linux-cachyos linux-cachyos-headers rocm-core rocm-hip-runtime \
  hip-runtime-amd rocminfo mesa vulkan-radeon linux-firmware
rocminfo
systemctl --system show <unit> -p Id -p LoadState -p ActiveState \
  -p SubState -p MainPID -p InvocationID -p NRestarts \
  -p ExecMainStartTimestamp -p FragmentPath -p ExecStart
ss -H -ltnp
curl -fsS http://127.0.0.1:8081/health
curl -fsS http://127.0.0.1:8081/props
readlink /proc/<pid>/exe
sha256sum /proc/<pid>/exe
LD_LIBRARY_PATH=/opt/llm-usb4-cluster/llama /proc/<pid>/exe --version
git -C /opt/llm-usb4-cluster/llama.cpp rev-parse HEAD
df -B1 --output=source,fstype,size,used,avail,target /
uptime -s
```

The post-screen service check at 2026-08-12T20:51:00Z used:

```bash
# nimo-1
systemctl is-active minimax-m27-q6-server.service
systemctl show minimax-m27-q6-server.service -p NRestarts -p MainPID --value
curl -sS -o /dev/null -w '%{http_code}\n' http://127.0.0.1:8081/health

# nimo-2
systemctl is-active minimax-m27-rpc-worker.service
systemctl show minimax-m27-rpc-worker.service -p NRestarts -p MainPID --value
```

The RPC worker does not implement `--version`; its command returned the usage
text and no build identity. Raw `/props` output is intentionally not retained
because it contains the full chat template and adds no authority beyond the
normalized `build_info`, model alias, type, and path fields above.
