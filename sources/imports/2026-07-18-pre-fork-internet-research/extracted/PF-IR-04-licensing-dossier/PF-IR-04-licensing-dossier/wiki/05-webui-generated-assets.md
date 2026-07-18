# Web UI, npm dependencies, and generated embedded assets

## Generation chain

```text
webui source + package-lock
  → npm install/build
  → index.html + bundle.js + bundle.css + loading.html
  → xxd/CMake generated C++ headers
  → server object files and binary
```

The build can also consume bucket-hosted assets, including a mutable `latest` path. That path is blocked for release evidence unless an immutable revision, checksums, source correspondence, and notices are captured.

## Direct runtime dependency evidence

The exact lockfile identifies direct runtime components under MIT, BSD-3-Clause, and Apache-2.0. The complete table is `manifests/webui-direct-dependencies.csv`. This is not the full transitive SBOM and does not replace package license-file retention.

## Release evidence required

- Exact Web UI source commit and dirty-tree status.
- Exact `package-lock.json` blob and `npm ci` version.
- Node/npm versions and build environment digest.
- Full runtime and transitive dependency SBOM, tarball integrity, and license texts.
- Hashes for the four built assets.
- Hashes for generated headers and the object/binary sections containing them.
- Build recipe and reproducibility comparison.
- Consolidated Web UI notice file shipped with source and binary releases.
