# L87 focused-gates receipt

- Accepted base: `4c77d7af13ae03b425ccf32377af7e8bc1024aa8`
- Exact reviewed candidate diff:
  `387468b422a06ec105bcca58af93a3106868d47b`
- Build host/source: `nimo-2`, `/var/tmp/halofpx-l87-source`
- Feature-on: `GGML_RPC=ON`,
  `GGML_RPC_HALOFPX_LOCAL_STATE=ON`, `LLAMA_BUILD_TESTS=ON`,
  `GGML_HIP=OFF`
- Feature-off: `GGML_RPC=ON`,
  `GGML_RPC_HALOFPX_LOCAL_STATE=OFF`, `LLAMA_BUILD_TESTS=ON`,
  `GGML_HIP=OFF`
- Exact disposable endpoints: `127.0.0.1:50189` and
  `127.0.0.1:50190`

Final zero-exit results:

- scheduler: `self_tests=19`, `projection_failures=11`,
  `rpc_census_filter=1`, `composed=1`;
- real two-session composed execution: `real_composed=1`,
  `recompute=1`, `concurrent=1`, `exact=1`;
- storage/two-session fixture: `resolved_storage_identity=1`,
  `false_rpc_destination_refused=1`;
- feature-off: `feature_off_inert=1`;
- durable discriminator mappings: `projection_reason_mappings=8`;
- cleanup: `units_absent=1`.

The mixed composed graph executes with one local mutable input and an RPC
scheduler copy. Its local root remains in graph execution but the exported RPC
census contains only the RPC root/copy. A backend falsely marked RPC while
holding CPU storage remains in the census and refuses as typed
`NON_RPC_STORAGE`; the existing reason-4 self-test retains the impossible
wrong-destination fail-closed check.

`binaries.sha256` binds all final on/off binaries and `outputs.sha256` binds
the five final stdout artifacts. Earlier immutable authority files in this
directory came from disclosed mechanical fixture corrections; the final
stdout/binary receipts replace them as qualification authority.
