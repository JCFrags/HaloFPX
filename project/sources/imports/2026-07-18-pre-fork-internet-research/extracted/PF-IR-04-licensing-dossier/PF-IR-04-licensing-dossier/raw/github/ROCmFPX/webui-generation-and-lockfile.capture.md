# ROCmFPX Web UI generation, embedding, and lockfile evidence

capture_method: GitHub/public-web connector capture
byte_fidelity: line-normalized authoritative connector capture
claim_label: PRIMARY-DEPENDENCY-DECLARATION
source_url: https://github.com/charlie12345/ROCmFPX/tree/61f2f2d7bc4955e9bca821095ef69125837133b5/tools/server/webui
repository: charlie12345/ROCmFPX
commit_or_revision: 61f2f2d7bc4955e9bca821095ef69125837133b5
repository_path: tools/server/webui and server build scripts
git_blob_sha1: n/a
access_date: 2026-07-18

---

Key blobs:

- `tools/server/webui/package.json` — `2338c38402a0ad01fb85a24c646cd7ad316f21b4`.
- `tools/server/webui/package-lock.json` — `bf23307b82c2489a680e9da3f11efed455149246`, lockfile v3.
- `scripts/webui-download.cmake` — `47c4a784d2fd10aad077111002b020752115b433`.
- `tools/server/CMakeLists.txt` — `9778e6341575858471aa28571ce658d6f62d3d00`.
- `scripts/xxd.cmake` — `14d2753808a8e2fd7709b87d9568e982e7192fb1`.
- Web UI publishing workflow — `bf0d707a23e33a6e65593cd4786afcd5bc6e49e3`.

The server build can obtain a cached/local npm build or a Hugging Face bucket build, then embed `index.html`, `bundle.js`, `bundle.css`, and `loading.html` as generated C++ headers. The download path does not itself acquire a third-party notice bundle. A binary release must retain exact asset hashes, source tree, lockfile, build command/environment, npm license inventory, and source-to-generated-header-to-binary mapping. Mutable `latest` bucket outputs are blocked unless pinned and verified.
