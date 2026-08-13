# Portable ROCmFPX fixture lane

This directory defines the small, model-general fixture tracked by
[issue #43](https://github.com/JCFrags/HaloFPX/issues/43). The GGUF files are
external artifacts; ordinary Git contains their exact identities, provenance,
license record, reconstruction procedure, and off-target validation evidence.

## Claim boundary

**[VERIFIED]** The pinned BF16 source identity, tracked provenance, license, and
three derived artifact identities match `registry.json`. **[MEASURED]** The
off-target WSL2 run downloaded the source, produced all three pure ROCmFPX
artifacts, and used exact HaloFPX
`b77f2bce6e7875ab065e09894f45915585c9f156` (the `main` authority when this
fixture run began) to load, tensor-check, and generate four tokens from each on
CPU under WSL2.

This establishes exact registered identities and a bounded CPU smoke only. Q3
was independently converted a second time and recorded as byte-identical after
validation; no collective repeatability claim is made for Q6 or Q8. This is not
a quality result, a speed result, or evidence that HIP, Vulkan, one Strix Halo
node, or two distributed nodes work correctly. Those remain **[OPEN]**.

## Source and license authority

**[VERIFIED]** The source is the exact Hugging Face distribution
`unsloth/Qwen3-0.6B-GGUF@28675487b4ea2d7766af79bf32527c73ec715cae`,
file `Qwen3-0.6B-BF16.gguf`, size `1198182848`, SHA-256
`f9c9f1d3c1e21755b82d4e165f88dbbbd4355646d632fb5d6cef7c66ed4ee04e`.
The pinned Hub file pointer and API metadata agreed with the downloaded bytes.

The exact-revision distribution card declares `license: apache-2.0`, names
`Qwen/Qwen3-0.6B` as the base model, and links to its license. The tracked
[Apache-2.0 text](Qwen3-0.6B-LICENSE-c1899de2.txt) adds one terminal LF for
repository normalization (SHA-256
`c156170b718ec29139d3653d40ed1986fd92fb7e0959b5c71f3c48f62e6636f4`).
The materialized sidecar preserves the exact base-repository license bytes at
`Qwen/Qwen3-0.6B@c1899de289a04d12100db370d81485cdf75e47ca`, size `11343`,
SHA-256 `832dd9e00a68dd83b3c3fb9f5588dad7dcf337a0db50f7d9483f310cd292e92e`.

**[OPEN]** The distribution card does not pin the exact upstream base-model
checkpoint revision used to create its BF16 GGUF. The immutable distribution
revision and GGUF hash are therefore the fixture's source authority. The
separate base-model snapshot preserves license text, not conversion lineage.

The small pinned raw sources are retained beside this record:

- [distribution model card](source-distribution-README-28675487.md), tracked
  with one repository-normalized terminal LF;
- [distribution config](source-distribution-config-28675487.json), byte-
  identical to the pinned download; and
- [base-model license text](Qwen3-0.6B-LICENSE-c1899de2.txt), tracked with one
  repository-normalized terminal LF.

The ordinary source record therefore lives with the portable fixture rather
than in `project/sources`, whose imported publication tree is immutable under
the repository validator. Wiki Sections 29 and 30 cite this tracked source
record and route its interpretation into issue #43.

## Fresh-PC reconstruction

Use Linux or WSL2. The exact pinned-b77 Windows native build was attempted,
but current RPC/local-state link requirements make the required CPU-only tools
Linux-only at this commit. Install Git, CMake, Ninja, a C/C++ toolchain, Python
3.10+ with `venv`, and TLS CA certificates. Allow at least 6 GiB of free space
for external fixture bytes plus separate build-tree space. Keep artifact bytes
outside the clone:

```bash
git clone https://github.com/JCFrags/HaloFPX.git HaloFPX
cd HaloFPX
python3 -m venv .venv-fixture
. .venv-fixture/bin/activate
python -m pip install -e ./gguf-py
export HALOFPX_FIXTURE_ROOT="$HOME/halofpx-fixtures/qwen3-0.6b-rocmfpx-pure-v1"
python scripts/materialize-rocmfpx-fixture.py --validate-registry
python scripts/materialize-rocmfpx-fixture.py --download
```

Build the pinned compatible artifact producer in a separate worktree:

```bash
git worktree add ../HaloFPX-fixture-quantizer 6c88472bf5f567a1064f27f4d8a90fc8e2b47a02
cmake -S ../HaloFPX-fixture-quantizer \
  -B ../HaloFPX-fixture-quantizer/build-fixture-cpu -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DGGML_NATIVE=OFF \
  -DGGML_CUDA=OFF -DGGML_HIP=OFF -DGGML_VULKAN=OFF -DGGML_RPC=ON \
  -DGGML_RPC_HALOFPX_LOCAL_STATE=ON -DLLAMA_CURL=OFF \
  -DLLAMA_BUILD_SERVER=OFF -DLLAMA_BUILD_TESTS=OFF
cmake --build ../HaloFPX-fixture-quantizer/build-fixture-cpu \
  --target llama-quantize -j 8
python scripts/materialize-rocmfpx-fixture.py \
  --quantize --verify --census \
  --quantizer ../HaloFPX-fixture-quantizer/build-fixture-cpu/bin/llama-quantize \
  --quantizer-source ../HaloFPX-fixture-quantizer --threads 8
```

The producer pin is intentional. Exact `main` fails all three conversions with
`tensor 'output_norm.weight' has no loader-owned GGUF source-offset authority`.
The enforcing L111 change is commit
`620ef60aa446990335ef46c7d76738f797e62f8f`; its accepted parent `6c88472...`
is used only to create these artifacts. Do not silently replace this producer.

Build the pinned b77 CPU smoke consumer in a second worktree, then
validate all artifacts from the recipe-bearing checkout:

```bash
git worktree add ../HaloFPX-fixture-consumer b77f2bce6e7875ab065e09894f45915585c9f156
cmake -S ../HaloFPX-fixture-consumer \
  -B ../HaloFPX-fixture-consumer/build-fixture-cpu -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DGGML_NATIVE=OFF \
  -DGGML_CUDA=OFF -DGGML_HIP=OFF -DGGML_VULKAN=OFF -DGGML_RPC=ON \
  -DGGML_RPC_HALOFPX_LOCAL_STATE=ON -DLLAMA_CURL=OFF \
  -DLLAMA_BUILD_SERVER=OFF -DLLAMA_BUILD_TESTS=OFF
cmake --build ../HaloFPX-fixture-consumer/build-fixture-cpu \
  --target llama-completion -j 8
python scripts/materialize-rocmfpx-fixture.py \
  --verify --census --smoke \
  --completion ../HaloFPX-fixture-consumer/build-fixture-cpu/bin/llama-completion \
  --completion-source ../HaloFPX-fixture-consumer
```

Keep the recipe-bearing checkout on the branch containing these files; the two
pinned worktrees provide the build source authorities. The script refuses
wrong-size or wrong-hash final files, writes downloads and quantizations through
`.partial` files, requires exact clean source commits, checks the commit reported
by each executed binary, records the actual executable hashes, and retains
command logs below the external artifact root. Rebuilt executables need not be
byte-identical because paths and toolchains can change their hashes. Add
`--require-recorded-binary` only when replaying the exact evidence binaries in
`registry.json`.

The GGUF census uses the repository's `gguf-py` parser and NumPy. Registry
validation and download/hash verification use only the Python standard library;
the isolated environment above makes the parser dependency explicit for the
census phase.

## Publication state

No GGUF in this registry is tracked by Git. The intended private prerelease
publishes only the three derived GGUFs; the BF16 source remains an exact pinned
Hugging Face download.
[The tracked release manifest](qwen3-0.6b-rocmfpx-pure-v1-release-manifest.json)
and [checksum ledger](qwen3-0.6b-rocmfpx-pure-v1-SHA256SUMS.txt) define the
complete nine-asset set for tag `fixture-qwen3-0.6b-rocmfpx-pure-v1`.
[GitHub rejects ordinary Git blobs larger than 100 MiB](https://docs.github.com/en/repositories/working-with-files/managing-large-files/about-large-files-on-github).
[GitHub release assets must each be smaller than 2 GiB](https://docs.github.com/en/repositories/releasing-projects-on-github/about-releases);
all four files satisfy that individual limit, and repository permissions were
sufficient when checked on 2026-08-12. Recheck limits and permissions
immediately before publication, then publish license and an immutable byte/hash
manifest beside any payload. Never infer identity from a filename.

After the immutable private prerelease is published, an authenticated fresh PC
can materialize only the three derived fixtures without downloading BF16:

```bash
mkdir -p "$HALOFPX_FIXTURE_ROOT/derived"
gh release download fixture-qwen3-0.6b-rocmfpx-pure-v1 \
  --repo JCFrags/HaloFPX \
  --pattern '*ROCMFPX-pure.gguf' \
  --dir "$HALOFPX_FIXTURE_ROOT/derived"
python scripts/materialize-rocmfpx-fixture.py --verify-derived --census
```

Download and verify the release manifest, checksum file, license, model card,
config, and modification notice before trusting the payload names. The exact
tag is mandatory; do not substitute an unpinned `latest` release.
