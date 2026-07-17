# Strix Halo / gfx1151 build and software compatibility wiki

Version **2026.07.17**. This is a downloadable, offline-first research package for AMD Strix Halo (`gfx1151`, RDNA 3.5) covering:

- Linux kernel and amdgpu firmware gates;
- ROCm, HIP, ROCr, and bundled LLVM/Clang pairings;
- Mesa RADV, AMDVLK, Vulkan Loader, and ICD selection;
- CMake requirements and exact llama.cpp build flags;
- current llama.cpp release and verified prebuilt asset;
- experimental ROCmFPX builds and fork-only controls;
- standard USB4/Thunderbolt IP networking and research-only verbs transport;
- regressions, unsupported combinations, diagnostics, and acceptance gates.

## Open the wiki

Open [`site/index.html`](site/index.html) in a browser. It is fully offline, includes search, dark/light themes, source pages, and the complete navigation tree.

The Markdown entry point is [`Home.md`](Home.md).

## Key classification rules

1. ROCm 7.14 is the current official **Core SDK** lane for gfx1151 on the captured Ubuntu 24.04.4/HWE 6.17 and Ubuntu 26.04/kernel 7.0 hosts.
2. AMD’s captured RDNA 3.5 target-specific table explicitly marks ROCm 7.2.1–7.2.3 stable on qualifying kernels.
3. ROCm 7.2.4 is an official general release and a maintained community Strix Halo baseline, but is not relabeled target-specific official support here.
4. Firmware 20251125 / MES 0x83 is classified known bad.
5. Current llama.cpp is pinned to b10064 / `86d86ed4396b4130922f7b9af26e3d9fc11a591b`; the safe HIP recipe keeps rocWMMA FlashAttention OFF.
6. RADV is the default Vulkan lane. AMDVLK is classified conditionally usable because of reported large single-allocation failures.
7. `thunderbolt_net` is an upstream kernel component. `thunderbolt-ibverbs` is explicitly research-only, buggy, and insecure.

## Folder layout

```text
.
├── Home.md, _Sidebar.md, _Footer.md     Standard wiki files
├── docs/                                Human-readable knowledge base
│   ├── compatibility-matrix.md
│   ├── official-support.md
│   ├── community-validation.md
│   ├── regressions.md
│   ├── build-flags.md
│   ├── diagnostics.md
│   └── recipes/                         Pinned host/build/network recipes
├── data/                                Versioned CSV, JSON, YAML and schemas
├── sources/                             Stable source registry and claims
├── scripts/                             Build, diagnostics, validation and render tools
├── containers/                          ROCm 7.2.1, ROCm 7.14, RADV and ROCmFPX images
├── site/                                Rendered offline LLM-style wiki
├── llms.txt                             LLM-oriented index
└── MANIFEST.sha256                      Release integrity manifest
```

## Fast start

Inspect the host without changing it:

```bash
./scripts/verify-host.sh
./scripts/collect-diagnostics.sh
```

Build pinned llama.cpp for HIP:

```bash
LLAMA_COMMIT=86d86ed4396b4130922f7b9af26e3d9fc11a591b \
  ./scripts/build-llama-hip.sh
```

Build pinned llama.cpp for Vulkan:

```bash
LLAMA_COMMIT=86d86ed4396b4130922f7b9af26e3d9fc11a591b \
  ./scripts/build-llama-vulkan.sh
```

Download and verify the upstream ROCm 7.2 binary:

```bash
./scripts/download-llama-b10064-rocm72.sh
```

## Data files

- [`data/compatibility-matrix-2026.07.17.csv`](data/compatibility-matrix-2026.07.17.csv)
- [`data/compatibility-matrix-2026.07.17.json`](data/compatibility-matrix-2026.07.17.json)
- [`data/compatibility-matrix-2026.07.17.yaml`](data/compatibility-matrix-2026.07.17.yaml)
- [`data/component-versions-2026.07.17.csv`](data/component-versions-2026.07.17.csv)
- [`data/regressions-2026.07.17.csv`](data/regressions-2026.07.17.csv)
- [`data/environment-variables.csv`](data/environment-variables.csv)
- [`sources/source-registry.json`](sources/source-registry.json)

## Validate and rebuild the offline site

```bash
python3 -m pip install -r requirements-wiki.txt
make validate
make render
```

`make validate` checks data parity, source references, shell syntax, Python compilation, and local links.

## Validation boundary

This release was researched against vendor/upstream sources and first-hand community reports, then statically validated. The generation environment did not contain physical Strix Halo hardware, so it did not execute HIP, Vulkan, amdgpu, USB4, container builds, or performance benchmarks. Recipes use explicit acceptance gates and do not claim local hardware certification.

## License and attribution

Documentation is licensed under CC BY 4.0; original scripts and templates are MIT-licensed. Third-party source material remains under its original licenses and is linked rather than mirrored. This project is not affiliated with or endorsed by AMD, ggml-org, Mesa, Kitware, Khronos, or the Linux kernel project.
