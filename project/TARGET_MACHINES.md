# HaloFPX Target Machines

This page is the current routing authority for the physical systems on which
HaloFPX performance is judged. It does not replace dated raw observations or
historical experiment records.

Last broad live read-only inventory: `2026-08-12T20:33:55Z` through
`2026-08-12T20:49:20Z`. The retained receipt is
[`../docs/halofpx/evidence/2026-08-12-strix-halo-live-authority/README.md`](../docs/halofpx/evidence/2026-08-12-strix-halo-live-authority/README.md).

Latest retained production recovery: a real 5-prompt-token plus
1-generated-token request completed at `2026-08-12T19:16:23.124104-07:00`,
and the final read-only identity/health capture began at about
`2026-08-12T19:28:39-07:00`. The retained
[`incident and recovery receipt`](../docs/halofpx/evidence/2026-08-12-target-hmm-oom-incident/README.md)
supersedes the earlier service identities and restart counters. It did not
re-audit the platform, packages, firmware, unit or launcher contents,
executables/libraries, model, cache contents, network policy, or performance.
[Issue #41](https://github.com/JCFrags/HaloFPX/issues/41) is the P0
target-ownership prerequisite for any target build, quantization, disposable
inference, or benchmark.

The older `2026-08-12T23:06:08Z`
[`health-only receipt`](../docs/halofpx/evidence/2026-08-12-strix-halo-health-recheck/README.md)
is historical before-state for the later local-time incident. Its zero-restart
PIDs, InvocationIDs, listeners, kernel string, and coordinator health remain
valid only for that observation window; they are not current production
authority.

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

The earlier broad inventory identified the live service as a conventional
MiniMax M2.7 `UD-Q6_K_XL` comparison and rollback deployment. The recovered
units retain the same systemd unit names and completed a real request, but the
incident receipt did not re-audit their unit/launcher contents or model
identity. During that broad inventory, the nimo-1 coordinator reported
upstream llama.cpp commit
`8f114a9b573b69035299f9b924047f53c1e22c7e`; the RPC worker exposed no build
version interface. The executable digests below are evidence from that
pre-incident inventory only. Neither executable nor its loaded libraries was
rehashed after the later restarts, so those digests must not be treated as the
current recovered binary identity. This is not a deployed HaloFPX or ROCmFPX
performance result.

| Host | System unit | Executable observed in broad inventory | SHA-256 at pre-incident observation |
|---|---|---|---|
| `nimo-1` | `minimax-m27-q6-server.service` | `/opt/llm-usb4-cluster/llama/llama-server` | `d62ab220a4743a347461c958ce99a701e7ed21a938d9ab033334d9fb77fabbdb` |
| `nimo-2` | `minimax-m27-rpc-worker.service` | `/opt/llm-usb4-cluster/llama/ggml-rpc-server` | `cf0f39231fdab6b30254959edbb8de0c36cde2312cf4ee6761cfc27a3267bf63` |

At the broad-inventory observation, both units were active with `NRestarts=0`
and root-volume free space was approximately 30.5 GB on nimo-1 and 87.1 GB on
nimo-2. Capacity is volatile and must be rechecked before staging builds,
models, or evidence. The later health-only recheck retained those service
identities, but both observations are now historical before-state.

The post-incident recovered authority is:

| Host | Recovered unit authority | Retained recovery proof |
|---|---|---|
| `nimo-1` | PID `3113343`; InvocationID `0656332b63a140eab7214627baa43253`; `NRestarts=1`; active/running | local `/health` returned `{"status":"ok"}`; the real 5-prompt-token plus 1-generated-token request completed |
| `nimo-2` | PID `2248760`; InvocationID `d15fe49610274e77bd9a3d84a0b791a5`; `NRestarts=1`; active/running | the same real two-rank request completed only after the worker and coordinator had recovered |

These are volatile measured identities from the incident receipt, not a
promise that the units have remained unchanged. Recheck both before any target
operation. Health alone is not distributed readiness after either rank changes
identity; require exact two-rank authority plus a real minimal inference.

The 2026-07-17 ROCmFP4 experiments used the opposite role assignment because
the stress model was resident on nimo-2. Historical roles remain valid for
those experiments. Never infer a current role from the hostname.

## Fresh-PC continuation

The Git repository restores source, Wiki, decisions, tests, scripts, and
sanitized evidence. [Issue #11](https://github.com/JCFrags/HaloFPX/issues/11)
owns the pinned bootstrap/tooling prerequisite for
[issue #2](https://github.com/JCFrags/HaloFPX/issues/2); completing that
prerequisite alone does not close the end-to-end clean-PC acceptance. Issue #2
and a full fresh-PC `PASS` remain `[OPEN]` until a retained run completes every
gate.

A machine intended to run the full recovery must have PowerShell `7.2` or
newer; Python `3.12` with `venv`; Git; an authenticated GitHub CLI with
`gh release verify`; CMake; Ninja; `cc`; `c++`; `sha256sum`; and `tar`. The
chosen external recovery-work volume must have at least `53,687,091,200` free
bytes before recovery starts; `60 GiB` (`64,424,509,440` bytes) or more is
recommended. Use the fail-closed command preflight in the root
[`HANDOFF.md`](../HANDOFF.md#first-clean-clone-checks), and retain its tool
versions and exact free-byte result in the recovery receipt.

After that preflight, bootstrap and validate the clone with:

```powershell
$env:PYTHONUTF8 = '1'
$env:PYTHONIOENCODING = 'utf-8'
gh auth status --hostname github.com
gh release verify --help | Out-Null
gh auth setup-git
git clone https://github.com/JCFrags/HaloFPX.git
Set-Location HaloFPX
git switch main
git fetch --tags --force origin
if ((git rev-parse --is-shallow-repository).Trim() -eq 'true') {
    git fetch --unshallow --tags --force origin
}
git merge --ff-only origin/main
git fsck --full
python3.12 -X utf8 -m venv .venv
./.venv/bin/python -m pip install --requirement requirements/requirements-halofpx-validation.txt
./.venv/bin/python -X utf8 -B project/research/prompts/tools/generate_wiki_manifest.py project/wiki/HaloFPX_Wiki --check
./.venv/bin/python -X utf8 -B project/research/prompts/tools/validate_wiki.py project/wiki/HaloFPX_Wiki
./.venv/bin/python -X utf8 -B -m unittest discover -s project/research/prompts/tools -p "test_*.py"
./.venv/bin/python -X utf8 -B -m unittest tests/test_halofpx_strix_ab.py tests/test_halofpx_strix_ab_cachyos.py -v
./.venv/bin/python -X utf8 -B tests/test_materialize_rocmfpx_fixture.py -v
./.venv/bin/python -X utf8 -B project/project-management/documentation/validate_documentation.py
```

The UTF-8 environment is required on a Windows control checkout so a CP1252
console cannot fail while printing Unicode documentation diagnostics.

Issue #2's acceptance lane is one clean Linux environment. A Windows control
checkout may substitute `py -3.12` and
`.\.venv\Scripts\python.exe -X utf8`, but that run does not satisfy issue #2
by itself.

The Wiki discovery pattern includes both validator and manifest-generator
tests. The model-general Strix A/B, CachyOS adapter, and fixture-materialization
suites are offline contracts; they neither contact the targets nor download
any model or release payload. The fixture suite currently contains 12 tests.
The repository is private, so GitHub
authentication must authorize it. Continue with the resumable metadata and
attestation command in the root
[`HANDOFF.md`](../HANDOFF.md#first-clean-clone-checks); its metadata-only result
does not replace the full issue #2 receipt.

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

Compare the result with the recovered identities above and retain a new dated
receipt if either PID, InvocationID, or restart count differs. Do not infer
that the pre-incident executable digests still apply; hash the current
executables and loaded libraries when binary identity is required.

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
  coordinator. Require exact identities, coordinator HTTP health, and a real
  minimal two-rank inference before closing a transition.
- No service mutation is authorized merely by reading this page. Follow the
  current Project Lead decision and use a controller with bounded rollback.
