# HaloFPX Target Machines

This page is the current routing authority for the physical systems on which
HaloFPX performance is judged. It does not replace dated raw observations or
historical experiment records.

Last live read-only observation: `2026-08-12T20:33:55Z` through
`2026-08-12T20:49:20Z`. The retained receipt is
[`../docs/halofpx/evidence/2026-08-12-strix-halo-live-authority/README.md`](../docs/halofpx/evidence/2026-08-12-strix-halo-live-authority/README.md).

## Physical target

| Host | Hardware | Installed OS | Accelerator | Current production role |
|---|---|---|---|---|
| `nimo-1` | Nimo Direct MME3L, Ryzen AI MAX+ 395, 133,623,508,992 host-visible bytes (about 124.45 GiB) | CachyOS rolling, kernel `7.1.3-1-cachyos` | Radeon 8060S, `gfx1151` | coordinator and LAN API on port 8081 |
| `nimo-2` | Nimo Direct MME3L, Ryzen AI MAX+ 395, 133,623,500,800 host-visible bytes (about 124.45 GiB) | CachyOS rolling, kernel `7.1.3-1-cachyos` | Radeon 8060S, `gfx1151` | RPC worker on port 50052 |

Both nodes reported ROCm 7.2.4-family packages, Mesa/RADV 26.1.4, and
linux-firmware 20260622. The observed `/etc/os-release` omitted a numeric
`VERSION_ID`; the kernel, package, firmware, boot, source, binary, and model
identities must be captured with every promoted measurement.

Ubuntu is an AMD vendor-support and portability/control lane. It is not the
installed target operating system. `MAINCOMPUTER`, the local Windows 11 PC, is
for source control, orchestration, documentation, and bounded CPU checks; it
cannot establish target performance.

## Product target and runtime vocabulary

HaloFPX is a llama.cpp-derived inference engine specialized for
ROCmFPX-family GGUF model-weight artifacts on these machines. It is intended to
remain model-architecture-general within the architectures and ROCmFPX tensor
types that this tree can convert, load, execute, and qualify. MiniMax is the
largest stress/capacity fixture, not the model-specific optimization target.

ROCmFPX names a model-weight serialization and quantization family. HIP/ROCm
and Vulkan are accelerated execution backends; CPU is the correctness
reference; RPC is the dual-node transport/proxy. Runtime K/V-cache types are a
separate choice from the model's weight format.

## Current always-on deployment

The live service is a conventional MiniMax M2.7 `UD-Q6_K_XL` comparison and
rollback deployment. The nimo-1 coordinator reports upstream llama.cpp commit
`8f114a9b573b69035299f9b924047f53c1e22c7e`; the RPC worker exposes no build
version interface, so its recorded executable digest is the current identity
boundary rather than proof of that source commit. This is not a deployed
HaloFPX or ROCmFPX performance result.

| Host | System unit | Observed executable | SHA-256 at observation |
|---|---|---|---|
| `nimo-1` | `minimax-m27-q6-server.service` | `/opt/llm-usb4-cluster/llama/llama-server` | `d62ab220a4743a347461c958ce99a701e7ed21a938d9ab033334d9fb77fabbdb` |
| `nimo-2` | `minimax-m27-rpc-worker.service` | `/opt/llm-usb4-cluster/llama/ggml-rpc-server` | `cf0f39231fdab6b30254959edbb8de0c36cde2312cf4ee6761cfc27a3267bf63` |

Both units were active with `NRestarts=0`. At observation time, root-volume
free space was approximately 30.5 GB on nimo-1 and 87.1 GB on nimo-2. Capacity
is volatile and must be rechecked before staging builds, models, or evidence.

The 2026-07-17 ROCmFP4 experiments used the opposite role assignment because
the stress model was resident on nimo-2. Historical roles remain valid for
those experiments. Never infer a current role from the hostname.

## Fresh-PC continuation

The Git repository restores source, Wiki, decisions, tests, scripts, and
sanitized evidence:

```powershell
gh auth status
gh auth setup-git
git clone https://github.com/JCFrags/HaloFPX.git
Set-Location HaloFPX
git switch main
git fsck --full
py -3.12 -m venv .venv
.\.venv\Scripts\python.exe -m pip install PyYAML==6.0.3
.\.venv\Scripts\python.exe -B project/research/prompts/tools/generate_wiki_manifest.py project/wiki/HaloFPX_Wiki --check
.\.venv\Scripts\python.exe -B project/research/prompts/tools/validate_wiki.py project/wiki/HaloFPX_Wiki
.\.venv\Scripts\python.exe -B -m unittest discover -s project/research/prompts/tools -p "test_validate_wiki.py"
.\.venv\Scripts\python.exe -B project/project-management/documentation/validate_documentation.py
```

