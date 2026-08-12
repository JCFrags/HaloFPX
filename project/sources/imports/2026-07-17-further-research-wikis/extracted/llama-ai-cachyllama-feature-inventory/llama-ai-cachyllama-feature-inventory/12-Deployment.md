# Deployment

The parent repository supplies practical build, model provisioning, service, GPU detection, and APU memory helpers. Its GPL license and several host-specific or destructive behaviors require a clear separation from an MIT ROCmFPX integration.

| Decision | Count |
|---|---:|
| RETAIN | 1 |
| REDESIGN | 6 |
| REJECT | 2 |

<a id="f-085"></a>
### F-085 — Multi-backend build helper

**Decision:** `REDESIGN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Builds CachyLlama for Vulkan, ROCm/HIP, or Metal with backend-specific CMake settings.

**Implementation.** `scripts/rebuild.sh`

**Dependencies.** CMake; compiler toolchain; Vulkan SDK or ROCm or Metal

**License.** GPL-3.0

**ROCmFPX overlap.** ROCmFPX already ships dedicated AMD/backend build scripts.

**Porting rationale.** Keep ROCmFPX build scripts; transplant only configuration knowledge through clean-room MIT-compatible changes.

**Risks / caveats.** Direct script copying would impose GPL obligations.

**Evidence.** [E-010](20-Evidence-Index.md#e-010)

<a id="f-086"></a>
### F-086 — systemd service unit

**Decision:** `REDESIGN` · **Portability:** `High` · **Maturity:** `M1` · **Confidence:** `High`

**Observed behavior.** Runs llama-server as a managed service with restart policy and hardening directives.

**Implementation.** `systemd/llama-server.service`

**Dependencies.** systemd; installed binary/model paths

**License.** GPL-3.0 parent repository

**ROCmFPX overlap.** ROCmFPX deployments may use systemd but paths/options differ.

**Porting rationale.** Generate a parameterized unit or packaging template from target-supported flags; validate with systemd-analyze.

**Risks / caveats.** Pinned unit has host-specific paths and option drift.

**Evidence.** [E-013](20-Evidence-Index.md#e-013)

<a id="f-087"></a>
### F-087 — Aggressive stale-process cleanup

**Decision:** `REJECT` · **Portability:** `High` · **Maturity:** `M1` · **Confidence:** `High`

**Observed behavior.** Force-kills existing llama-server processes and port holders before starting a new server.

**Implementation.** `llama-run.sh`

**Dependencies.** pkill; fuser/lsof or process inspection; shell privileges

**License.** GPL-3.0

**ROCmFPX overlap.** A shared ROCmFPX host may run multiple independent model services.

**Porting rationale.** Use PID files, systemd/container ownership, graceful shutdown, and a bounded escalation policy.

**Risks / caveats.** Can kill unrelated tenants or services.

**Evidence.** [E-009](20-Evidence-Index.md#e-009)

<a id="f-088"></a>
### F-088 — API key and TLS configuration

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M3` · **Confidence:** `High`

**Observed behavior.** Supports API-key authentication and server TLS certificate/key options.

**Implementation.** `common/arg.cpp`, `tools/server/server.cpp`

**Dependencies.** TLS library/backend; HTTP middleware; secret storage

**License.** MIT

**ROCmFPX overlap.** ROCmFPX inherits server authentication/TLS features.

**Porting rationale.** Keep target-native implementation and use authenticated principal context for cache tenant binding.

**Risks / caveats.** Static API keys need rotation and must not appear in presets/status output.

