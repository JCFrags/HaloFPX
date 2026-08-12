---
section_id: "16"
title: "Build, CI, license, and AI-workflow procedures"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "HaloFPX planned fork"
  software_versions: []
  hardware_revisions: ["dual Strix Halo / gfx1151 (planned)"]
related_sections: ["03.11", "03.15", "04", "11"]
---

# Procedures and checks

Commands below are safe/read-only unless marked otherwise. They are templates and were not executed on either Strix Halo host.

## 1. Freeze source and dependency identity

Prerequisites: Git. Root: no.

```bash
git status --short
git rev-parse HEAD
git rev-parse HEAD^{tree}
git submodule status --recursive
git diff --check
git diff --submodule=log --stat
```

Fail if the tree is dirty for a release build, a submodule begins with `-`/`+`/`U`, a required source is branch-only rather than commit-pinned, or the recorded patch hashes do not match.

For the inspected `llama-ai` baseline, verify:

```bash
test "$(git rev-parse HEAD:CachyLLama)" = \
  6be745998f568e379ea197fcf827baec73ff9940
```

## 2. Capture the host/toolchain manifest

Prerequisites: Linux host tools, CMake, compiler, ROCm/Vulkan utilities as installed. Root: no.

```bash
uname -a
cat /etc/os-release
cmake --version
ninja --version
python3 --version
git --version
clang --version
hipcc --version
rocminfo
vulkaninfo --summary
```

Write complete output to the experiment evidence directory and reference it from the build manifest. Also capture package-manager versions, kernel command line, firmware/BIOS revision, GPU PCI ID, `rocminfo` target, compiler paths (`command -v`), relevant environment variables, and SHA-256 for downloaded SDK archives. Redact secrets before preservation.

## 3. Materialize then build without drift

Prerequisites: reviewed dependency manifest; CMake >=3.14; C11/C++17 compiler; ROCm toolchain; Vulkan SDK/tools; enough disk. Root: no after prerequisites are installed.

```bash
export SOURCE_DATE_EPOCH="$(git log -1 --pretty=%ct)"
cmake -S . -B build-halofpx -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DGGML_NATIVE=OFF \
  -DGGML_HIP=ON \
  -DGGML_VULKAN=ON \
  -DGGML_CUDA=OFF \
  -DCMAKE_HIP_ARCHITECTURES=gfx1151 \
  -DGPU_TARGETS=gfx1151 \
  -DLLAMA_BUILD_SERVER=ON \
  -DLLAMA_BUILD_WEBUI=OFF \
  -DLLAMA_USE_PREBUILT_WEBUI=OFF \
  -DLLAMA_BUILD_TESTS=ON
cmake --build build-halofpx -j "$(nproc)" --target \
  llama-cli llama-server llama-quantize llama-bench llama-perplexity \
  test-backend-ops test-quantize-fns test-quantize-perf
```

Record `build-halofpx/CMakeCache.txt`, `compile_commands.json`, configure/build logs, and `llama-cli --version`. `SOURCE_DATE_EPOCH` is necessary input normalization, not proof of identical binaries. [S16-15]

After all declared dependencies are mirrored or cached, repeat with outbound network disabled. Any undeclared fetch is a gate failure.

## 4. Required test layers

Prerequisites: successful build. Root: no. GPU tests require exclusive/controlled access to each host.

```bash
ctest --test-dir build-halofpx --output-on-failure
build-halofpx/bin/test-backend-ops test -b CPU
build-halofpx/bin/test-backend-ops test -b ROCm0
build-halofpx/bin/test-backend-ops test -b Vulkan0
```

Then run, with raw output and exact model/workload hashes:

- ROCmFPX CPU reference and quantization round-trip gates;
- the custom ROCmFPX reference workflow commands from [S16-04];
- HIP versus CPU and Vulkan versus CPU operator comparisons;
- server API regression tests;
- persistent-cache cold/hit/restart/corruption/version-mismatch tests;
- two-rank startup, rank loss, transport loss, timeout, and single-node fallback tests;
- matched perplexity/output and performance gates defined by Section 11.

Never relabel a source-repository benchmark as `[MEASURED]` for HaloFPX. A HaloFPX measurement needs raw output plus environment metadata from the actual pair.

## 5. Reproducibility check

Prerequisites: two isolated clean build roots with the same manifest. Root: no.

```bash
find build-a/bin -type f -print0 | sort -z | xargs -0 sha256sum > build-a.sha256
find build-b/bin -type f -print0 | sort -z | xargs -0 sha256sum > build-b.sha256
diff -u build-a.sha256 build-b.sha256
```

If hashes differ, compare build IDs, RPATHs, timestamps, archive ordering, generated shader/assets, compiler paths, and embedded build info. Document every normalization; do not strip evidence merely to force equality. Decide and record whether release acceptance requires bit-identical files or a defined normalized comparison.

## 6. Dependency and license gate

Prerequisites: license/SBOM scanner selected by the project. Root: no.

Checks:

1. enumerate Git-tracked, vendored, fetched, generated, Python, npm, model, and runtime dependencies;
2. resolve every dependency to version/commit plus artifact digest;
3. compare SPDX findings to `LICENSE`, per-file SPDX identifiers, and third-party notices;
4. fail on an unknown license, missing notice, unreviewed GPL import, mutable branch dependency, or unverified download;
5. generate SPDX 3.0.1 SBOM and attach it to release provenance. [S16-12]

This automated result does not replace legal review.

## 7. Append-only AI change record

Recommended NDJSON schema (one physical line per record):

```json
{"schema":"halofpx.ai-change.v1","id":"AIC-YYYYMMDD-0001","time_utc":"YYYY-MM-DDTHH:MM:SSZ","human_owner":"github-or-local-id","agent":"provider/product/model-version","assistance":["research","mechanical-edit"],"requirement":"issue-or-decision-id","base_commit":"40-hex","source_ids":["S16-03"],"files":["path"],"validation":[{"command":"exact command","result":"pass","evidence":"relative/path"}],"commit":null,"risks":[],"human_review":{"reviewer":null,"status":"pending","time_utc":null},"previous_digest":"sha256:...","supersedes":null}
```

Append locally; never edit or delete prior valid lines. After the code commit exists, append a completion record referencing it instead of rewriting the pending record. CI must validate JSON schema, unique IDs, chronological linkage, `previous_digest`, referenced evidence, and that changed material paths have an entry. Publish the final log-head digest in build provenance.

Do not record secrets, raw private prompts, hidden reasoning, or credentials.

## 8. Human review and merge checklist

- linked requirement/issue/decision and focused scope;
- human author understands and can debug every line;
- AI use disclosed according to destination repository rules;
- no AI-authored upstream commit/PR/review prose;
- clean diff and generated-file consistency;
- required CPU, GPU, cache, distributed, performance, and fallback evidence;
- dependency hashes, notices, SBOM, and provenance complete;
- CODEOWNERS/maintainer approval;
- append-only AI log complete;
- squash title follows `<module> : <title> (#issue)` when targeting upstream conventions;
- explicit human authorization before commit, push, release, or upstream submission.

## 9. Internet follow-up cadence

Before rebasing or releasing, re-fetch exact upstream heads, compare workflow and contribution-policy changes, review ROCm/driver support matrices and advisories from official sources, and update the frozen-baseline record. A moving HEAD may inform a candidate update; it never silently replaces a qualified baseline.

