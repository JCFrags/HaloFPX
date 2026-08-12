# Evidence index

Every evidence record is pinned to an exact commit and source path. The machine-readable form is [`data/evidence.json`](data/evidence.json); the flattened file list is [`data/source-files.csv`](data/source-files.csv).

## Repository pins

- `fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722`
- `fewtarius/CachyLlama@6be745998f568e379ea197fcf827baec73ff9940`
- `charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394`

## `fewtarius/llama-ai`

<a id="e-001"></a>
### E-001 — `(commit)`

- **Commit:** [`1017f3dfdce3ca2b06aa9007b23295db3bb35722`](https://github.com/fewtarius/llama-ai/commit/1017f3dfdce3ca2b06aa9007b23295db3bb35722)
- **Source:** [`fewtarius/llama-ai / (commit)`](https://github.com/fewtarius/llama-ai/commit/1017f3dfdce3ca2b06aa9007b23295db3bb35722)
- **Locator:** commit metadata
- **Evidence type:** commit
- **Supports:** The assessed llama-ai revision is the exact parent commit pin.
- **Referenced by:** pin/method evidence only

<a id="e-002"></a>
### E-002 — `.gitmodules`

- **Commit:** [`1017f3dfdce3ca2b06aa9007b23295db3bb35722`](https://github.com/fewtarius/llama-ai/commit/1017f3dfdce3ca2b06aa9007b23295db3bb35722)
- **Source:** [`fewtarius/llama-ai / .gitmodules`](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/.gitmodules)
- **Locator:** submodule stanza for CachyLlama
- **Evidence type:** manifest
- **Supports:** llama-ai declares CachyLlama as a Git submodule and tracks its upstream repository.
- **Referenced by:** pin/method evidence only

<a id="e-003"></a>
### E-003 — `LICENSE`

- **Commit:** [`1017f3dfdce3ca2b06aa9007b23295db3bb35722`](https://github.com/fewtarius/llama-ai/commit/1017f3dfdce3ca2b06aa9007b23295db3bb35722)
- **Source:** [`fewtarius/llama-ai / LICENSE`](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/LICENSE)
- **Locator:** full license text
- **Evidence type:** license
- **Supports:** llama-ai is distributed under GNU GPL v3.
- **Referenced by:** `F-093`

<a id="e-004"></a>
### E-004 — `README.md`

- **Commit:** [`1017f3dfdce3ca2b06aa9007b23295db3bb35722`](https://github.com/fewtarius/llama-ai/commit/1017f3dfdce3ca2b06aa9007b23295db3bb35722)
- **Source:** [`fewtarius/llama-ai / README.md`](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/README.md)
- **Locator:** license/documentation section
- **Evidence type:** documentation
- **Supports:** The parent README distinguishes software licensing from documentation licensing.
- **Referenced by:** `F-093`

<a id="e-005"></a>
### E-005 — `llama-run.sh`

- **Commit:** [`1017f3dfdce3ca2b06aa9007b23295db3bb35722`](https://github.com/fewtarius/llama-ai/commit/1017f3dfdce3ca2b06aa9007b23295db3bb35722)
- **Source:** [`fewtarius/llama-ai / llama-run.sh`](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/llama-run.sh)
- **Locator:** default variables and command assembly
- **Evidence type:** source
- **Supports:** The runner sets model/server defaults, builds the llama-server command, and exposes profile-dependent cache options.
- **Referenced by:** `F-025`, `F-049`

<a id="e-006"></a>
### E-006 — `llama-run.sh`

- **Commit:** [`1017f3dfdce3ca2b06aa9007b23295db3bb35722`](https://github.com/fewtarius/llama-ai/commit/1017f3dfdce3ca2b06aa9007b23295db3bb35722)
- **Source:** [`fewtarius/llama-ai / llama-run.sh`](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/llama-run.sh)
- **Locator:** SSD cache defaults and --cache-ssd* arguments
- **Evidence type:** source
- **Supports:** The runner overrides CachyLlama SSD defaults, including hot window, checkpoint count, cold count, system prompt count, retention, and fsync behavior.
- **Referenced by:** `F-049`, `F-092`, `F-115`

<a id="e-007"></a>
### E-007 — `llama-run.sh`

- **Commit:** [`1017f3dfdce3ca2b06aa9007b23295db3bb35722`](https://github.com/fewtarius/llama-ai/commit/1017f3dfdce3ca2b06aa9007b23295db3bb35722)
- **Source:** [`fewtarius/llama-ai / llama-run.sh`](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/llama-run.sh)
- **Locator:** GGUF metadata inspection and model profile selection
- **Evidence type:** source
- **Supports:** The runner examines GGUF metadata to distinguish dense, MoE, and recurrent/SSM model profiles.
- **Referenced by:** `F-049`, `F-064`, `F-096`

<a id="e-008"></a>
### E-008 — `llama-run.sh`

- **Commit:** [`1017f3dfdce3ca2b06aa9007b23295db3bb35722`](https://github.com/fewtarius/llama-ai/commit/1017f3dfdce3ca2b06aa9007b23295db3bb35722)
- **Source:** [`fewtarius/llama-ai / llama-run.sh`](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/llama-run.sh)
- **Locator:** Hugging Face repository discovery and split download branches
- **Evidence type:** source
- **Supports:** The runner can resolve and download local or Hugging Face GGUF models, including split files.
- **Referenced by:** `F-063`, `F-089`

<a id="e-009"></a>
### E-009 — `llama-run.sh`

- **Commit:** [`1017f3dfdce3ca2b06aa9007b23295db3bb35722`](https://github.com/fewtarius/llama-ai/commit/1017f3dfdce3ca2b06aa9007b23295db3bb35722)
- **Source:** [`fewtarius/llama-ai / llama-run.sh`](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/llama-run.sh)
- **Locator:** cleanup_existing_servers / process termination branches
- **Evidence type:** source
- **Supports:** The runner forcefully removes existing llama-server instances and port holders before launch.
- **Referenced by:** `F-087`

**Caveat.** Operationally effective but too destructive for a shared service host.

<a id="e-010"></a>
### E-010 — `scripts/rebuild.sh`

- **Commit:** [`1017f3dfdce3ca2b06aa9007b23295db3bb35722`](https://github.com/fewtarius/llama-ai/commit/1017f3dfdce3ca2b06aa9007b23295db3bb35722)
- **Source:** [`fewtarius/llama-ai / scripts/rebuild.sh`](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/scripts/rebuild.sh)
- **Locator:** backend selection and CMake command construction
- **Evidence type:** source
- **Supports:** The build helper supports Vulkan, ROCm/HIP, and Metal-oriented builds and backend-specific CMake flags.
- **Referenced by:** `F-085`

<a id="e-011"></a>
### E-011 — `scripts/detect-gpu.sh`

- **Commit:** [`1017f3dfdce3ca2b06aa9007b23295db3bb35722`](https://github.com/fewtarius/llama-ai/commit/1017f3dfdce3ca2b06aa9007b23295db3bb35722)
- **Source:** [`fewtarius/llama-ai / scripts/detect-gpu.sh`](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/scripts/detect-gpu.sh)
- **Locator:** PCI mapping, gfx target, hardware tier, and memory guidance
- **Evidence type:** source
- **Supports:** Hardware detection recognizes Strix Halo gfx1151 and derives tier/profile variables.
- **Referenced by:** `F-064`, `F-090`, `F-094`

<a id="e-012"></a>
### E-012 — `scripts/apply-ttm-kernel-params.sh`

- **Commit:** [`1017f3dfdce3ca2b06aa9007b23295db3bb35722`](https://github.com/fewtarius/llama-ai/commit/1017f3dfdce3ca2b06aa9007b23295db3bb35722)
- **Source:** [`fewtarius/llama-ai / scripts/apply-ttm-kernel-params.sh`](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/scripts/apply-ttm-kernel-params.sh)
- **Locator:** amd-smi, systemd-boot, and GRUB mutation paths
- **Evidence type:** source
- **Supports:** The deployment helper can alter APU GTT/TTM settings at runtime or through bootloader configuration.
- **Referenced by:** `F-091`

**Caveat.** Privileged host mutation; unsuitable as an embedded library feature.

<a id="e-013"></a>
### E-013 — `systemd/llama-server.service`

- **Commit:** [`1017f3dfdce3ca2b06aa9007b23295db3bb35722`](https://github.com/fewtarius/llama-ai/commit/1017f3dfdce3ca2b06aa9007b23295db3bb35722)
- **Source:** [`fewtarius/llama-ai / systemd/llama-server.service`](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/systemd/llama-server.service)
- **Locator:** unit paths, restart policy, hardening directives, ExecStart
- **Evidence type:** source
- **Supports:** A systemd unit is supplied for automatic restart and service hardening.
- **Referenced by:** `F-086`

**Caveat.** The pinned unit contains host-specific paths and an option that has drifted from the runner.

<a id="e-014"></a>
### E-014 — `AGENTS.md`

- **Commit:** [`1017f3dfdce3ca2b06aa9007b23295db3bb35722`](https://github.com/fewtarius/llama-ai/commit/1017f3dfdce3ca2b06aa9007b23295db3bb35722)
- **Source:** [`fewtarius/llama-ai / AGENTS.md`](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/AGENTS.md)
- **Locator:** repository layout and workflow notes
- **Evidence type:** documentation
- **Supports:** The parent repository treats CachyLlama as the inference component and the shell layer as deployment orchestration.
- **Referenced by:** pin/method evidence only

<a id="e-117"></a>
### E-117 — `scripts/detect-gpu.sh`

- **Commit:** [`1017f3dfdce3ca2b06aa9007b23295db3bb35722`](https://github.com/fewtarius/llama-ai/commit/1017f3dfdce3ca2b06aa9007b23295db3bb35722)
- **Source:** [`fewtarius/llama-ai / scripts/detect-gpu.sh`](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/scripts/detect-gpu.sh)
- **Locator:** HSA_OVERRIDE_GFX_VERSION and Strix Halo profile variables
- **Evidence type:** source
- **Supports:** Parent deployment exports gfx1151-compatible HSA and UMA settings.
- **Referenced by:** `F-095`

<a id="e-118"></a>
### E-118 — `llama-run.sh`

- **Commit:** [`1017f3dfdce3ca2b06aa9007b23295db3bb35722`](https://github.com/fewtarius/llama-ai/commit/1017f3dfdce3ca2b06aa9007b23295db3bb35722)
- **Source:** [`fewtarius/llama-ai / llama-run.sh`](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/llama-run.sh)
- **Locator:** Strix Halo profile branch
- **Evidence type:** source
- **Supports:** The runner prefers large in-memory contexts and may disable SSD state caching for selected Halo dense/MoE profiles.
- **Referenced by:** `F-049`, `F-064`, `F-096`, `F-104`

<a id="e-119"></a>
### E-119 — `scripts/rebuild.sh`

- **Commit:** [`1017f3dfdce3ca2b06aa9007b23295db3bb35722`](https://github.com/fewtarius/llama-ai/commit/1017f3dfdce3ca2b06aa9007b23295db3bb35722)
- **Source:** [`fewtarius/llama-ai / scripts/rebuild.sh`](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/scripts/rebuild.sh)
- **Locator:** AMDGPU_TARGETS/gfx target and unified-memory options
- **Evidence type:** source
- **Supports:** The build helper emits ROCm/HIP target and unified-memory flags appropriate to AMD APUs.
- **Referenced by:** `F-095`

## `fewtarius/CachyLLama`

<a id="e-020"></a>
### E-020 — `(commit)`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / (commit)`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Locator:** commit metadata
- **Evidence type:** commit
- **Supports:** The assessed CachyLlama revision is the exact gitlink selected by the parent.
- **Referenced by:** pin/method evidence only

<a id="e-021"></a>
### E-021 — `LICENSE`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / LICENSE`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/LICENSE)
- **Locator:** full license text
- **Evidence type:** license
- **Supports:** CachyLlama source is MIT licensed.
- **Referenced by:** `F-093`

<a id="e-022"></a>
### E-022 — `common/common.h`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/common.h`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/common.h)
- **Locator:** common_params server/cache fields
- **Evidence type:** source
- **Supports:** Source defaults define SSD path, checkpoint count, hot/warm windows, cold limit, page size, conversation limit, system cache retention, and per-user concurrency.
- **Referenced by:** `F-027`, `F-034`, `F-039`, `F-048`, `F-092`

<a id="e-023"></a>
### E-023 — `common/arg.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/arg.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/arg.cpp)
- **Locator:** server cache and isolation argument registrations
- **Evidence type:** source
- **Supports:** CLI options and environment variables expose SSD cache, system cache, checkpoint, and per-user concurrency controls.
- **Referenced by:** `F-005`, `F-025`, `F-027`, `F-035`, `F-039`, `F-048`

<a id="e-024"></a>
### E-024 — `tools/server/README.md`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/README.md`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/README.md)
- **Locator:** server-specific parameters and user-isolation section
- **Evidence type:** documentation
- **Supports:** Server documentation lists prompt cache, checkpoint, metrics, slots, router, and user-isolation interfaces.
- **Referenced by:** `F-035`, `F-048`, `F-092`

**Caveat.** Several documented defaults or anonymous-cap statements conflict with source behavior.

<a id="e-025"></a>
### E-025 — `common/CMakeLists.txt`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/CMakeLists.txt`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/CMakeLists.txt)
- **Locator:** common library source list
- **Evidence type:** source
- **Supports:** The primary SSD cache and system-prompt cache units are linked into the common library.
- **Referenced by:** pin/method evidence only

<a id="e-026"></a>
### E-026 — `tools/server/CMakeLists.txt`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/CMakeLists.txt`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/CMakeLists.txt)
- **Locator:** llama-server source list
- **Evidence type:** source
- **Supports:** The server-specific page manager and SSD cache adapter are compiled into llama-server.
- **Referenced by:** pin/method evidence only

<a id="e-030"></a>
### E-030 — `common/kv-ssd-cache.h`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/kv-ssd-cache.h`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.h)
- **Locator:** kv_ssd_cache_config, metadata, stats, and public methods
- **Evidence type:** source
- **Supports:** The component defines hot/warm/cold tiers, persistent checkpoint metadata, target/draft/spec blobs, matching, continuation, turn, and prefetch APIs.
- **Referenced by:** `F-001`, `F-002`, `F-003`, `F-007`, `F-054`, `F-084`

<a id="e-031"></a>
### E-031 — `common/kv-ssd-cache.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/kv-ssd-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.cpp)
- **Locator:** init(), directory scan, index load, and checkpoint reconstruction
- **Evidence type:** source
- **Supports:** Initialization scans an existing directory and reconstructs checkpoint state, enabling process-restart persistence.
- **Referenced by:** `F-001`, `F-015`, `F-052`, `F-081`

<a id="e-032"></a>
### E-032 — `common/kv-ssd-cache.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/kv-ssd-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.cpp)
- **Locator:** read/write helpers and 64 MiB chunk loop
- **Evidence type:** source
- **Supports:** State files are transferred in bounded chunks and use platform I/O helpers.
- **Referenced by:** `F-004`

<a id="e-033"></a>
### E-033 — `common/kv-ssd-posix.h`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/kv-ssd-posix.h`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-posix.h)
- **Locator:** POSIX open/pread/pwrite/fsync/fadvise wrappers
- **Evidence type:** source
- **Supports:** The SSD implementation isolates POSIX-specific file and read-ahead operations behind a helper layer.
- **Referenced by:** `F-004`, `F-010`, `F-014`

<a id="e-034"></a>
### E-034 — `common/kv-ssd-cache.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/kv-ssd-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.cpp)
- **Locator:** checkpoint store path, optional fsync, failure cleanup
- **Evidence type:** source
- **Supports:** Checkpoint writes can fsync and delete incomplete files on failure, but write directly to final names.
- **Referenced by:** `F-005`, `F-013`

**Caveat.** No temp-file plus atomic-rename commit protocol was found.

<a id="e-035"></a>
### E-035 — `common/kv-ssd-cache.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/kv-ssd-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.cpp)
- **Locator:** find_match() longest-common-prefix ranking
- **Evidence type:** source
- **Supports:** Cache matching computes token LCP and ranks matches by prefix length with recency/token-count tie breakers.
- **Referenced by:** `F-021`

**Caveat.** Any positive prefix is eligible at this layer; stronger thresholds are applied by the server page manager.

<a id="e-036"></a>
### E-036 — `common/kv-ssd-cache.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/kv-ssd-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.cpp)
- **Locator:** find_continuation() and global directory scanner
- **Evidence type:** source
- **Supports:** Anonymous cache namespaces can be scanned for a continuation candidate based on stored token prefixes.
- **Referenced by:** `F-023`, `F-024`

<a id="e-037"></a>
### E-037 — `common/kv-ssd-cache.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/kv-ssd-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.cpp)
- **Locator:** tier transition, on_turn_complete(), and cold-ring eviction
- **Evidence type:** source
- **Supports:** Turn completion demotes older entries through hot/warm/cold tiers and enforces a cold-entry cap.
- **Referenced by:** `F-002`, `F-008`, `F-008`, `F-050`, `F-081`

<a id="e-038"></a>
### E-038 — `common/kv-ssd-cache.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/kv-ssd-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.cpp)
- **Locator:** compatibility hash check during load
- **Evidence type:** source
- **Supports:** Loads reject checkpoints whose model/cache compatibility fingerprint does not match.
- **Referenced by:** `F-006`, `F-055`, `F-065`

<a id="e-039"></a>
### E-039 — `common/kv-ssd-cache.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/kv-ssd-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.cpp)
- **Locator:** automatic hot/warm RAM budget calculation
- **Evidence type:** source
- **Supports:** Automatic tier budgets derive from reported free system RAM and an 85% allocation factor.
- **Referenced by:** `F-002`, `F-009`

**Caveat.** The Linux calculation uses sysinfo free RAM rather than MemAvailable/cgroup-aware pressure.

<a id="e-040"></a>
### E-040 — `common/kv-ssd-cache.h`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/kv-ssd-cache.h`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.h)
- **Locator:** format version and token-prefix metadata
- **Evidence type:** source
- **Supports:** Checkpoint metadata carries a format version, model compatibility value, and up to 4096 prefix tokens.
- **Referenced by:** `F-006`, `F-007`, `F-015`

<a id="e-041"></a>
### E-041 — `common/kv-ssd-cache.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/kv-ssd-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.cpp)
- **Locator:** permissions used while creating cache directories and files
- **Evidence type:** source
- **Supports:** The cache uses conventional directory/file permissions rather than an owner-only tenant store.
- **Referenced by:** `F-012`

**Caveat.** The observed 0755/0644 style is not sufficient for secrets-bearing multi-tenant persistence.

<a id="e-042"></a>
### E-042 — `common/kv-ssd-cache.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/kv-ssd-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.cpp)
- **Locator:** prefetch/read-ahead path
- **Evidence type:** source
- **Supports:** The cache can issue platform read-ahead hints before a likely restore.
- **Referenced by:** `F-010`, `F-026`

<a id="e-050"></a>
### E-050 — `common/kv-ssd-system-cache.h`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/kv-ssd-system-cache.h`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-system-cache.h)
- **Locator:** kv_ssd_system_cache_config, entry metadata, public API
- **Evidence type:** source
- **Supports:** A separate persistent system-prompt cache is defined with bounded entries and retention.
- **Referenced by:** `F-016`, `F-018`, `F-019`

<a id="e-051"></a>
### E-051 — `common/kv-ssd-system-cache.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/kv-ssd-system-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-system-cache.cpp)
- **Locator:** init scan and sys-{hash}.bin load
- **Evidence type:** source
- **Supports:** System-prompt entries are rediscovered from disk and loaded into RAM at startup.
- **Referenced by:** `F-017`, `F-052`

<a id="e-052"></a>
### E-052 — `common/kv-ssd-system-cache.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/kv-ssd-system-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-system-cache.cpp)
- **Locator:** save/load keying and compatibility checks
- **Evidence type:** source
- **Supports:** System-prompt state is keyed by a prompt hash and guarded by compatibility metadata.
- **Referenced by:** `F-016`, `F-019`, `F-055`

<a id="e-053"></a>
### E-053 — `common/kv-ssd-system-cache.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/kv-ssd-system-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-system-cache.cpp)
- **Locator:** expiration and LRU enforcement
- **Evidence type:** source
- **Supports:** Expired entries and least-recently-used entries are pruned according to age and count limits.
- **Referenced by:** `F-018`, `F-050`, `F-052`

<a id="e-054"></a>
### E-054 — `common/kv-ssd-system-cache.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/kv-ssd-system-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-system-cache.cpp)
- **Locator:** detect_system_prompt_boundary() token-text heuristic
- **Evidence type:** source
- **Supports:** System-prompt boundaries are inferred by decoding token text and searching for user/human/EOG markers.
- **Referenced by:** `F-020`

**Caveat.** The chat-template hint is not used to perform structural parsing; this is heuristic.

<a id="e-055"></a>
### E-055 — `tools/server/server-context.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-context.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context.cpp)
- **Locator:** system cache initialization and lookup/store integration
- **Evidence type:** source
- **Supports:** llama-server invokes the system cache during request processing.
- **Referenced by:** `F-016`

<a id="e-060"></a>
### E-060 — `tools/server/server-context-page-manager.h`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-context-page-manager.h`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context-page-manager.h)
- **Locator:** server_context_page_manager API and wrapper maps
- **Evidence type:** source
- **Supports:** The server page manager coordinates per-conversation and per-user persistent cache instances.
- **Referenced by:** `F-006`, `F-043`, `F-047`, `F-065`

<a id="e-061"></a>
### E-061 — `tools/server/server-context-page-manager.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-context-page-manager.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context-page-manager.cpp)
- **Locator:** get_or_create_cache(), anonymous and user namespace routing
- **Evidence type:** source
- **Supports:** Anonymous conversations and explicit users are routed to different directory namespaces.
- **Referenced by:** `F-011`, `F-042`

<a id="e-062"></a>
### E-062 — `tools/server/server-context-page-manager.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-context-page-manager.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context-page-manager.cpp)
- **Locator:** find_continuation_match() and 0.90 threshold
- **Evidence type:** source
- **Supports:** The server accepts cross-conversation continuation only when the stored prefix reaches the configured high similarity threshold.
- **Referenced by:** `F-022`, `F-023`, `F-026`

<a id="e-063"></a>
### E-063 — `tools/server/server-context-page-manager.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-context-page-manager.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context-page-manager.cpp)
- **Locator:** store_checkpoint/load_checkpoint target, draft, and spec state
- **Evidence type:** source
- **Supports:** Checkpoint records can restore target, draft, and speculative state into a destination slot.
- **Referenced by:** `F-003`, `F-029`, `F-030`, `F-120`

<a id="e-064"></a>
### E-064 — `tools/server/server-context-page-manager.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-context-page-manager.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context-page-manager.cpp)
- **Locator:** get_stats() and on_turn_complete() map iteration
- **Evidence type:** source
- **Supports:** The pinned aggregation and turn-completion paths iterate anonymous maps but omit the corresponding user maps.
- **Referenced by:** `F-043`, `F-044`, `F-054`, `F-084`

**Caveat.** This is an implementation gap found by source inspection.

<a id="e-065"></a>
### E-065 — `tools/server/server-context-page-manager.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-context-page-manager.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context-page-manager.cpp)
- **Locator:** max-conversation enforcement and oldest-directory deletion
- **Evidence type:** source
- **Supports:** The manager removes an old cache directory when its conversation limit is reached.
- **Referenced by:** `F-051`

**Caveat.** Anonymous and user collections are managed separately rather than under one shared quota.

<a id="e-066"></a>
### E-066 — `tools/server/server-context-ssd-cache.h`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-context-ssd-cache.h`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context-ssd-cache.h)
- **Locator:** server_context_ssd_cache adapter API
- **Evidence type:** source
- **Supports:** The server adapter converts llama sequence state into cache blobs and back.
- **Referenced by:** pin/method evidence only

<a id="e-067"></a>
### E-067 — `tools/server/server-context-ssd-cache.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-context-ssd-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context-ssd-cache.cpp)
- **Locator:** save/restore target, draft, and speculative components
- **Evidence type:** source
- **Supports:** The adapter serializes and restores target/draft/spec components and reports partial failures.
- **Referenced by:** `F-003`, `F-029`, `F-033`, `F-081`

<a id="e-068"></a>
### E-068 — `include/llama.h`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / include/llama.h`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/include/llama.h)
- **Locator:** llama_state_seq_*_ext and llama_memory_seq_rm_attn_only declarations
- **Evidence type:** source
- **Supports:** The component exposes extended per-sequence state serialization and attention-only removal primitives.
- **Referenced by:** `F-001`, `F-029`, `F-030`, `F-031`

<a id="e-069"></a>
### E-069 — `src/llama-memory-hybrid.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / src/llama-memory-hybrid.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/src/llama-memory-hybrid.cpp)
- **Locator:** attention-only sequence removal implementation
- **Evidence type:** source
- **Supports:** Hybrid/recurrent model restoration can clear attention state without discarding recurrent state.
- **Referenced by:** `F-031`

<a id="e-070"></a>
### E-070 — `(commit)`

- **Commit:** [`c8ead677a7fe42fb0a67e6e866fb254cc338e9fd`](https://github.com/fewtarius/CachyLLama/commit/c8ead677a7fe42fb0a67e6e866fb254cc338e9fd)
- **Source:** [`fewtarius/CachyLLama / (commit)`](https://github.com/fewtarius/CachyLLama/commit/c8ead677a7fe42fb0a67e6e866fb254cc338e9fd)
- **Locator:** cold continuation destination-sequence fix
- **Evidence type:** history
- **Supports:** A post-feature fix corrected cold continuation restoration into the selected destination sequence.
- **Referenced by:** `F-030`

**Caveat.** Recent corrective history is evidence of active maturation, not stability.

<a id="e-071"></a>
### E-071 — `tools/server/server-context.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-context.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context.cpp)
- **Locator:** slot fields and persistent-cache restore/store call sites
- **Evidence type:** source
- **Supports:** Slots track cold-start restore use, conversation hash, user identity, and checkpoint timing.
- **Referenced by:** `F-027`, `F-028`, `F-031`, `F-033`, `F-034`

<a id="e-072"></a>
### E-072 — `common/kv_page_manager.h`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/kv_page_manager.h`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv_page_manager.h)
- **Locator:** standalone page-manager public API and tier constants
- **Evidence type:** source
- **Supports:** A second asynchronous page-manager prototype exists outside the server integration.
- **Referenced by:** `F-056`

<a id="e-073"></a>
### E-073 — `common/kv_page_manager.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/kv_page_manager.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv_page_manager.cpp)
- **Locator:** prototype implementation
- **Evidence type:** source
- **Supports:** The standalone page manager has worker/writeback code but is separate from the active server path.
- **Referenced by:** `F-056`

**Caveat.** Its comments describe GiB-scale defaults while literals are MiB-scale.

<a id="e-074"></a>
### E-074 — `test_kv_page_manager.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / test_kv_page_manager.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/test_kv_page_manager.cpp)
- **Locator:** standalone test driver
- **Evidence type:** source
- **Supports:** A standalone test source exercises page-manager concepts.
- **Referenced by:** `F-056`

<a id="e-075"></a>
### E-075 — `common/CMakeLists.txt`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/CMakeLists.txt`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/CMakeLists.txt)
- **Locator:** absence of kv_page_manager.cpp from common sources
- **Evidence type:** source
- **Supports:** The standalone page-manager implementation is not linked into the common library.
- **Referenced by:** `F-056`

**Caveat.** Absence is established by comparing the file with the pinned source list.

<a id="e-080"></a>
### E-080 — `docs/development/user-isolation-design.md`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / docs/development/user-isolation-design.md`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/docs/development/user-isolation-design.md)
- **Locator:** request mapping, namespace, validation, concurrency, precedence
- **Evidence type:** design
- **Supports:** The design specifies request user identifiers, per-user namespaces, slot affinity, and concurrency limits.
- **Referenced by:** `F-011`, `F-038`, `F-040`, `F-042`

**Caveat.** Design claims must be reconciled with the implementation evidence below.

<a id="e-081"></a>
### E-081 — `tools/server/server-task.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-task.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-task.cpp)
- **Locator:** request parsing and validate_user_id()
- **Evidence type:** source
- **Supports:** OpenAI-style requests accept llama_user_id; Anthropic conversion supplies metadata.user_id; identifiers are validated.
- **Referenced by:** `F-011`, `F-036`, `F-037`, `F-038`, `F-046`, `F-047`, `F-068`

<a id="e-082"></a>
### E-082 — `tools/server/server-context.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-context.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context.cpp)
- **Locator:** slot allocator per-user cap check and same-user affinity
- **Evidence type:** source
- **Supports:** The slot allocator checks active counts for non-empty user IDs and prefers a least-recently-used slot already associated with that user.
- **Referenced by:** `F-025`, `F-036`, `F-039`, `F-041`, `F-047`

<a id="e-083"></a>
### E-083 — `tools/server/server-context.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-context.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context.cpp)
- **Locator:** HTTP admission check for max_concurrent_per_user
- **Evidence type:** source
- **Supports:** HTTP admission performs an early per-user cap check for non-empty user IDs.
- **Referenced by:** `F-039`

<a id="e-084"></a>
### E-084 — `tools/server/server-context.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-context.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context.cpp)
- **Locator:** comment and conditional excluding empty user IDs
- **Evidence type:** source
- **Supports:** Anonymous requests are limited only by the global slot count in the pinned implementation.
- **Referenced by:** `F-040`

**Caveat.** This contradicts the README/design statement that an anonymous bucket receives the same per-user cap.

<a id="e-085"></a>
### E-085 — `tools/server/server-context.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-context.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context.cpp)
- **Locator:** active-user increment/decrement and slot release
- **Evidence type:** source
- **Supports:** Active-user counters are updated only for non-empty identities and decremented on slot release.
- **Referenced by:** `F-039`, `F-040`, `F-041`, `F-047`

<a id="e-086"></a>
### E-086 — `tools/server/server-context.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-context.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context.cpp)
- **Locator:** log messages containing user_id
- **Evidence type:** source
- **Supports:** Scheduling and rate-limit logs include raw user identifiers.
- **Referenced by:** `F-045`

**Caveat.** Raw tenant identifiers should be redacted or hashed in shared observability.

<a id="e-087"></a>
### E-087 — `tools/server/README.md`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/README.md`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/README.md)
- **Locator:** llama_user_id and Anthropic metadata.user_id examples
- **Evidence type:** documentation
- **Supports:** The documented API surfaces the user identifier as request metadata rather than a credential.
- **Referenced by:** `F-036`, `F-037`, `F-046`

**Caveat.** It does not authenticate or cryptographically bind the supplied identity.

<a id="e-090"></a>
### E-090 — `tools/server/server.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server.cpp)
- **Locator:** HTTP route registrations
- **Evidence type:** source
- **Supports:** llama-server registers health, metrics, properties, generation, embeddings, rerank, tokenization, LoRA, expert, slot, model, and stream routes.
- **Referenced by:** `F-032`, `F-053`, `F-066`, `F-067`, `F-068`, `F-069`, `F-070`, `F-071`, `F-072`, `F-077`, `F-078`, `F-088`, `F-117`

<a id="e-091"></a>
### E-091 — `tools/server/server-context.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-context.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context.cpp)
- **Locator:** post_slots route and save/restore/erase handlers
- **Evidence type:** source
- **Supports:** Slot state can be saved, restored, or erased through an HTTP route when slot persistence is enabled.
- **Referenced by:** `F-032`, `F-053`, `F-072`

<a id="e-092"></a>
### E-092 — `tools/server/server-context.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-context.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context.cpp)
- **Locator:** get_metrics route and Prometheus rendering
- **Evidence type:** source
- **Supports:** The metrics endpoint reports prompt/generation counters, rates, queue depth, and slot utilization.
- **Referenced by:** `F-054`, `F-079`

<a id="e-093"></a>
### E-093 — `tools/server/server-task.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-task.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-task.cpp)
- **Locator:** task timing and cache-token result serialization
- **Evidence type:** source
- **Supports:** Completion results expose cached prompt tokens and prompt/generation timing statistics.
- **Referenced by:** `F-054`, `F-066`, `F-067`, `F-080`, `F-120`

<a id="e-094"></a>
### E-094 — `tools/server/server-models.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-models.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-models.cpp)
- **Locator:** server_models load/unload/autoload/LRU child process management
- **Evidence type:** source
- **Supports:** Router mode manages model child processes, autoload, maximum active models, and LRU unloading.
- **Referenced by:** `F-057`, `F-059`, `F-060`, `F-062`

<a id="e-095"></a>
### E-095 — `tools/server/server-models.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-models.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-models.cpp)
- **Locator:** router route handlers for /models, /models/load, /models/unload, /models/sse
- **Evidence type:** source
- **Supports:** Router model lifecycle is exposed through JSON and SSE APIs.
- **Referenced by:** `F-058`, `F-074`, `F-082`

<a id="e-096"></a>
### E-096 — `tools/server/server.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server.cpp)
- **Locator:** router-mode warning
- **Evidence type:** runtime-warning
- **Supports:** The server labels router mode experimental and warns against untrusted exposure.
- **Referenced by:** `F-058`

<a id="e-097"></a>
### E-097 — `tools/server/server.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server.cpp)
- **Locator:** resumable stream route registration
- **Evidence type:** source
- **Supports:** Resumable stream lookup, retrieval, and deletion routes are registered.
- **Referenced by:** `F-073`

<a id="e-098"></a>
### E-098 — `tools/server/server-stream.h`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-stream.h`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-stream.h)
- **Locator:** session manager and conversation identity interfaces
- **Evidence type:** source
- **Supports:** A session manager tracks resumable stream output by conversation identity.
- **Referenced by:** `F-073`

<a id="e-099"></a>
### E-099 — `tools/server/server-stream.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-stream.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-stream.cpp)
- **Locator:** ring buffer, lookup, deletion, and garbage collection
- **Evidence type:** source
- **Supports:** Streaming output is retained in a bounded session structure for reconnect/continuation.
- **Referenced by:** `F-073`

<a id="e-100"></a>
### E-100 — `tools/server/server.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server.cpp)
- **Locator:** LoRA and expert-stat route registrations
- **Evidence type:** source
- **Supports:** LoRA hot-swap and MoE expert observation/control endpoints are exposed.
- **Referenced by:** `F-075`, `F-076`, `F-083`

<a id="e-101"></a>
### E-101 — `tools/server/server-context.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-context.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context.cpp)
- **Locator:** LoRA route handlers
- **Evidence type:** source
- **Supports:** The server can enumerate and apply LoRA adapters without restarting the process.
- **Referenced by:** `F-075`

<a id="e-102"></a>
### E-102 — `tools/server/server.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server.cpp)
- **Locator:** OpenAI, Responses, Anthropic, embeddings, rerank, and transcription routes
- **Evidence type:** source
- **Supports:** The server offers several compatibility APIs beyond the legacy completion endpoint.
- **Referenced by:** `F-066`, `F-067`, `F-068`, `F-069`, `F-070`, `F-071`, `F-119`

<a id="e-103"></a>
### E-103 — `common/arg.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/arg.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/arg.cpp)
- **Locator:** API key, TLS, metrics, slots, model-router arguments
- **Evidence type:** source
- **Supports:** Authentication keys, TLS files, observability endpoints, slots, and router limits are configurable.
- **Referenced by:** `F-046`, `F-059`, `F-088`

<a id="e-104"></a>
### E-104 — `tools/server/server-context.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-context.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context.cpp)
- **Locator:** get_health and get_props handlers
- **Evidence type:** source
- **Supports:** Health is available during load/sleep; properties expose model and server capability metadata.
- **Referenced by:** `F-077`, `F-078`, `F-114`

<a id="e-105"></a>
### E-105 — `tools/server/server-models.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-models.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-models.cpp)
- **Locator:** child process environment and reserved-argument filtering
- **Evidence type:** source
- **Supports:** Router children receive rendered presets while sensitive/router-owned options are withheld.
- **Referenced by:** `F-057`, `F-062`

<a id="e-106"></a>
### E-106 — `tools/server/server-models.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server-models.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-models.cpp)
- **Locator:** model preset reload and SSE notification
- **Evidence type:** source
- **Supports:** Model definitions can be reloaded and lifecycle changes are broadcast by SSE.
- **Referenced by:** `F-061`, `F-074`, `F-082`

<a id="e-110"></a>
### E-110 — `STRIX_HALO_NOTES.md`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / STRIX_HALO_NOTES.md`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/STRIX_HALO_NOTES.md)
- **Locator:** goal, environment, accepted/rejected experiments, validation, benchmark tables
- **Evidence type:** engineering-notes
- **Supports:** The fork records Strix Halo gfx1151 tuning experiments, correctness runs, and benchmark caveats.
- **Referenced by:** `F-097`, `F-098`, `F-102`, `F-103`, `F-104`

**Caveat.** Several entries describe local or historical builds, not the exact clean pin.

<a id="e-111"></a>
### E-111 — `ggml/src/ggml-cuda/vendors/hip.h`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / ggml/src/ggml-cuda/vendors/hip.h`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/ggml/src/ggml-cuda/vendors/hip.h)
- **Locator:** RDNA3.5 architecture classification and AMD intrinsics
- **Evidence type:** source
- **Supports:** The HIP backend recognizes gfx115x devices and selects RDNA3.5 behavior.
- **Referenced by:** `F-094`

<a id="e-112"></a>
### E-112 — `ggml/src/ggml-cuda/mmvq.cu`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / ggml/src/ggml-cuda/mmvq.cu`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/ggml/src/ggml-cuda/mmvq.cu)
- **Locator:** RDNA3.5 MMVQ launch tuning
- **Evidence type:** source
- **Supports:** Quantized matrix-vector launch choices include AMD RDNA3.5-specific tuning.
- **Referenced by:** `F-097`

<a id="e-113"></a>
### E-113 — `ggml/src/ggml-cuda/gated_delta_net.cu`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / ggml/src/ggml-cuda/gated_delta_net.cu`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/ggml/src/ggml-cuda/gated_delta_net.cu)
- **Locator:** RDNA3.5 gated-delta-net launch tuning
- **Evidence type:** source
- **Supports:** Selected recurrent/MoE kernels have target-specific launch parameters.
- **Referenced by:** `F-098`

<a id="e-114"></a>
### E-114 — `ggml/src/ggml-vulkan/ggml-vulkan.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / ggml/src/ggml-vulkan/ggml-vulkan.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/ggml/src/ggml-vulkan/ggml-vulkan.cpp)
- **Locator:** AMD architecture detection, graph optimization, command submission, memory accounting
- **Evidence type:** source
- **Supports:** The Vulkan backend includes AMD architecture routing, graph-order optimization, asynchronous submission, and integrated-memory accounting.
- **Referenced by:** `F-099`, `F-101`

<a id="e-115"></a>
### E-115 — `ggml/src/ggml-vulkan/ggml-vulkan.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / ggml/src/ggml-vulkan/ggml-vulkan.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/ggml/src/ggml-vulkan/ggml-vulkan.cpp)
- **Locator:** get_device_memory() integrated-GPU branch
- **Evidence type:** source
- **Supports:** Integrated Vulkan devices account all memory heaps rather than device-local heaps only.
- **Referenced by:** `F-100`

<a id="e-116"></a>
### E-116 — `ggml/src/ggml-vulkan/ggml-vulkan.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / ggml/src/ggml-vulkan/ggml-vulkan.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/ggml/src/ggml-vulkan/ggml-vulkan.cpp)
- **Locator:** graph optimize pattern matching and fused-op ordering
- **Evidence type:** source
- **Supports:** The fork contains graph scheduling/fusion logic relevant to MoE and recurrent workloads.
- **Referenced by:** `F-101`

<a id="e-120"></a>
### E-120 — `common/arg.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/arg.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/arg.cpp)
- **Locator:** --tools and --agent option registrations
- **Evidence type:** source
- **Supports:** The server can enable built-in agent tools, including file and shell operations, and warns against untrusted exposure.
- **Referenced by:** `F-111`

<a id="e-121"></a>
### E-121 — `tools/server/server.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server.cpp)
- **Locator:** /tools route registration and experimental security warning
- **Evidence type:** source
- **Supports:** Built-in tool routes are conditionally exposed and explicitly labeled experimental/unsafe for untrusted environments.
- **Referenced by:** `F-111`

<a id="e-122"></a>
### E-122 — `common/arg.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/arg.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/arg.cpp)
- **Locator:** --ui-mcp-proxy option
- **Evidence type:** source
- **Supports:** An experimental MCP CORS proxy can be enabled for the Web UI.
- **Referenced by:** `F-112`

<a id="e-123"></a>
### E-123 — `tools/server/server.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server.cpp)
- **Locator:** /cors-proxy route registration and warning
- **Evidence type:** source
- **Supports:** The MCP CORS proxy is conditionally exposed and carries an untrusted-environment warning.
- **Referenced by:** `F-112`

<a id="e-124"></a>
### E-124 — `tools/server/server.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server.cpp)
- **Locator:** register_gcp_compat()
- **Evidence type:** source
- **Supports:** The server registers Google Cloud/Vertex AI compatibility routes.
- **Referenced by:** `F-113`

<a id="e-125"></a>
### E-125 — `common/arg.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/arg.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/arg.cpp)
- **Locator:** --sleep-idle-seconds option
- **Evidence type:** source
- **Supports:** The server can enter an idle sleep state after a configured interval.
- **Referenced by:** `F-114`

<a id="e-126"></a>
### E-126 — `common/arg.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/arg.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/arg.cpp)
- **Locator:** --reasoning, --reasoning-budget, and --reasoning-preserve controls
- **Evidence type:** source
- **Supports:** Reasoning parsing, budgets, and history preservation are configurable.
- **Referenced by:** `F-115`

<a id="e-127"></a>
### E-127 — `common/arg.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/arg.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/arg.cpp)
- **Locator:** --grammar and --json-schema options
- **Evidence type:** source
- **Supports:** Generation can be constrained by grammar or JSON Schema.
- **Referenced by:** `F-116`

<a id="e-128"></a>
### E-128 — `common/arg.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/arg.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/arg.cpp)
- **Locator:** --ui and UI configuration options
- **Evidence type:** source
- **Supports:** The server can host a configurable Web UI.
- **Referenced by:** `F-117`

<a id="e-129"></a>
### E-129 — `common/arg.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/arg.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/arg.cpp)
- **Locator:** --fit and fit-target options
- **Evidence type:** source
- **Supports:** The runtime can estimate memory and adjust unset parameters to fit device capacity.
- **Referenced by:** `F-118`

<a id="e-130"></a>
### E-130 — `tools/server/server.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / tools/server/server.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server.cpp)
- **Locator:** multimodal and audio route registrations
- **Evidence type:** source
- **Supports:** The server exposes multimodal-compatible generation and transcription routes when model capabilities permit.
- **Referenced by:** `F-119`

<a id="e-131"></a>
### E-131 — `common/arg.cpp`

- **Commit:** [`6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940)
- **Source:** [`fewtarius/CachyLLama / common/arg.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/arg.cpp)
- **Locator:** speculative decoding argument group
- **Evidence type:** source
- **Supports:** Draft-model and n-gram speculative decoding are configurable.
- **Referenced by:** `F-120`

## `charlie12345/ROCmFPX`

<a id="e-200"></a>
### E-200 — `(commit)`

- **Commit:** [`a5605a72768c6562241b248e268e33dc92787394`](https://github.com/charlie12345/ROCmFPX/commit/a5605a72768c6562241b248e268e33dc92787394)
- **Source:** [`charlie12345/ROCmFPX / (commit)`](https://github.com/charlie12345/ROCmFPX/commit/a5605a72768c6562241b248e268e33dc92787394)
- **Locator:** commit metadata
- **Evidence type:** commit
- **Supports:** The portability target is assessed at an exact ROCmFPX main-branch commit.
- **Referenced by:** pin/method evidence only

<a id="e-201"></a>
### E-201 — `LICENSE`

- **Commit:** [`a5605a72768c6562241b248e268e33dc92787394`](https://github.com/charlie12345/ROCmFPX/commit/a5605a72768c6562241b248e268e33dc92787394)
- **Source:** [`charlie12345/ROCmFPX / LICENSE`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/LICENSE)
- **Locator:** full license text
- **Evidence type:** license
- **Supports:** ROCmFPX is MIT licensed.
- **Referenced by:** pin/method evidence only

<a id="e-202"></a>
### E-202 — `README.md`

- **Commit:** [`a5605a72768c6562241b248e268e33dc92787394`](https://github.com/charlie12345/ROCmFPX/commit/a5605a72768c6562241b248e268e33dc92787394)
- **Source:** [`charlie12345/ROCmFPX / README.md`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/README.md)
- **Locator:** project status, Strix Halo quick start, tested support, MTP and ROCmFPX scope
- **Evidence type:** documentation
- **Supports:** ROCmFPX already targets AMD/Strix Halo with HIP and Vulkan and labels its feature family experimental.
- **Referenced by:** `F-103`, `F-110`

<a id="e-203"></a>
### E-203 — `common/common.h`

- **Commit:** [`a5605a72768c6562241b248e268e33dc92787394`](https://github.com/charlie12345/ROCmFPX/commit/a5605a72768c6562241b248e268e33dc92787394)
- **Source:** [`charlie12345/ROCmFPX / common/common.h`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/common/common.h)
- **Locator:** cache_ram_mib, cache_disk_path, cache_disk_limit_mib
- **Evidence type:** source
- **Supports:** The target already exposes RAM prompt-cache and disk-spill configuration.
- **Referenced by:** `F-105`

<a id="e-204"></a>
### E-204 — `tools/server/README.md`

- **Commit:** [`a5605a72768c6562241b248e268e33dc92787394`](https://github.com/charlie12345/ROCmFPX/commit/a5605a72768c6562241b248e268e33dc92787394)
- **Source:** [`charlie12345/ROCmFPX / tools/server/README.md`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/README.md)
- **Locator:** --cache-disk and --cache-disk-limit options
- **Evidence type:** source
- **Supports:** The target documents an owner-only, per-run disk prompt cache that is removed at shutdown.
- **Referenced by:** `F-105`

<a id="e-205"></a>
### E-205 — `tools/server/server-task.cpp`

- **Commit:** [`a5605a72768c6562241b248e268e33dc92787394`](https://github.com/charlie12345/ROCmFPX/commit/a5605a72768c6562241b248e268e33dc92787394)
- **Source:** [`charlie12345/ROCmFPX / tools/server/server-task.cpp`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/server-task.cpp)
- **Locator:** server_prompt_cache constructor, namespace ownership marker, lock, stale cleanup
- **Evidence type:** source
- **Supports:** The target creates a private per-process namespace, secures it, locks it, and cleans abandoned owned runs.
- **Referenced by:** `F-105`

<a id="e-206"></a>
### E-206 — `tools/server/server-task.cpp`

- **Commit:** [`a5605a72768c6562241b248e268e33dc92787394`](https://github.com/charlie12345/ROCmFPX/commit/a5605a72768c6562241b248e268e33dc92787394)
- **Source:** [`charlie12345/ROCmFPX / tools/server/server-task.cpp`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/server-task.cpp)
- **Locator:** save_disk() temporary pair, flush, atomic rename, directory sync
- **Evidence type:** source
- **Supports:** The target commits target/draft state through temporary files, validation, atomic rename, and directory synchronization.
- **Referenced by:** `F-106`

<a id="e-207"></a>
### E-207 — `tools/server/server-task.cpp`

- **Commit:** [`a5605a72768c6562241b248e268e33dc92787394`](https://github.com/charlie12345/ROCmFPX/commit/a5605a72768c6562241b248e268e33dc92787394)
- **Source:** [`charlie12345/ROCmFPX / tools/server/server-task.cpp`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/server-task.cpp)
- **Locator:** disk-state LCP matching, state_spec exact-boundary behavior, LRU
- **Evidence type:** source
- **Supports:** The target already performs disk-backed prefix matching with special handling for stateful MTP records and bounded LRU eviction.
- **Referenced by:** `F-109`

<a id="e-208"></a>
### E-208 — `tools/server/server-task.cpp`

- **Commit:** [`a5605a72768c6562241b248e268e33dc92787394`](https://github.com/charlie12345/ROCmFPX/commit/a5605a72768c6562241b248e268e33dc92787394)
- **Source:** [`charlie12345/ROCmFPX / tools/server/server-task.cpp`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/server-task.cpp)
- **Locator:** disable_disk_saves() circuit breaker and corruption paths
- **Evidence type:** source
- **Supports:** Persistent disk write failures open a circuit breaker while reads/fallback remain available.
- **Referenced by:** `F-107`

<a id="e-209"></a>
### E-209 — `tools/server/server-task.cpp`

- **Commit:** [`a5605a72768c6562241b248e268e33dc92787394`](https://github.com/charlie12345/ROCmFPX/commit/a5605a72768c6562241b248e268e33dc92787394)
- **Source:** [`charlie12345/ROCmFPX / tools/server/server-task.cpp`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/server-task.cpp)
- **Locator:** destructor cleanup
- **Evidence type:** source
- **Supports:** The target deletes its owned run directory on clean shutdown, so its current disk cache is not restart-persistent.
- **Referenced by:** pin/method evidence only

<a id="e-210"></a>
### E-210 — `tools/server/tests/unit/test_prompt_cache_disk.py`

- **Commit:** [`a5605a72768c6562241b248e268e33dc92787394`](https://github.com/charlie12345/ROCmFPX/commit/a5605a72768c6562241b248e268e33dc92787394)
- **Source:** [`charlie12345/ROCmFPX / tools/server/tests/unit/test_prompt_cache_disk.py`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/tests/unit/test_prompt_cache_disk.py)
- **Locator:** disk cache hit, LRU, target/draft pair, corruption, failure and cleanup tests
- **Evidence type:** test
- **Supports:** The target has focused automated tests for disk prompt-cache behavior and failure containment.
- **Referenced by:** `F-107`, `F-108`

<a id="e-211"></a>
### E-211 — `tools/server/tests/utils.py`

- **Commit:** [`a5605a72768c6562241b248e268e33dc92787394`](https://github.com/charlie12345/ROCmFPX/commit/a5605a72768c6562241b248e268e33dc92787394)
- **Source:** [`charlie12345/ROCmFPX / tools/server/tests/utils.py`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/tests/utils.py)
- **Locator:** server test configuration for cache_disk and failure injection
- **Evidence type:** test
- **Supports:** The test harness exposes disk-cache settings and portable failure probes.
- **Referenced by:** `F-107`, `F-108`

<a id="e-212"></a>
### E-212 — `README.md`

- **Commit:** [`a5605a72768c6562241b248e268e33dc92787394`](https://github.com/charlie12345/ROCmFPX/commit/a5605a72768c6562241b248e268e33dc92787394)
- **Source:** [`charlie12345/ROCmFPX / README.md`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/README.md)
- **Locator:** verified Strix Halo MTP results and backend recommendations
- **Evidence type:** documentation
- **Supports:** The target already contains newer Strix Halo validation and MTP performance baselines.
- **Referenced by:** `F-103`, `F-110`

**Caveat.** Performance figures are local measurements, not universal guarantees.

<a id="e-213"></a>
### E-213 — `include/llama.h`

- **Commit:** [`a5605a72768c6562241b248e268e33dc92787394`](https://github.com/charlie12345/ROCmFPX/commit/a5605a72768c6562241b248e268e33dc92787394)
- **Source:** [`charlie12345/ROCmFPX / include/llama.h`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/include/llama.h)
- **Locator:** sequence-state API at target pin
- **Evidence type:** source
- **Supports:** The target's llama.cpp base supplies the sequence state primitives required by its existing prompt cache.
- **Referenced by:** pin/method evidence only

<a id="e-214"></a>
### E-214 — `THIRD_PARTY_NOTICES.md`

- **Commit:** [`a5605a72768c6562241b248e268e33dc92787394`](https://github.com/charlie12345/ROCmFPX/commit/a5605a72768c6562241b248e268e33dc92787394)
- **Source:** [`charlie12345/ROCmFPX / THIRD_PARTY_NOTICES.md`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/THIRD_PARTY_NOTICES.md)
- **Locator:** third-party attributions
- **Evidence type:** license
- **Supports:** The target maintains third-party notices alongside its MIT license.
- **Referenced by:** pin/method evidence only

