# Upstream Watch Query Catalog and Freshness Budgets

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.



The source of truth is [`config/upstream-watch.yaml`](../config/upstream-watch.yaml). Queries are discovery controls: a matching commit, issue, release, or page hash is not proof that the SUT is affected. Triage maps the immutable event to exact layers and experiment canaries.

## Polling catalog

| Source ID | Collector | Repository, URL, query, or path | Freshness class | Minimum canaries |
|---|---|---|---|---|
| `WATCH-LLAMA-RELEASES` | `github_releases` | ggml-org/llama.cpp | `runtime_releases` | EXP-003, EXP-004, EXP-005, EXP-006, EXP-007, EXP-008, EXP-009, EXP-015, EXP-019 |
| `WATCH-LLAMA-SECURITY-ADVISORIES` | `web_hash` | https://github.com/ggml-org/llama.cpp/security/advisories | `security_critical` | EXP-001, EXP-015, EXP-016, EXP-017 |
| `WATCH-LLAMA-RPC-COMMITS` | `github_commits` | ggml-org/llama.cpp :: tools/rpc | `relevant_commits_and_issues` | EXP-001, EXP-002, EXP-008, EXP-009, EXP-016, EXP-017, EXP-018, EXP-019 |
| `WATCH-LLAMA-SERVER-COMMITS` | `github_commits` | ggml-org/llama.cpp :: tools/server | `relevant_commits_and_issues` | EXP-004, EXP-007, EXP-008, EXP-009, EXP-011, EXP-015 |
| `WATCH-LLAMA-ISSUES-STRIX` | `github_issue_search` | repo:ggml-org/llama.cpp (gfx1151 OR "Strix Halo" OR "Ryzen AI Max" OR ROCm OR HIP) is:open | `community_signal` | EXP-005, EXP-006, EXP-012, EXP-019 |
| `WATCH-LLAMA-ISSUES-RPC` | `github_issue_search` | repo:ggml-org/llama.cpp (RPC OR remote) (hang OR crash OR regression OR performance OR cache) is:open | `community_signal` | EXP-007, EXP-008, EXP-009, EXP-016, EXP-017, EXP-018 |
| `WATCH-ROCM-RELEASES` | `github_releases` | ROCm/ROCm | `runtime_releases` | EXP-001, EXP-005, EXP-006, EXP-007, EXP-012, EXP-013, EXP-019 |
| `WATCH-ROCM-SECURITY-ADVISORIES` | `web_hash` | https://github.com/ROCm/ROCm/security/advisories | `security_critical` | EXP-001, EXP-005, EXP-006, EXP-012, EXP-019 |
| `WATCH-ROCM-ISSUES-GFX1151` | `github_issue_search` | org:ROCm (gfx1151 OR "Strix Halo" OR "Ryzen AI Max") is:issue | `relevant_commits_and_issues` | EXP-005, EXP-006, EXP-012, EXP-013, EXP-019 |
| `WATCH-THEROCK-COMMITS` | `github_commits` | ROCm/TheRock | `relevant_commits_and_issues` | EXP-001, EXP-005, EXP-006, EXP-019 |
| `WATCH-ROCM-SYSTEMS-COMMITS` | `github_commits` | ROCm/rocm-systems | `relevant_commits_and_issues` | EXP-012, EXP-013 |
| `WATCH-ROCM-RELEASE-NOTES` | `web_hash` | https://rocm.docs.amd.com/en/latest/about/release-notes.html | `compatibility_docs` | EXP-001, EXP-005, EXP-006, EXP-012, EXP-019 |
| `WATCH-ROCM-COMPAT` | `web_hash` | https://rocm.docs.amd.com/projects/radeon-ryzen/en/latest/docs/compatibility/compatibilityryz/native_linux/native_linux_compatibility.html | `compatibility_docs` | EXP-001, EXP-005, EXP-006, EXP-019 |
| `WATCH-RDNA35-GUIDANCE` | `web_hash` | https://rocm.docs.amd.com/en/latest/how-to/system-optimization/rdna3-5.html | `compatibility_docs` | EXP-001, EXP-005, EXP-006, EXP-012, EXP-019 |
| `WATCH-AMD-SECURITY` | `web_hash` | https://www.amd.com/en/resources/product-security.html | `security_critical` | triage only |
| `WATCH-LINUX-AMDGPU` | `git_atom` | https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/atom/drivers/gpu/drm/amd · keywords: gfx1151, Strix, KFD, amdgpu, reset, power, thermal | `kernel_mesa_llvm` | EXP-005, EXP-006, EXP-012, EXP-013, EXP-019 |
| `WATCH-LINUX-FIRMWARE-AMDGPU` | `git_atom` | https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git/atom/amdgpu · keywords: Strix, gfx1151, gc_11, dcn_3_5, psp_14, amdgpu | `kernel_mesa_llvm` | EXP-001, EXP-005, EXP-006, EXP-012, EXP-013, EXP-019 |
| `WATCH-LINUX-THUNDERBOLT` | `git_atom` | https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/atom/drivers/thunderbolt · keywords: USB4, thunderbolt, net, lane, retimer, reset | `kernel_mesa_llvm` | EXP-002, EXP-007, EXP-016, EXP-018, EXP-019 |
| `WATCH-MESA-RELEASE-NOTES` | `web_hash` | https://docs.mesa3d.org/relnotes.html | `kernel_mesa_llvm` | EXP-005, EXP-006, EXP-012, EXP-019 |
| `WATCH-MESA-AMD-COMMITS` | `git_atom` | https://gitlab.freedesktop.org/mesa/mesa/-/commits/main/src/amd?format=atom · keywords: gfx1151, Strix Halo, GFX11.5, RDNA 3.5, amdgpu | `kernel_mesa_llvm` | EXP-005, EXP-006, EXP-012, EXP-019 |
| `WATCH-LLVM-AMDGPU-COMMITS` | `github_commits` | llvm/llvm-project :: llvm/lib/Target/AMDGPU · keywords: gfx1151, GFX11, RDNA 3.5, AMDGPU | `kernel_mesa_llvm` | EXP-005, EXP-006, EXP-015, EXP-019 |
| `WATCH-VLLM-RELEASES` | `github_releases` | vllm-project/vllm | `runtime_releases` | EXP-007, EXP-008, EXP-009, EXP-015, EXP-019 |
| `WATCH-PYTORCH-RELEASES` | `github_releases` | pytorch/pytorch | `runtime_releases` | EXP-005, EXP-006, EXP-015, EXP-019 |
| `WATCH-OEM-BIOS` | `manual_or_web_hash` | exact OEM support/advisory URL for both node models | `oem_bios_firmware` | EXP-001, EXP-002, EXP-012, EXP-013, EXP-019 |
| `WATCH-DISTRO-SECURITY` | `manual_or_web_hash` | exact security/advisory feed for the deployed Linux distribution and release | `security_critical` | EXP-001, EXP-015 |

