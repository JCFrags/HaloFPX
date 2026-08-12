---
section_id: "56"
title: "CachyLLama source audit and port validation"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["fewtarius/CachyLLama", "ggml-org/llama.cpp", "charlie12345/ROCmFPX"]
  software_versions: ["CachyLLama 6be745998f568e379ea197fcf827baec73ff9940"]
  hardware_revisions: ["dual gfx1151 Strix Halo; exact hosts pending inventory"]
related_sections: ["57", "58", "61", "63", "65"]
---

# CachyLLama source audit and port validation

## Safety boundary

Source inspection and ordinary read-only comparisons require no root access. Any checkpoint mutation, corruption, deletion, crash, tenant-isolation probe, link interruption, or I/O pressure must use a disposable cache/store and disposable service instance with exact resolved paths/store UUID, resource ceilings, preserved evidence/recovery access, stop conditions, cleanup, and a receipt. Refuse production cache/model/workspace/boot paths and sole evidence copies. Kernel, device, cable, reboot, or power faults require Section 80 authorization and declared minimum privilege.

## Reproduce the source audit

No root access is required. Work in a disposable clone and verify the commit before inspection.

```bash
git clone https://github.com/fewtarius/CachyLLama.git
git -C CachyLLama checkout 6be745998f568e379ea197fcf827baec73ff9940
git -C CachyLLama status --short
git -C CachyLLama show --stat --oneline HEAD
rg -n "kv_ssd_|compat_hash|llama_user_id|state_seq|recurrent|spec_data" \
  CachyLLama/common CachyLLama/tools/server CachyLLama/include CachyLLama/src
```

Inspect every file in the integration map, record blob hashes, and compare CachyLLama against its actual merge base with the selected ROCmFPX/llama.cpp commits. Do not assume same-named APIs have the same ABI.

## Machine experiment matrix

1. Inventory exact model/tokenizer/template/build/topology and capture section 57 fingerprint inputs.
2. For transformer, recurrent/hybrid, MTP and speculative cases, save at multiple prefix boundaries; restart; restore; replay suffix; compare token logits/state-dependent outputs against full recomputation under deterministic settings.
3. Mutate one compatibility field at a time; every mutation must produce a diagnostic miss.
4. Truncate, corrupt, swap, rename and replay checkpoint/index files; no invalid state may reach inference.
5. Crash at each write boundary, including data write, sync, publication, index update and directory sync; restart and inventory survivors.
6. Exercise explicit tenant A/B plus anonymous traffic. Attempt cross-tenant IDs, path characters, hash collisions and continuation discovery.
7. Compare cold/warm/hot and readahead behavior at realistic state sizes. Capture page cache, faults, NVMe I/O, DRAM duplication, restore latency and evaluation saved.
8. In two-rank mode, independently restore rank-local components; delay, corrupt or remove one rank; verify no mixed-generation readiness and correct single-node/recompute fallback.

## Acceptance rules

- Exact continuation output alone is insufficient; verify the restored suffix-only path was actually used.
- A cache hit requires complete required component validation; missing draft/spec/recurrent state is a miss unless an explicit safe recomputation path is proven.
- Performance claims require raw runs, environment/build manifests, matched full-recompute baseline and repeated distributions.
- Migration reads CachyLLama files as untrusted input and writes new HaloKV objects; it never relabels files in place.

## Internet follow-up

Recheck the pinned forks' merge bases, open cache-related issues/PRs, state-format revisions and upstream llama-state API changes immediately before implementation. Preserve exact source commits and licenses under `sources/` if code is imported.
