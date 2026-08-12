# Capture commands and evidence boundaries

Commands were executed non-interactively through `ssh -o BatchMode=yes nimo-1` and `nimo-2`. The capture used these read-only command families:

| Area | Commands |
|---|---|
| Identity and OS | `hostnamectl`, `uname -a`, `/etc/os-release`, `/proc/cmdline`, DMI sysfs fields |
| CPU and memory | `lscpu`, `numactl --hardware`, `free`, `swapon`, selected `/proc/meminfo` fields |
| PCI and accelerator | `lspci -Dnnk`, `rocminfo`, `rocm-smi`, `hipconfig`, `/dev/kfd`, `/dev/dri`, amdgpu module parameters |
| Storage | `lsblk`, `findmnt`, `df`, `btrfs filesystem usage`, bounded `du`, `smartctl -x` |
| Network and USB4 | `ip link/address/route/rule`, `ethtool`, USB4 sysfs, `lsusb -t`, `ip mptcp`, `ss -M -tin` |
| Diagnostic latency | `ping -c 5` over `10.44.0.0/30` and `10.44.0.4/30` in both directions |
| Software | `pacman -Q`, compiler/tool version commands, current kernel config, module and device-node checks |
| Runtime | constrained `systemctl show`, `ps`, `ss -ltn`, loopback-only health/model API GETs |
| Source provenance | `git rev-parse`, `describe`, `status`, `remote`, `log`, executable `sha256sum` |
| Cache audit | file counts/sizes plus source inspection at deployed commit `4860505e...` |

Interpretation rules:

- Package presence is not workload correctness.
- A healthy HTTP endpoint is readiness evidence only.
- A Git commit plus executable digest identifies the deployed baseline but does not prove reproducible build flags by itself.
- `rocm-smi` GTT accounting, process RSS, cgroup memory, and physical available memory are distinct views.
- `ethtool` reports a logical 40,000 Mb/s interface rate while USB4 sysfs reports two 20.0 Gb/s lanes per direction. Neither is achieved application goodput.
- The RPC server `--cache` is a model-tensor transfer cache, not a persistent attention-KV/session cache.

