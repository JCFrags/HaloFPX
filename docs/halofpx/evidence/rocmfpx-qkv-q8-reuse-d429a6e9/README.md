# ROCmFPX Q/K/V Q8_1 reuse host qualification

Status: **[VERIFIED] host source/graph qualification only** for the patch set
captured at source head `d429a6e933dac620d91f58f58f8c27e3335ceefa`,
based on `ad4930fd632f2f57bbe852dc2268ba3b5b7f5666`. The candidate was then
rebased to publication-source head
`760276e39123622aadf5ef915e9c2b4f92172f8f`, based on
`3758febacfc07fdc6e84b63637131b02d413de59`. Exact range-diff, blob, and
Q/K/V-region comparisons transfer only the bounded source confidence described
below; they do not turn the raw capture into evidence produced at the rebased
head.

## Scope and hard boundary

The checks ran on the local Windows development PC through WSL Ubuntu, not on
either CachyOS Strix Halo node. No HIP compiler or target GPU was used. Issue
[#41](https://github.com/JCFrags/HaloFPX/issues/41) remained the target-test
blocker, and neither `nimo-1` nor `nimo-2` was accessed.

This receipt establishes only that:

- both compile-time selector modes build and pass host graph contracts;
- the alias barrier rejects compaction across an in-place activation write and
  a copy into a projection weight while preserving safe positive controls;
- the source contract binds the option, reorder, dispatch, single conversion,
  ordered three-MMQ calls, metrics procedure, production optimizer callback,
  backend-op assertions, and CI definitions;
- `test-backend-ops` compiles and links with the Q/K/V graph case; and
- a CPU-only focused invocation correctly selects zero feature-dependent cases,
  while a representative existing CPU lane still passes 54/54.

It does not establish a HIP build, runtime triple dispatch, numerical GPU
parity, real-model reachability, launch reduction, graph-replay behavior, or
speed. The pinned GPU-less HIP workflow is part of the source candidate, but
its published run is not claimed by this local receipt.

## Retained evidence

The capture driver is [`capture-driver.sh`](capture-driver.sh). It refused to
reuse either an existing build directory or evidence directory, configured a
fresh build under `build/qkv-host-evidence-d429a6e9`, and retained the following
under [`host-wsl-ubuntu-26.04/`](host-wsl-ubuntu-26.04/):

| Artifact | Purpose |
|---|---|
| `source-state.txt` | exact head, two preceding commits, merge base, branch, status, and implementation-path cleanliness |
| `implementation-blobs.txt` | Git blob identities for every implementation/CI/test input |
| `toolchain.txt` | capture time, WSL/Ubuntu identity, CMake, Ninja, GCC, and G++ |
| `configure-output.txt` | raw fresh CMake configure output |
| `build-output.txt` | raw 243-step focused build output |
| `qkv-contracts-output.txt` | raw focused CTest output |
| `backend-qkv-cpu-output.txt` | raw expected CPU 0/0 feature-gated result |
| `backend-add-f32-output.txt` | raw existing-harness 54/54 result |
| `git-diff-check-output.txt` | exact diff whitespace check |
| `CMakeCache.txt` | complete configured option/toolchain cache |
| `compile_commands.json` | exact host compile database |
| `CMakeConfigureLog.yaml` | CMake compiler/probe log |
| `CTestLastTest.txt` | CTest's own retained per-test record |
| `build-info.cpp` | generated build-information source |
| `build-artifacts.sha256` | hashes of the three executed binaries; binaries are not committed |
| `retained-evidence.sha256` | checksum manifest for every retained host artifact above |

The SHA-256 of `retained-evidence.sha256` is
`756207e980a48b45e35b5416c6cd3ec4942720ffac98b5e1d621ce0453bfff13`.
The local [`.gitattributes`](.gitattributes) disables text normalization for
this evidence subtree so the captured bytes remain stable.

The third captured source commit changes only the pinned-container `hipconfig`
command and its source-contract assertion. The fresh build and retained 3/3
run were performed at `d429a6e9`, after that correction but before the final
rebase to `760276e3`. The final audit found the implementation blobs and Q/K/V
workflow/test regions byte-identical after the rebase. An incremental post-
rebase 3/3 contract run and 54/54 CPU `ADD` lane also passed, but those console
bytes are not part of this raw capture.

## Exact environment

- capture time: `2026-08-13T04:21:26+00:00`;
- WSL Ubuntu `26.04 LTS`;
- CMake `4.2.3`;
- Ninja `1.13.2`;
- Ubuntu GNU C/C++ `15.2.0-16ubuntu1`;
- `Debug`, static libraries, `GGML_HIP=OFF`, `GGML_RPC=OFF`, and
  `GGML_NATIVE=OFF`;
- tests enabled; tools, examples, server, and WebUI disabled.

Windows Git supplied the exact source state because the linked worktree's
`.git` file contains a Windows absolute path that WSL Git cannot resolve. The
fresh CMake build therefore reports its embedded Git identity as unknown; the
retained Windows Git state and blob list bind the captured implementation
paths to `d429a6e9`. Independent range-diff and blob comparisons bind the
reviewed Q/K/V source equivalence to `760276e3`; they are not a substitute for
a fresh raw build at that later head.

## Results

The fresh configure and all three requested build targets completed. Focused
CTest passed 3/3:

| Test | Result |
|---|---|
| `test-halofpx-rocmfpx-qkv-q8-reuse-off` | passed |
| `test-halofpx-rocmfpx-qkv-q8-reuse-on` | passed |
| `test-halofpx-rocmfpx-qkv-q8-reuse-source-contract` | passed |

The source-contract output was:

```text
PASS: ROCmFPX QKV Q8_1 reuse default-off/source/runtime-test contract
```

`test-backend-ops` compiled and linked from the same fresh build directory.
Its focused CPU Q/K/V invocation reported 0/0 and success, as required because
CPU does not advertise `ROCMFPX_QKV_Q8_REUSE`. This is compile coverage for the
target-only graph case, not a runtime counter result. The representative F32
`ADD` lane passed 54/54 after the harness began invoking pre-allocation hooks.
`git diff --check` passed.

The executed-binary SHA-256 values are retained in
`build-artifacts.sha256`; they are scoped to this WSL host build and are not
target binary identities.

## Independent review

The first adversarial review found and blocked publication on an unsafe graph
compaction across an in-place activation mutation. Core head `7a6984c5` adds
the fail-closed alias barrier, negative/positive controls, and production
optimizer seam coverage. The exact final-rebase verdict is retained in
[`independent-review.md`](independent-review.md).
