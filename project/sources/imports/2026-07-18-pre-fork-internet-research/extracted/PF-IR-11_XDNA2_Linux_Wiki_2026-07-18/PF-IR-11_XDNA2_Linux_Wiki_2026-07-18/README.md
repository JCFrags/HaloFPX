# PF-IR-11 — Linux XDNA2 suitability for a bounded auxiliary role

**Research date:** 2026-07-18  
**Target:** AMD Ryzen AI MAX+ 395 / Strix Halo / XDNA2  
**Decision:** **KEEP EXCLUDED**

This directory is an offline evidence bundle styled as an LLM Wiki. Open [`index.html`](index.html) for the rendered local site or start with [`DECISION.md`](DECISION.md).

## Executive result

[DECISION] The XDNA2 NPU is not adopted for embeddings, reranking, moderation, prompt classification, draft-model inference, or transformer offload in the primary architecture.

[UPSTREAM] Linux has a real `amdxdna` substrate for the expected npu5 PCI revision.

[VENDOR-ONLY] High-level deployment remains dependent on AMD's exact XRT/plugin/Ryzen AI package set, compiler/runtime assets, model preparation, and release-specific compatibility.

[MISSING] The actual target distro and local behavior have not been probed.

[DECISION] The only preserved re-entry gate is an isolated BF16 DistilBERT-class prompt classifier or moderation experiment. It must not alter or delay the primary HIP/Vulkan fork.

## Bundle map

| Path | Purpose |
|---|---|
| [`wiki/`](wiki/) | Research chapters in Markdown and rendered HTML |
| [`decision/`](decision/) | Dated decision and conditional experiment gate |
| [`sources/raw/`](sources/raw/) | Exact primary-source snapshots where captured |
| [`sources/excerpts/`](sources/excerpts/) | Exact source excerpts with pinned provenance |
| [`sources/records/`](sources/records/) | Structured claim captures where full mirroring was not appropriate |
| [`manifests/`](manifests/) | Sources, claims, versions, support boundary, licenses, access log, verification, and hashes |
| [`probe/`](probe/) | Non-invasive target-host probe; no install, load, reset, or workload by default |
| [`sources/fetch-primary-sources.sh`](sources/fetch-primary-sources.sh) | Re-fetch recipe using immutable source URLs |
| [`assets/`](assets/) | Offline wiki CSS and JavaScript |

## Integrity

Run:

```bash
sha256sum -c manifests/files.sha256
```

The ZIP has a separate SHA-256 sibling file.

## Important limitations

[MISSING] Account-gated AMD DEB/TGZ packages were not downloaded, so their hashes are absent.

[MISSING] Firmware binaries are not redistributed in this bundle. Exact paths, versions, source pin, license reference, and target-host hashing procedure are preserved.

[UNKNOWN] Public XRT, `llvm-aie`, and `mlir-aie` repository heads are recorded, but they are not asserted to be the build inputs for AMD's Ryzen AI 1.7.1 binary packages.

[MISSING] No target performance result is claimed.
