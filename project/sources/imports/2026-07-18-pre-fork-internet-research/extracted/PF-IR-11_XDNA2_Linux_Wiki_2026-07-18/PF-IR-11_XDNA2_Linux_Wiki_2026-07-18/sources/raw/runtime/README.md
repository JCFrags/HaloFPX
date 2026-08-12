# Runtime/compiler source boundary

The bundle does not mirror entire runtime/compiler repositories. It preserves immutable repository pins, license blob IDs, exact license excerpts, and an internet-connected re-fetch script.

Research pins:

- XRT: `a21f10d59b51e9235b7c90b804470fce9650cfa0`
- `llvm-aie`: `ce8c0f8fd66bff15b347351c67e9fb4fe0a17205`
- `mlir-aie`: `79089be0abae3c7ec8a69b69357031b99667bdd4`

[UNKNOWN] These public heads are not asserted to be the build inputs for AMD's Ryzen AI Software 1.7.1 binary packages.

Use [`../../fetch-primary-sources.sh`](../../fetch-primary-sources.sh) on a networked host to fetch and Git-blob-verify the public files.
