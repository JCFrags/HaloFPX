# ROCmFPX–CachyLLama Integration Wiki Package

This directory is a GitHub-Wiki-style design package. Open `Home.md` first. The standard wiki files are:

- `Home.md`
- `_Sidebar.md`
- `_Footer.md`

The root Markdown files can be copied into a GitHub Wiki repository without restructuring. Mermaid source files are under `diagrams/`; evidence locks, templates, and checklists are in their named subdirectories.

## Evidence lock

- Canonical: `charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394`
- Donor engine: `fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940`
- Donor parent: `fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722`
- Upstream: `ggml-org/llama.cpp@86d86ed4396b4130922f7b9af26e3d9fc11a591b`
- Evidence date: `2026-07-17`

## Package constraints

This package contains architecture, process, source references, decision records, and acceptance criteria. It contains **no implementation patch, no copied donor source, and no generated diff**. See `No-Patch-Notice.md`.

`MANIFEST.sha256` covers every package file except itself.
