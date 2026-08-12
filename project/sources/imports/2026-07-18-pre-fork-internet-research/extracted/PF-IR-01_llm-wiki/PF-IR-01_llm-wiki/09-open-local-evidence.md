# Local-only evidence collection

[VERIFIED] These procedures are passive local inspection or isolated negative tests. They do not probe a live remote service.

## Artifact and source identity

[RECOMMENDATION] Record the executable SHA-256, ELF/PE build ID, package signature, exact source commit, dirty-state output, initialized submodule commit, compiler version, full CMake cache, build command, and link map.

```text
sha256sum <artifact>
git -C <source> rev-parse HEAD
git -C <source> status --porcelain=v1
git -C <source> submodule status --recursive
cmake -LAH -N <build-dir>
readelf -n <artifact>
```

## Compiled feature proof

[RECOMMENDATION] Demonstrate that the standard artifact lacks RPC executables and backend registration. Use symbol inspection and the produced file list; do not infer from source defaults.

```text
find <artifact-dir> -maxdepth 2 -type f -print
nm -C <artifact> | grep -Ei 'rpc|ggml_backend_rpc'
strings <artifact> | grep -Ei 'rpc-server|rpc backend'
```

[RECOMMENDATION] Treat any RPC hit as a release-engineering investigation, not automatically as exploitability proof.

## Loaded-library provenance

[RECOMMENDATION] Capture the dynamic loader's resolved libraries in the actual deployment image and map every file to package/version/hash.

```text
ldd <artifact>
readelf -d <artifact>
sha256sum <each-resolved-library>
```

[OPEN] ROCm/HIP runtime, GPU driver, OpenSSL, libc, libstdc++, compression libraries, and container base image versions were not provable from the source snapshots.

## Listener and route proof

[RECOMMENDATION] Inspect local sockets without connecting to an existing service. Confirm no RPC listener and only the approved HTTP loopback/Unix-socket binding.

```text
ss -ltnp
ss -lxnp
```

[RECOMMENDATION] Authentication and negative-route tests must start a fresh disposable local instance in the safe lab; do not test an existing deployment.

## Filesystem and model proof

[RECOMMENDATION] Record production model/auxiliary file hashes, ownership, mount options, source license, approval ticket, and read-only status. Record state/cache directory ownership, permissions, mount boundaries, and symlink resolution.

[OPEN] A source commit cannot prove which GGUF, control vector, imatrix, LoRA, multimodal projection, prompt cache, or state file is loaded at runtime.
