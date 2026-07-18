# Reference repositories

Purpose: read-only local source-code references for HaloFPX fork planning and provenance checks.

Does not belong here: build outputs, experimental patches, generated code, model data, or a working HaloFPX fork. Create implementation work outside these reference clones.

The four clone directories are deliberately ignored by the parent project so their nested Git histories and worktrees cannot be accidentally committed as project content. `manifest.yaml` is the canonical inventory; `2026-07-17-clone-receipt.md` records the import operation.

## Usage contract

The [2026-07-17 pre-fork source lock](source-locks/2026-07-17-pre-fork/README.md) preserves complete all-ref bundles, exact object/patch/build/license inventories, cryptographic manifests, and verification evidence for the accepted L00A lane. It is a candidate evidence package; OPEN-PIN-01 remains unresolved.

- Treat each clone as immutable evidence captured at the manifest retrieval time.
- Inspect files and history without building, executing, rebasing, pulling, checking out, or editing in place.
- Use the manifest's frozen research pins when reproducing Wiki claims; use captured HEAD only to assess newer upstream changes.
- Create a fresh fork or separate working clone when implementation begins.
- Re-capture provenance in a new dated receipt if any reference clone is intentionally refreshed.

## Canonical local paths

| Repository | Local reference |
|---|---|
| `charlie12345/ROCmFPX` | `sources/repositories/charlie12345__rocmfpx/` |
| `fewtarius/llama-ai` | `sources/repositories/fewtarius__llama-ai/` |
| `fewtarius/CachyLLama` | `sources/repositories/fewtarius__cachyllama/` |
| `ggml-org/llama.cpp` | `sources/repositories/ggml-org__llama.cpp/` |

> This is a source layer, not an implementation decision. Route conclusions through the Wiki and reviewed fork plan.
