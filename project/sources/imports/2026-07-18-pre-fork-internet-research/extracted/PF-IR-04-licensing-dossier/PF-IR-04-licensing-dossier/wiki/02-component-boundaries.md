# Component and gitlink boundaries

## Public component graph

```text
fewtarius/llama-ai@1017f3d  [GPL operational code; CC docs]
  └─ gitlink CachyLLama = 6be745998f568e379ea197fcf827baec73ff9940
       └─ fewtarius/CachyLLama@6be7459 [root MIT; mixed file exceptions]

charlie12345/ROCmFPX@a5605a7 [root MIT; mixed file exceptions]
  └─ one commit later: ROCmFPX@61f2f2d
```

A Git submodule has independent history. The superproject tree records a gitlink object naming the child commit; it does not flatten all child file blobs into the superproject commit. A release archive, installer, or binary can nonetheless materialize or combine components, so the technical boundary is evidence, not a legal conclusion.

## License-scope separation

- **MIT engine source:** ROCmFPX and CachyLLama root assertions, subject to exceptions and provenance.
- **GPL operational expression:** runner/build/install/benchmark/systemd scripts in `llama-ai`.
- **Separately licensed documentation:** `llama-ai` README asserts CC-BY-NC-SA-4.0.
- **External model/data assets:** separate publisher terms; not relicensed by code repositories.
- **Toolchains/runtimes:** per package/component licensing.

See `manifests/components.*` and `manifests/provenance-chain.jsonld`.
