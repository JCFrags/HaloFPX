# llama-ai GPL operational-layer file headers

capture_method: GitHub/public-web connector capture
byte_fidelity: line-normalized authoritative connector capture
claim_label: PRIMARY-FILE-SPDX
source_url: https://github.com/fewtarius/llama-ai/tree/1017f3dfdce3ca2b06aa9007b23295db3bb35722
repository: fewtarius/llama-ai
commit_or_revision: 1017f3dfdce3ca2b06aa9007b23295db3bb35722
repository_path: sampled operational files
git_blob_sha1: n/a
access_date: 2026-07-18

---

The following exact blobs carry the literal header `SPDX-License-Identifier: GPL-3.0-or-later` and 2026 fewtarius copyright text:

- `llama-run.sh` — `3684a299361b3ca45ff57656a43073f8b9629047`
- `scripts/rebuild.sh` — `dedf0d299169a2898331289bc0009554a75b1952`
- `scripts/install-deps.sh` — `77ee08d914e597596255c189254ab462dc5da857`
- `scripts/env.sh` — `4a6e94d04d55e2836e5a70cbdedba47d66fe4e1a`
- `scripts/detect-gpu.sh` — `4f2a4bbedc82a610f1852b81759016347562e1ca`
- `scripts/benchmark.sh` — `6d4743a25906a271f8eef17bbe5e153c3d430704`
- `scripts/apply-ttm-kernel-params.sh` — `76b50c10cf0ec4dd2b76c0afcdd90851ede882ec`
- `systemd/llama-server.service` — `f991dae4c5a5b73be725401984e7f7a54fa48eef`
- backend build scripts: ROCm `170fbf8e9f4c1af4d74e71ae207dc98da7dae1ef`, Vulkan `a05384400a51e5e88bcfa4c5e867fb0f9829de72`, Metal `54a07bf0fdd4c1d2cc530d961bf495895688ae9b`.

This supports a GPL-licensed operational layer. It does not change the separately pinned CachyLLama engine's own license evidence.
