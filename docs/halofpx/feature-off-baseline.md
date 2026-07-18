# HaloFPX feature-off baseline

This record characterizes the first local implementation baseline at
`charlie12345/ROCmFPX@61f2f2d7bc4955e9bca821095ef69125837133b5`. It is L01
evidence, not a release, performance, target-node, or persistent-state claim.

## Contract

With HaloFPX additions absent or disabled, retain:

- model selection and CLI generation surfaces;
- loopback-default server binding and explicit API-key configuration;
- `/health`, `/v1/models`, `/v1/chat/completions`, `/completion`, `/props`, and
  `/slots` route registrations;
- existing K/V cache type, slot-save, speculative/draft, backend, quantization,
  and RPC configuration surfaces;
- no new persistent-state write path.

The executable CTest `test-halofpx-feature-off-contract` checks the stable route,
default-bind, and command-line surface without loading a model or opening a
listener.

## Reproduction

The source was clean before this record and the L01 test were added. The local
build used:

```powershell
cmake -S . -B build/halofpx-baseline -G "Visual Studio 18 2026" -A x64 `
  -DGGML_NATIVE=OFF -DLLAMA_CURL=OFF -DLLAMA_BUILD_SERVER=ON `
  -DLLAMA_BUILD_WEBUI=OFF -DLLAMA_BUILD_TESTS=ON -DLLAMA_BUILD_EXAMPLES=ON
cmake --build build/halofpx-baseline --config Release --parallel 12
```

The inherited default WebUI build first failed because local npm provisioning
was unavailable to CMake and the fallback asset download was incomplete. The
successful baseline explicitly disables WebUI; no UI asset is admitted into the
implementation repository.

`build/halofpx-baseline` is the clean-source build represented by
`baseline-manifest.json`; it predates the addition of the L01 test. After adding
the test, the same configuration was generated independently as
`build/halofpx-milestone`. The L01 CTest result below comes from that post-change
registry. The clean-source hashes remain baseline evidence and are not silently
replaced by dirty-worktree artifacts.

The post-change reproduction uses the same definitions with a separate output
directory, then proves that CTest selected one contract test before running it:

```powershell
cmake -S . -B build/halofpx-milestone -G "Visual Studio 18 2026" -A x64 `
  -DGGML_NATIVE=OFF -DLLAMA_CURL=OFF -DLLAMA_BUILD_SERVER=ON `
  -DLLAMA_BUILD_WEBUI=OFF -DLLAMA_BUILD_TESTS=ON -DLLAMA_BUILD_EXAMPLES=ON
cmake --build build/halofpx-milestone --config Release --parallel 12
ctest --test-dir build/halofpx-milestone -C Release -N `
  -R '^test-halofpx-feature-off-contract$'
ctest --test-dir build/halofpx-milestone -C Release --output-on-failure `
  -R '^test-halofpx-feature-off-contract$'
```

Focused inherited baseline tests:

```text
test-download-model               passed (CTest fixture dependency)
test-arg-parser                   passed
test-gguf                         passed
test-backend-ops                  passed
test-state-restore-fragmented     passed
test-turboquant                  passed
test-alloc                        passed
```

The build/test command returned 100% pass, 7/7. Target-node HIP/runtime/RPC
qualification remains owned by the retained Custom Inference Project evidence;
this local Windows result does not replace it.

The post-change registry selected exactly one
`test-halofpx-feature-off-contract` test, which passed 1/1. CTest selection was
enumerated with `ctest -N -R` before execution so an empty selection could not be
mistaken for success.

## Gate boundary

- G0A selected source identity: reproduced locally from the locked bundle.
- G2 initial baseline: reused only as previously approved target evidence.
- L01 feature-off characterization: implemented here.
- G3 feature-off equivalence: remains open until post-change matched evidence.
- L02 state/scope/format decisions: remain open.
- Donor import, persistent reads/writes, remote creation, deployment, and release:
  not performed.