## Freshness budgets

| Class | Poll interval | Stale after | Triage SLA | Release blocking |
|---|---:|---:|---|---|
| `security_critical` | 4h | 12h | same business day | Yes |
| `runtime_releases` | 12h | 36h | 1 business day | Yes |
| `relevant_commits_and_issues` | 24h | 72h | 3 business days | Yes |
| `kernel_mesa_llvm` | 24h | 72h | 3 business days | Yes |
| `compatibility_docs` | 7d | 10d | 5 business days | Yes |
| `oem_bios_firmware` | 7d | 10d | 5 business days | Yes |
| `community_signal` | 24h | 7d | weekly review | No |

## Query discipline

- Poll immutable release IDs, commit SHAs, advisory IDs, and page content hashes; do not use a mutable page title as identity.
- Use a two-hour overlap when polling commits/issues, then deduplicate by normalized event ID.
- Record both publication and discovery timestamps. Freshness is based on the last successful ledger update, not on the configured cron expression.
- Search `gfx1151`, `Strix Halo`, `Ryzen AI Max`, RDNA 3.5, KFD/amdgpu, RPC, prompt/KV cache, USB4/Thunderbolt, profiler/telemetry, resets, hangs, corruption, and regression terms.
- Configure exact OEM BIOS/USB4 firmware and deployed-distribution security feeds before G1. Their placeholder URLs are deliberately release blocking.
- Never auto-merge or auto-upgrade from a watch hit. P0/P1 requires owner, exposure analysis, targeted canaries, decision, and evidence links.

GitHub schedules can be delayed or disabled under documented conditions, so the process gates on ledger timestamps and source-specific stale budgets. [[SRC-022]](../references/Sources.md#src-022) [[SRC-023]](../references/Sources.md#src-023) [[SRC-024]](../references/Sources.md#src-024)
