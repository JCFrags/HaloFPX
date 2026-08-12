# Strix Halo live authority — 2026-08-12

Claim scope: normalized read-only observations collected from the saved
OpenSSH aliases `nimo-1` and `nimo-2` on 2026-08-12 between 20:33:55Z and
20:49:20Z. No service, file, package, model, or host configuration was changed.
This is a sanitized authority receipt, not raw stdout.

## Normalization and retention boundary

The operator copied the named fields from the read-only command results into
the two normalized records and removed unrelated output. Full `/props` output
was deliberately excluded because it contains the complete chat template.
Raw stdout/stderr from this bounded audit was not promoted into the repository
or retained as durable project authority; these files therefore support the
listed observations but do not permit independent re-normalization from Git
alone. `SHA256SUMS` binds the sanitized files as published. Future promoted
measurements must retain raw machine-readable records whenever the output can
be stored safely.

## Current target platform

Both AMD Strix Halo targets reported:

- CachyOS Linux (`ID=cachyos`, `BUILD_ID=rolling`);
- kernel and package `linux-cachyos 7.1.3-1`;
- `rocm-core 7.2.4-1.1`, `rocm-hip-runtime 7.2.4-1`,
  `hip-runtime-amd 7.2.4-1.1`, and `rocminfo 7.2.4-1.1`;
- `mesa 3:26.1.4-1` and `vulkan-radeon 3:26.1.4-1`;
- `linux-firmware 1:20260622-1`;
- AMD Ryzen AI MAX+ 395 with Radeon 8060S, `gfx1151`;
- Nimo Direct MME3L hardware with AMI BIOS `3.05`, dated 2025-10-11; and
- host-visible memory of 133,623,508,992 bytes on nimo-1 and
  133,623,500,800 bytes on nimo-2.

The observed CachyOS `/etc/os-release` identifies a rolling build and omits
`VERSION_ID`. The exact machine authority is therefore the OS identity plus
the recorded kernel, packages, boot identity, hardware, and BIOS.

## Current roles and deployment

| Host | Current role | Service | Process | Listener |
|---|---|---|---:|---|
| `nimo-1` | coordinator and LAN API | `minimax-m27-q6-server.service` | PID `3027112`, InvocationID `e6da1fe637144cb394119959c0e88736`, `NRestarts=0` | `0.0.0.0:8081`, HTTP health 200 |
| `nimo-2` | RPC worker | `minimax-m27-rpc-worker.service` | PID `2148915`, InvocationID `3480c89086e04d5d80060366c5c7ab7f`, `NRestarts=0` | `0.0.0.0:50052` |

The nimo-1 server reports build `b1-8f114a9b`, and its source checkout reports
upstream llama.cpp commit `8f114a9b573b69035299f9b924047f53c1e22c7e`.
The worker does not expose a version flag; its same-build origin is an
**[INFERENCE]** from the retained production authority, not a fresh executable
self-report. The active model is the conventional MiniMax M2.7 UD-Q6_K_XL
GGUF. This is the standard production/control/rollback baseline, not proof that
HaloFPX or a ROCmFPX-format artifact is currently deployed.

The active executable SHA-256 values were:

- nimo-1 `llama-server`:
  `d62ab220a4743a347461c958ce99a701e7ed21a938d9ab033334d9fb77fabbdb`;
- nimo-2 `ggml-rpc-server`:
  `cf0f39231fdab6b30254959edbb8de0c36cde2312cf4ee6761cfc27a3267bf63`.

Both executables are dynamically linked. These hashes identify the executable
files only, not every loaded library or the complete runtime environment.

The observed process identifiers and unit state match the 2026-07-29 Project
Lead authority. This receipt supersedes only the statement that production was
not rechecked on 2026-08-12. It does not alter historical deployments.

## Role-history boundary

The 2026-07-17 ROCmFP4 capture used nimo-1 as worker and nimo-2 as
coordinator. The active UD-Q6 deployment has the opposite assignment. Machine
identity, operating system, and role assignment are separate facts. Historical
experiment records must keep their original roles; current operations must use
the live authority above.

## Storage observation

At observation time the root Btrfs volume reported about 30.5 GB free on
nimo-1 and 87.1 GB free on nimo-2. These are volatile capacity observations,
not quotas or durable reserves.

## Commands

The audit used read-only commands only:

```text
hostname
uname -srm
cat /etc/os-release
cat /proc/cmdline
hostnamectl
free -b
lspci -nn
rocminfo
pacman -Q
systemctl --system show
ss -ltnp
lsblk
uptime -s
readlink /proc/<pid>/exe
sha256sum /proc/<pid>/exe
tr '\0' ' ' </proc/<pid>/cmdline
git -C /opt/llm-usb4-cluster/llama.cpp rev-parse HEAD
df -B1
```

## Claim labels

- **[MEASURED]** The OS, kernel, package, hardware, process, listener, binary,
  role, and storage observations above are scoped to this timestamp and the
  exact commands.
- **[MEASURED]** Both target machines reported CachyOS; Ubuntu was not the
  installed target OS in this receipt.
- **[MEASURED]** The active always-on service is the conventional UD-Q6
  comparison deployment, not HaloFPX/ROCmFPX.
- **[OPEN]** No performance result is created by this inventory.