This assumes Git, GitHub CLI, Python 3.12, and PowerShell 7 are installed. The
repository is private, so GitHub authentication must already authorize it.

Ordinary development needs only the clone. Optional large historical evidence
from the immutable release is more than 23.3 GB and needs additional
reconstruction headroom. Follow [`../ARTIFACTS.md`](../ARTIFACTS.md) for its
manifest-first, two-phase recovery. The direct download/verification commands
are:

```powershell
gh release download evidence-2026-08-12 --repo JCFrags/HaloFPX `
  --dir .\halofpx-release --skip-existing
pwsh -NoProfile -File scripts/verify-publication-assets.ps1 `
  -AssetDirectory .\halofpx-release `
  -ManifestPath docs/publication/release-manifest.json
```

SSH credentials, private keys, and `known_hosts` entries are intentionally not
stored in Git. On a newly authorized PC, restore the credentials outside the
repository, then establish an equivalent SSH configuration:

```text
Host nimo-1
    HostName 192.168.40.11
    User connorb
    IdentityFile <operator-key-outside-repository>
    IdentitiesOnly yes

Host nimo-2
    HostName 192.168.40.12
    User connorb
    IdentityFile <operator-key-outside-repository>
    IdentitiesOnly yes
```

The Windows control PC trust store reported these ED25519 fingerprints on
2026-08-12:

| Host/address | ED25519 SHA-256 fingerprint |
|---|---|
| `nimo-1` / `192.168.40.11` | `SHA256:rOQQA0dAirWpqwKyGVkvG8V9k4q8sD7CUPqTEmoSnRA` |
| `nimo-2` / `192.168.40.12` | `SHA256:CEL+oTdkod6Mj4DZJqjSaLndofrMnYWq94lA3GK0+ls` |

Treat these as a repository-carried continuity record, not sufficient proof of
a replacement host. Before accepting a key on a new PC, compare its fingerprint
with this record and confirm it through an independent trusted channel or
physical console. Investigate any mismatch; never bypass it with
`StrictHostKeyChecking=no`.

The hostnames, addresses, and service paths are internal operational data.
Keep this repository private or redact and re-review them before any public
release.

Before any target change, perform a bounded read-only check:

```powershell
$opts = @('-o','BatchMode=yes','-o','StrictHostKeyChecking=yes',
  '-o','UpdateHostKeys=no','-o','ConnectTimeout=10','-o','ConnectionAttempts=1')
ssh @opts nimo-1 "hostname; uname -a; cat /etc/os-release"
ssh @opts nimo-2 "hostname; uname -a; cat /etc/os-release"
ssh @opts nimo-1 "systemctl --system show minimax-m27-q6-server.service -p ActiveState -p SubState -p MainPID -p InvocationID -p NRestarts -p ExecStart -p FragmentPath"
ssh @opts nimo-2 "systemctl --system show minimax-m27-rpc-worker.service -p ActiveState -p SubState -p MainPID -p InvocationID -p NRestarts -p ExecStart -p FragmentPath"
ssh @opts nimo-1 "ss -H -ltnp; curl -fsS http://127.0.0.1:8081/health; readlink /proc/`$(systemctl show -p MainPID --value minimax-m27-q6-server.service)/exe"
ssh @opts nimo-2 "ss -H -ltnp; readlink /proc/`$(systemctl show -p MainPID --value minimax-m27-rpc-worker.service)/exe"
```

GitHub alone cannot restore SSH private keys, verified host fingerprints, live
service state, locally licensed or very large model files, or independently
managed Agent Harness material. It also does not yet preserve a bootable
CachyOS image/package-repository snapshot, bootloader/kernel command line,
network/MPTCP/firewall configuration, current systemd unit and launcher files,
service accounts/permissions, `/opt/llm-usb4-cluster`, or cache contents.
Therefore a new control PC can continue development, but a wiped target cannot
yet be rebuilt perfectly from GitHub alone. [`../ARTIFACTS.md`](../ARTIFACTS.md)
records these external dependencies and the immutable release boundary.

## Build and experiment boundary

- Build current source independently on both CachyOS nodes with
  `scripts/build-halofpx-primary-matched.sh`; do not copy a target binary
  between nodes without proving its runtime-library resolution.
- Hash the source commit, build configuration, and both coordinator and worker
  binaries for every distributed A/B condition.
- Use isolated ports and disposable paths for experiments. A meaningful large
  ROCmFPX trial requires a controlled maintenance window because the always-on
  deployment consumes most unified memory.
- Recover the production worker first, verify port 50052, then recover the
  coordinator and require HTTP health before closing a transition.
- No service mutation is authorized merely by reading this page. Follow the
  current Project Lead decision and use a controller with bounded rollback.
