# L86 focused gates release receipt

- Accepted base: `6f1f962ae0cb5670e727d4b2bfdbbcc462649f91`
- Candidate diff identity (`git diff --binary | git hash-object --stdin`): `29a5640c10ef830291ff47d5d875860d0f3c45f6`
- Build host: `nimo-2`
- Source root: `/var/tmp/halofpx-l86-source`
- Feature-on build: `GGML_RPC=ON`, `GGML_RPC_HALOFPX_LOCAL_STATE=ON`, `LLAMA_BUILD_TESTS=ON`
- Feature-off build: `GGML_RPC=ON`, `GGML_RPC_HALOFPX_LOCAL_STATE=OFF`, `LLAMA_BUILD_TESTS=ON`
- Disposable endpoints: `127.0.0.1:50187`, `127.0.0.1:50188`
- Disposable server units were stopped and collected; `cleanup.stdout` records exact absence.

All named commands exited zero:

1. `test-halofpx-scheduler-authority 127.0.0.1:50187`
2. `test-halofpx-rpc-mutable-authority 127.0.0.1:50187 127.0.0.1:50188`
3. `test-halofpx-rpc-mutable-authority --storage-identity 127.0.0.1:50187 127.0.0.1:50188`
4. `test-halofpx-rpc-mutable-authority --feature-off 127.0.0.1:50187` with every HaloFPX feature environment value removed
5. `test-halofpx-l83-preprepare-diagnostic`
6. Feature-on targets: `test-halofpx-scheduler-authority`, `test-halofpx-rpc-mutable-authority`, `test-halofpx-l83-preprepare-diagnostic`, `rpc-server`
7. Feature-off targets: `llama`, `rpc-server`

Key results:

- `self_tests=18 projection_failures=11`
- `resolved_storage_identity=1`
- `feature_off_inert=1`
- `projection_reason_mappings=8`
- full real composed fixture: `real_composed=1 recompute=1 concurrent=1 exact=1`

`binaries.sha256` binds the exact on/off binaries. `outputs.sha256` binds the reproducible stdout artifacts. The retained authority files are immutable fixture output; no key material is retained.
