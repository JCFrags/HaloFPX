# Dependencies and licenses

## License boundary

| Repository layer | Exact pin | Observed license | Port rule |
|---|---|---|---|
| `fewtarius/llama-ai` | `1017f3dfdce3ca2b06aa9007b23295db3bb35722` | GPL-3.0; scripts commonly carry GPL-3.0-or-later SPDX identifiers | Do not copy parent scripts or unit files into an MIT-only ROCmFPX distribution unless the corresponding GPL terms are accepted. Reimplement behavior or keep a separate GPL package. |
| `fewtarius/CachyLlama` | `6be745998f568e379ea197fcf827baec73ff9940` | MIT | Suitable for selective source adaptation with notices preserved. |
| `charlie12345/ROCmFPX` | `a5605a72768c6562241b248e268e33dc92787394` | MIT | Target integration baseline. |
| Parent documentation | `1017f3dfdce3ca2b06aa9007b23295db3bb35722` | Separate content license identified by the parent README | Paraphrase rather than copying substantial prose. |

Evidence: [E-003](20-Evidence-Index.md#e-003), [E-004](20-Evidence-Index.md#e-004), [E-021](20-Evidence-Index.md#e-021), [E-201](20-Evidence-Index.md#e-201).

## Dependency inventory

| Capability family | Direct dependencies | Portability notes |
|---|---|---|
| Primary SSD cache | C++17 filesystem; open/pread/pwrite/fsync/unlink; sequence-state serialization; threads; token metadata | Replace POSIX assumptions with ROCmFPX's cross-platform file layer. Preserve target failure injection. |
| System-prefix cache | tokenization; prompt hash; compatibility fingerprint; state serialization; startup directory scan | Boundary detection must use parsed message/template structure. |
| Checkpoint restore | `llama_state_seq_get_size_ext`, `get_data_ext`, `set_data_ext`; target/draft contexts; spec/MTP state | Reconcile against target API; preserve destination sequence ID and stateful exact-boundary semantics. |
| Hybrid state | hybrid memory implementation and attention-only sequence removal | Gate by model architecture; add recurrent correctness tests. |
| Server page manager | filesystem namespaces; cache registry; scheduler slot state; model fingerprint | Replace split anonymous/user maps with one scoped registry. |
| User isolation | JSON protocol converters; validation; active-counter map; slot allocator | Requires authenticated principal binding; request body metadata is only a hint. |
| HTTP APIs | server HTTP layer, JSON, SSE, protocol converters, chat templates | Prefer target/upstream implementation to reduce fork drift. |
| Model router | child process library; local HTTP proxy; preset parser; model catalog; SSE | Explicitly experimental; authorize lifecycle controls. |
| Observability | server logger; Prometheus text renderer; task timing counters | Add cache source and breaker state without tenant labels. |
| Parent launcher | Bash, coreutils, sed/grep/awk, process tools, Hugging Face CLI/Python | GPL boundary; shell quoting/process ownership require redesign. |
| Build helper | CMake, Ninja/Make, C/C++ compiler, Vulkan SDK, ROCm/HIP, Metal | Keep target-native ROCmFPX build scripts. |
| GPU detection | `lspci`, sysfs, ROCm environment, device maps | Device/runtime-version data is temporally unstable; use overrides and tests. |
| GTT/TTM helper | root, amd-smi, GRUB/systemd-boot, kernel parameters | Administrator-only documentation/tool; reject from inference core. |
| Strix HIP tuning | HIP compiler/runtime, gfx1151, quant/recurrent kernels | Benchmark per ROCmFPX format and workload. |
| Strix Vulkan tuning | Vulkan device properties, memory budget extension, graph optimizer, shaders | Target has newer custom Vulkan code; port by narrow symbol-level reconciliation. |
| ROCmFPX test baseline | Python, pytest, server test harness, portable I/O fault injection | Extend with multi-process restart and tenant tests. |

## Per-file notice handling

1. Preserve the MIT license and copyright notices for copied CachyLlama units.
2. Record the source commit and local modifications in ROCmFPX's third-party notices.
3. Avoid copying GPL parent scripts into the target tree unless ROCmFPX's distribution licensing is intentionally changed.
4. Audit vendored or bundled dependencies independently; repository-level MIT does not erase third-party obligations.
5. Audit model licenses separately from server code.

This is not legal advice. See [`notices/LICENSE-NOTICE.md`](notices/LICENSE-NOTICE.md).