**Evidence.** [E-090](20-Evidence-Index.md#e-090), [E-103](20-Evidence-Index.md#e-103)

<a id="f-089"></a>
### F-089 — Model download orchestration

**Decision:** `REDESIGN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Selects local models or downloads Hugging Face GGUF files before launch.

**Implementation.** `llama-run.sh`

**Dependencies.** Hugging Face tooling; network; filesystem

**License.** GPL-3.0

**ROCmFPX overlap.** ROCmFPX models are distributed as GGUF variants and may be split.

**Porting rationale.** Provide a separate, checksummed provisioning command or documented workflow under compatible licensing.

**Risks / caveats.** Supply-chain integrity and partial downloads.

**Evidence.** [E-008](20-Evidence-Index.md#e-008)

<a id="f-090"></a>
### F-090 — GPU/APU hardware detection

**Decision:** `REDESIGN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Maps AMD PCI/device information to gfx targets, HSA overrides, memory tiers, and recommended profiles.

**Implementation.** `scripts/detect-gpu.sh`

**Dependencies.** lspci/sysfs; AMD device map; shell

**License.** GPL-3.0

**ROCmFPX overlap.** ROCmFPX already has device-specific build scripts but benefits from runtime guidance.

**Porting rationale.** Move stable detection into an MIT-compatible capability probe with explicit overrides and tests.

**Risks / caveats.** Device IDs and driver behavior evolve.

**Evidence.** [E-011](20-Evidence-Index.md#e-011)

<a id="f-091"></a>
### F-091 — APU GTT/TTM configuration helper

**Decision:** `REJECT` · **Portability:** `High` · **Maturity:** `M1` · **Confidence:** `High`

**Observed behavior.** Can use amd-smi or mutate systemd-boot/GRUB kernel parameters to expand GPU-accessible memory.

**Implementation.** `scripts/apply-ttm-kernel-params.sh`

**Dependencies.** root privileges; amd-smi; bootloader tools

**License.** GPL-3.0

**ROCmFPX overlap.** ROCmFPX documents UMA settings but should not own host boot configuration.

**Porting rationale.** Document validated host settings and provide a separate opt-in administrator tool, not part of the inference package.

**Risks / caveats.** Boot failure, over-allocation, portability, and privilege escalation.

**Evidence.** [E-012](20-Evidence-Index.md#e-012)

<a id="f-092"></a>
### F-092 — Layered component/runner configuration

**Decision:** `REDESIGN` · **Portability:** `High` · **Maturity:** `M1` · **Confidence:** `High`

**Observed behavior.** Component source defaults, server documentation, and parent profile overrides form three configuration layers.

**Implementation.** `common/common.h`, `tools/server/README.md`, `llama-run.sh`

**Dependencies.** CLI precedence; shell environment

**License.** Mixed: MIT component, GPL-3.0 parent

**ROCmFPX overlap.** ROCmFPX already has its own CLI defaults.

**Porting rationale.** Publish one generated schema and precedence table; make profile overrides explicit and machine-readable.

**Risks / caveats.** Current values conflict, including hot window, cold count, and checkpoint-step documentation.

**Evidence.** [E-006](20-Evidence-Index.md#e-006), [E-022](20-Evidence-Index.md#e-022), [E-024](20-Evidence-Index.md#e-024)

<a id="f-093"></a>
### F-093 — License-separated orchestration layer

**Decision:** `REDESIGN` · **Portability:** `High` · **Maturity:** `M3` · **Confidence:** `High`

**Observed behavior.** The inference component is MIT, while the parent launch/build/deployment layer is GPL-3.0 and its documentation has a separate content license.

**Implementation.** `LICENSE`, `README.md`, `CachyLlama/LICENSE`

**Dependencies.** license compliance; distribution packaging

**License.** Mixed

**ROCmFPX overlap.** ROCmFPX is MIT.

**Porting rationale.** Port MIT component code with notices; clean-room reimplement parent behavior or keep it as a separate GPL work.

**Risks / caveats.** Combining GPL parent code into the MIT target changes distribution obligations.

**Evidence.** [E-003](20-Evidence-Index.md#e-003), [E-004](20-Evidence-Index.md#e-004), [E-021](20-Evidence-Index.md#e-021)


