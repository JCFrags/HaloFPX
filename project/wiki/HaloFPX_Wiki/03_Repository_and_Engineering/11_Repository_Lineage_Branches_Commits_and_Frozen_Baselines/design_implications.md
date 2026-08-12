---
section_id: "11"
title: "Repository Lineage Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"
    - "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
  software_versions: []
  hardware_revisions: []
related_sections:
  - "13"
  - "14"
  - "15"
  - "16"
---

# Design implications

## Repository roles

**[RECOMMENDATION]** Assign one role to each upstream rather than blending their branch histories:

| Role | Authority | HaloFPX use |
|---|---|---|
| Canonical engine upstream | `ggml-org/llama.cpp` | API, backend, model, server, and bug-fix tracking |
| AMD-format implementation base | `charlie12345/ROCmFPX` | Initial source tree and ROCmFPX format/kernel work |
| Cache/agent-serving donor | `fewtarius/CachyLLama` | Select capability commits after dependency and correctness review |
| Operational reference | `fewtarius/llama-ai` | Profiles, packaging, scripts, and end-to-end behavior; not the engine merge base |
| Product integration authority | New HaloFPX repository | Reviewed patch stack, decisions, baseline manifests, and releases |

**[INFERENCE]** This split follows the verified lineage: llama-ai wraps CachyLLama; CachyLLama is graph-related to upstream; ROCmFPX is content-related but graph-independent. One indiscriminate merge would obscure provenance and make later upstream synchronization harder.

## Commit-level integration, not branch-level composition

**[RECOMMENDATION]** Import a candidate capability as one or more traceable commits with:

1. donor repository and full source SHA;
2. original author/license and copied paths;
3. prerequisite commits and upstream equivalent search;
4. conflict-resolution notes;
5. tests proving the claimed behavior;
6. a stable patch ID or tree comparison after adaptation.

**[RECOMMENDATION]** A CachyLLama merge commit contains both fork changes and an upstream batch, so do not treat cherry-picking the merge head as capability selection. Identify the fork-side commits since its last upstream merge base, then review only the needed dependency closure (SRC-11-004, SRC-11-005).

**[INFERENCE]** ROCmFPX cannot be synchronized by assuming a standard fork relationship because its current history has no merge base with upstream. A sync must nominate an upstream commit, compare trees/patches, and record how upstream code was incorporated.

## Baseline as a compound object

**[RECOMMENDATION]** Define a HaloFPX baseline as the tuple below, not only a Git SHA:

```text
source graph
  + full commit IDs and recursive gitlinks
  + patch-series identity
  + dependency and toolchain lock
  + model/config/test-fixture hashes
  + build options and generated-artifact provenance
  + source bundles and license inventory
  + validation receipt
```

**[INFERENCE]** Git alone captures source objects but not ROCm packages, compiler behavior, downloaded WebUI/vendor data, model weights, BIOS/kernel state, or runtime flags. Those inputs can change correctness or performance while the source commit remains constant.

**[RECOMMENDATION]** Use a manifest committed inside HaloFPX and an annotated project tag pointing to that manifest commit. External upstream tags remain descriptive fields only; full object IDs are normative.

## Proposed remote and branch model

```text
origin       -> HaloFPX integration repository
rocmfpx     -> charlie12345/ROCmFPX
cachyllama  -> fewtarius/CachyLLama
llamacpp    -> ggml-org/llama.cpp
llama-ai    -> fewtarius/llama-ai (reference only)
```

**[RECOMMENDATION]** Protect the HaloFPX default branch and use these branch classes:

- `baseline/YYYY-MM-DD.N`: immutable candidate cut, deleted only after tag/archive review.
- `integration/<capability>`: one donor capability plus its tests.
- `sync/llama.cpp/<sha12>`: controlled upstream synchronization.
- `experiment/<issue>`: non-authoritative machine work; measurements live under `experiments/`.

**[RECOMMENDATION]** Never force-move a published `halofpx-baseline-*` tag. Supersede it with a new tag and record the reason.

## Upstream synchronization consequences

**[VERIFIED]** At the snapshot, CachyLLama is 125 upstream commits behind while carrying 53 commits after the common base (SRC-11-005).

**[INFERENCE]** The first cache import should not wait for permanent branch convergence, but it must isolate cache behavior behind tests and review upstream changes touching server slots, recurrent state, prompt caching, and memory APIs. These are likely conflict surfaces based on the recent commit sample, not guaranteed conflicts.

**[RECOMMENDATION]** Section 15 should keep a machine-readable patch ledger with states `candidate`, `adapted`, `accepted`, `upstream-equivalent`, `rejected`, and `superseded`. A source SHA never changes; its disposition may.

## Distributed and cache-specific safeguards

**[RECOMMENDATION]** Each frozen distributed baseline must state:

- rank ownership and process/device mapping;
- transport implementation and dual-link configuration;
- failure behavior if one rank or link is unavailable;
- single-node fallback commit/configuration;
- cache ownership, namespace, on-disk format version, and compatibility key.

**[RECOMMENDATION]** Cache corruption, a source/model/config mismatch, or a failed integrity check must produce a cache miss/recomputation, never acceptance of invalid state. The compatibility key should include the HaloFPX commit, model SHA-256, tokenizer/config identity, cache ABI/schema, backend, and relevant runtime options.

## Licensing and provenance

**[VERIFIED]** Repository metadata reports MIT for llama.cpp, CachyLLama, and ROCmFPX, while llama-ai declares GPL-3.0 and separately licenses documentation (SRC-11-002, SRC-11-003, SRC-11-006, SRC-11-010).

**[RECOMMENDATION]** Copying engine code from CachyLLama/ROCmFPX requires retained notices and per-file provenance. Copying runner code from llama-ai can change the product’s licensing obligations; route that decision through section 16 rather than assuming all four repositories are interchangeable.

## Promotion gates

| Gate | Required evidence | Owner section |
|---|---|---|
| Source identity | Full SHAs, remotes, gitlinks, bundles | 11 |
| Feature scope | File/symbol/commit inventory | 13 and 14 |
| Patch acceptance | Dependency closure, conflicts, tests | 15 |
| Build reproducibility | Toolchain/dependency lock, CI receipt | 16 |
| Machine applicability | Matched two-node experiment metadata | Relevant experiment section |

**[OPEN]** No project decision yet identifies the first accepted ROCmFPX base, selected CachyLLama commits, or llama.cpp tracking cadence.
