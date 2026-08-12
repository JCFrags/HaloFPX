# Publication validation receipt — 2026-08-12

This receipt records the local checks run immediately before the first GitHub
publication commit. It is environment-scoped evidence, not a production,
runtime, compatibility, or performance claim.

## Source boundary

- Worktree base before the publication commit:
  `728c3b441fcb38a9eb55272ed673da9d2d18c173`.
- Retained implementation source:
  `620ef60aa446990335ef46c7d76738f797e62f8f`.
- `git diff --quiet 620ef60... -- CMakeLists.txt cmake common ggml include src tests`
  returned `0`; publication work had not changed those implementation paths.

## Documentation checks

- Wiki generated-manifest check: `PASS`.
- Wiki structural/schema validation: `86/86` sections valid.
- Focused wiki-validator unit tests: `4/4 PASS`.
- Monorepo documentation/provenance validator: `PASS` from both the repository
  root and `project/`; 554 Markdown files, 12 category manifests, zero broken
  internal links, 2,883 imported protected Git files checked, and the legacy
  20,858-file evidence snapshot bound to its recorded digest and release
  manifest.
- Publication JSON/YAML and PowerShell syntax checks: `PASS`.
- `git diff --check`: `PASS`.

## Feature-off build

Environment:

- Windows on x86-64;
- Microsoft C/C++ compiler `19.50.35725.0` from Visual Studio Build Tools 18;
- CMake `4.1.2-msvc8`;
- Ninja `1.12.1`.

Configure boundary:

```text
-DCMAKE_BUILD_TYPE=Release
-DBUILD_SHARED_LIBS=OFF
-DGGML_NATIVE=OFF
-DGGML_RPC=OFF
-DGGML_RPC_HALOFPX_LOCAL_STATE=OFF
-DLLAMA_BUILD_TESTS=OFF
-DLLAMA_BUILD_TOOLS=OFF
-DLLAMA_BUILD_EXAMPLES=OFF
-DLLAMA_BUILD_SERVER=OFF
-DLLAMA_BUILD_WEBUI=OFF
-DLLAMA_OPENSSL=OFF
```

`cmake --build ... --target llama --parallel 4` completed successfully. The
resulting disposable `llama.lib` was 43,331,034 bytes with SHA-256
`c746c8bcf296dcf12d5b258d24255b7035de492d035eb0e1b93fa89dd87a7c72`.
The build directory is reproducible scratch and is not a publication asset.

The L111 RPC-on focused configuration was not rerun locally because the source
correctly rejects `GGML_RPC_HALOFPX_LOCAL_STATE=ON` on non-Linux hosts. The
historical Linux L111 evidence remains under `docs/halofpx/evidence/l111/`, and
the publication workflow schedules the portable feature-off build plus the
documentation gates on Ubuntu. This local receipt does not expand L111's
accepted bounded loader-foundation scope.

## Clean-checkout manifest correction

The first remote workflow exposed that the wiki input digest had been generated
from CRLF working-tree bytes before `.gitattributes` took effect. A clean clone
with `core.autocrlf=false` correctly rejected that digest. The manifest was
regenerated in that LF-normalized clone, producing input digest
`5ac4ce11a7ab40dc604ad0c24148dfdc671b8f4a963d4af7b80220f604c7eee5`.
This is a packaging-byte correction; it does not change wiki content or any
engineering claim. The corrected remote workflow is the publication gate.
