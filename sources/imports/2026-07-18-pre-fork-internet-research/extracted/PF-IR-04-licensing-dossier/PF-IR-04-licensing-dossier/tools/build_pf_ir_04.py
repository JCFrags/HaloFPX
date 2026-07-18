from __future__ import annotations

import csv
import hashlib
import json
import os
import shutil
import textwrap
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Iterable

import mistune
from jinja2 import Template

ROOT = Path('/mnt/data/PF-IR-04-licensing-dossier')
ZIP = Path('/mnt/data/PF-IR-04-licensing-dossier.zip')
ACCESS_DATE = '2026-07-18'
CREATED_UTC = datetime.now(timezone.utc).replace(microsecond=0).isoformat()

if ROOT.exists():
    shutil.rmtree(ROOT)
if ZIP.exists():
    ZIP.unlink()

for d in [
    ROOT / 'assets',
    ROOT / 'wiki',
    ROOT / 'raw' / 'github' / 'ROCmFPX',
    ROOT / 'raw' / 'github' / 'CachyLLama',
    ROOT / 'raw' / 'github' / 'llama-ai',
    ROOT / 'raw' / 'upstream',
    ROOT / 'manifests',
    ROOT / 'release',
    ROOT / 'hashes',
    ROOT / 'tools',
]:
    d.mkdir(parents=True, exist_ok=True)


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def git_blob_sha1(data: bytes) -> str:
    return hashlib.sha1(f'blob {len(data)}\0'.encode('ascii') + data).hexdigest()


def write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text.rstrip() + '\n', encoding='utf-8')


def write_json(path: Path, obj: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(obj, indent=2, sort_keys=False, ensure_ascii=False) + '\n', encoding='utf-8')


def write_csv(path: Path, rows: list[dict[str, Any]], fieldnames: list[str] | None = None) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if fieldnames is None:
        fieldnames = list(rows[0].keys()) if rows else []
    with path.open('w', newline='', encoding='utf-8') as f:
        w = csv.DictWriter(f, fieldnames=fieldnames, extrasaction='ignore')
        w.writeheader()
        for row in rows:
            w.writerow({k: row.get(k, '') for k in fieldnames})


def copy_exact(src: str, dst: Path, expected_git_blob: str, source_url: str, commit: str, path_in_repo: str, claim_label: str) -> dict[str, Any]:
    data = Path(src).read_bytes()
    actual = git_blob_sha1(data)
    if actual != expected_git_blob:
        raise RuntimeError(f'Git blob mismatch for {src}: expected {expected_git_blob}, got {actual}')
    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_bytes(data)
    meta = {
        'capture_path': str(dst.relative_to(ROOT)),
        'capture_method': 'exact-byte HTTP retrieval followed by Git blob verification',
        'byte_fidelity': 'exact',
        'source_url': source_url,
        'repository_commit': commit,
        'repository_path': path_in_repo,
        'git_blob_sha1': expected_git_blob,
        'sha256': sha256_bytes(data),
        'size_bytes': len(data),
        'access_date': ACCESS_DATE,
        'claim_label': claim_label,
    }
    write_json(dst.with_suffix(dst.suffix + '.capture.json'), meta)
    return meta


def write_connector_capture(dst: Path, title: str, source_url: str, repository: str | None, commit: str | None,
                            path_in_repo: str | None, git_blob: str | None, claim_label: str,
                            body: str, fidelity: str = 'line-normalized authoritative connector capture') -> dict[str, Any]:
    header = f"""# {title}

capture_method: GitHub/public-web connector capture
byte_fidelity: {fidelity}
claim_label: {claim_label}
source_url: {source_url}
repository: {repository or 'n/a'}
commit_or_revision: {commit or 'not pinned by source'}
repository_path: {path_in_repo or 'n/a'}
git_blob_sha1: {git_blob or 'n/a'}
access_date: {ACCESS_DATE}

---

"""
    write_text(dst, header + body)
    data = dst.read_bytes()
    meta = {
        'capture_path': str(dst.relative_to(ROOT)),
        'capture_method': 'connector capture',
        'byte_fidelity': fidelity,
        'source_url': source_url,
        'repository': repository,
        'repository_commit_or_revision': commit,
        'repository_path': path_in_repo,
        'git_blob_sha1': git_blob,
        'sha256': sha256_bytes(data),
        'size_bytes': len(data),
        'access_date': ACCESS_DATE,
        'claim_label': claim_label,
    }
    write_json(dst.with_suffix(dst.suffix + '.capture.json'), meta)
    return meta


# ---------------------------------------------------------------------------
# Exact-byte primary captures verified against Git blobs
# ---------------------------------------------------------------------------
captures: list[dict[str, Any]] = []

captures.append(copy_exact(
    '/mnt/data/rocmfpx-license.txt',
    ROOT / 'raw/github/ROCmFPX/LICENSE.blob-e7dca554bcb802f98408383a864404e3aa4eacca.txt',
    'e7dca554bcb802f98408383a864404e3aa4eacca',
    'https://raw.githubusercontent.com/charlie12345/ROCmFPX/a5605a72768c6562241b248e268e33dc92787394/LICENSE',
    'a5605a72768c6562241b248e268e33dc92787394 and 61f2f2d7bc4955e9bca821095ef69125837133b5',
    'LICENSE',
    'PRIMARY-REPO-LICENSE-TEXT',
))

captures.append(copy_exact(
    '/mnt/data/rocmfpx-readme.md',
    ROOT / 'raw/github/ROCmFPX/README.blob-2a446c96ecb2bf7fc8510c09c325141bd0152061.md',
    '2a446c96ecb2bf7fc8510c09c325141bd0152061',
    'https://raw.githubusercontent.com/charlie12345/ROCmFPX/a5605a72768c6562241b248e268e33dc92787394/README.md',
    'a5605a72768c6562241b248e268e33dc92787394 and 61f2f2d7bc4955e9bca821095ef69125837133b5',
    'README.md',
    'PRIMARY-REPOSITORY-ASSERTION',
))

captures.append(copy_exact(
    '/mnt/data/tpn-main.md',
    ROOT / 'raw/github/ROCmFPX/THIRD_PARTY_NOTICES.blob-4b2f877fe5011ac7eca70b5409a1cb7b032109ea.md',
    '4b2f877fe5011ac7eca70b5409a1cb7b032109ea',
    'https://raw.githubusercontent.com/charlie12345/ROCmFPX/main/THIRD_PARTY_NOTICES.md',
    'a5605a72768c6562241b248e268e33dc92787394 and 61f2f2d7bc4955e9bca821095ef69125837133b5 (verified unchanged by Git blob)',
    'THIRD_PARTY_NOTICES.md',
    'PRIMARY-FILE-NOTICE',
))

captures.append(copy_exact(
    '/mnt/data/cachy-readme-master.md',
    ROOT / 'raw/github/CachyLLama/README.blob-2a19230f301821314d4b061a1ae9dfd00c0254e1.md',
    '2a19230f301821314d4b061a1ae9dfd00c0254e1',
    'https://raw.githubusercontent.com/fewtarius/CachyLLama/master/README.md',
    '6be745998f568e379ea197fcf827baec73ff9940 (verified unchanged by Git blob)',
    'README.md',
    'PRIMARY-REPOSITORY-ASSERTION',
))

captures.append(copy_exact(
    '/mnt/data/test-gpl-license.txt',
    ROOT / 'raw/github/llama-ai/LICENSE.blob-f288702d2fa16d3cdf0035b15a9fcbc552cd88e7.txt',
    'f288702d2fa16d3cdf0035b15a9fcbc552cd88e7',
    'https://raw.githubusercontent.com/fewtarius/llama-ai/1017f3dfdce3ca2b06aa9007b23295db3bb35722/LICENSE',
    '1017f3dfdce3ca2b06aa9007b23295db3bb35722',
    'LICENSE',
    'PRIMARY-REPO-LICENSE-TEXT',
))

captures.append(copy_exact(
    '/mnt/data/llama-ai-gitmodules',
    ROOT / 'raw/github/llama-ai/.gitmodules.blob-c73a9ad6ca6261b42aea9c2d5107fd24298066bb',
    'c73a9ad6ca6261b42aea9c2d5107fd24298066bb',
    'https://raw.githubusercontent.com/fewtarius/llama-ai/main/.gitmodules',
    '1017f3dfdce3ca2b06aa9007b23295db3bb35722 (verified unchanged by Git blob)',
    '.gitmodules',
    'PRIMARY-GITLINK-CONFIG',
))

captures.append(copy_exact(
    '/mnt/data/llama-ai-readme.md',
    ROOT / 'raw/github/llama-ai/README.blob-cb8f9dee239db6df865ee08d0b7008eb5f8b71be.md',
    'cb8f9dee239db6df865ee08d0b7008eb5f8b71be',
    'https://raw.githubusercontent.com/fewtarius/llama-ai/1017f3dfdce3ca2b06aa9007b23295db3bb35722/README.md',
    '1017f3dfdce3ca2b06aa9007b23295db3bb35722',
    'README.md',
    'PRIMARY-REPOSITORY-ASSERTION',
))

# Reuse the verified MIT text for CachyLLama because the Git blob is identical.
mit_data = (ROOT / 'raw/github/ROCmFPX/LICENSE.blob-e7dca554bcb802f98408383a864404e3aa4eacca.txt').read_bytes()
mit_cachy = ROOT / 'raw/github/CachyLLama/LICENSE.blob-e7dca554bcb802f98408383a864404e3aa4eacca.txt'
mit_cachy.write_bytes(mit_data)
write_json(mit_cachy.with_suffix('.txt.capture.json'), {
    'capture_path': str(mit_cachy.relative_to(ROOT)),
    'capture_method': 'byte-identical reuse of verified Git blob capture',
    'byte_fidelity': 'exact',
    'source_url': 'https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/LICENSE',
    'repository_commit': '6be745998f568e379ea197fcf827baec73ff9940',
    'repository_path': 'LICENSE',
    'git_blob_sha1': 'e7dca554bcb802f98408383a864404e3aa4eacca',
    'sha256': sha256_bytes(mit_data),
    'size_bytes': len(mit_data),
    'access_date': ACCESS_DATE,
    'claim_label': 'PRIMARY-REPO-LICENSE-TEXT',
})
captures.append(json.loads(mit_cachy.with_suffix('.txt.capture.json').read_text()))

# Empty .gitmodules captures are exact by definition and Git blob hash.
for repo, commit in [
    ('ROCmFPX', '61f2f2d7bc4955e9bca821095ef69125837133b5'),
    ('CachyLLama', '6be745998f568e379ea197fcf827baec73ff9940'),
]:
    p = ROOT / f'raw/github/{repo}/.gitmodules.empty.blob-e69de29bb2d1d6434b8b29ae775ad8c2e48c5391'
    p.write_bytes(b'')
    meta = {
        'capture_path': str(p.relative_to(ROOT)),
        'capture_method': 'exact empty Git blob reconstruction',
        'byte_fidelity': 'exact',
        'source_url': f'https://github.com/{"charlie12345/ROCmFPX" if repo == "ROCmFPX" else "fewtarius/CachyLLama"}/blob/{commit}/.gitmodules',
        'repository_commit': commit,
        'repository_path': '.gitmodules',
        'git_blob_sha1': 'e69de29bb2d1d6434b8b29ae775ad8c2e48c5391',
        'sha256': sha256_bytes(b''),
        'size_bytes': 0,
        'access_date': ACCESS_DATE,
        'claim_label': 'PRIMARY-GITLINK-BOUNDARY',
    }
    write_json(p.with_suffix('.capture.json'), meta)
    captures.append(meta)

# ---------------------------------------------------------------------------
# Line-normalized authoritative captures and extracted primary-source facts
# ---------------------------------------------------------------------------
connector_specs = [
    {
        'dst': ROOT / 'raw/github/ROCmFPX/UPSTREAM-ATTRIBUTION.blob-1870f3bbdb04432b0ec56d2d74a2fe2895d3cdfc.capture.md',
        'title': 'ROCmFPX upstream attribution register',
        'source_url': 'https://github.com/charlie12345/ROCmFPX/blob/61f2f2d7bc4955e9bca821095ef69125837133b5/docs/UPSTREAM-ATTRIBUTION.md',
        'repository': 'charlie12345/ROCmFPX', 'commit': '61f2f2d7bc4955e9bca821095ef69125837133b5',
        'path': 'docs/UPSTREAM-ATTRIBUTION.md', 'blob': '1870f3bbdb04432b0ec56d2d74a2fe2895d3cdfc',
        'label': 'PRIMARY-UPSTREAM-ATTRIBUTION-ASSERTION',
        'body': '''The maintainer-authored register states that promoted work was manually ported or cherry-picked from identified upstream commits. Recorded source identifiers include:\n\n- Core/manual-port group: `2998a4d7b`, `b7ec1175d`, `fab339703`, `bcbb6d0ee`, `67e8a335a`, `7c941cb95`, `3d669b855`, `500e185e2`, `ee949e639`, `b4d949ab7`.\n- Chat parser fixtures: `a6dff7127092a9cd75db81aaef0456598d1d0452`.\n- Web UI CI fixes: `0c3e4fcc...`, `1348f67c...`.\n- Apple XCFramework claim: `4d742877...`.\n- DiffusionGemma sources: `c5fe75b...`, `c84e85a...`, `d8794ee...`, `6d75883...`, `8a4a856...`.\n- Qwen/Gemma/Step MTP sources: `eef59a...`, `2187e0...`, `04eb4c...`, `166fe2...`, `7d2b45...`, `e95dae...`.\n\nThis is primary evidence of the maintainer's provenance assertion. It is not an independent byte-identity or originality determination. A release reviewer must follow each identifier into the exact upstream tree and compare the locally proposed files.''',
    },
    {
        'dst': ROOT / 'raw/github/ROCmFPX/61f-delta-new-HIP-files.capture.md',
        'title': 'ROCmFPX a5605a7..61f2f2d added HIP files',
        'source_url': 'https://github.com/charlie12345/ROCmFPX/compare/a5605a72768c6562241b248e268e33dc92787394...61f2f2d7bc4955e9bca821095ef69125837133b5',
        'repository': 'charlie12345/ROCmFPX', 'commit': '61f2f2d7bc4955e9bca821095ef69125837133b5',
        'path': 'commit comparison', 'blob': None, 'label': 'PRIMARY-COMMIT-DIFF',
        'body': '''The one-commit delta adds two files:\n\n- `ggml/src/ggml-hip/fattn-kv-batched.cu`, blob `0aa5ddb57f295df482ebbf4603641fc072b419b3`.\n- `ggml/src/ggml-hip/fattn-vec-turbo-mixed.cu`, blob `5bb0577b1a2b0154b7bde487dc5b7fd21ff2ee78`.\n\nThe beginning of each file has no SPDX identifier or copyright notice. The public tree therefore supplies only the root MIT assertion for these files. Origin/history and any source adaptation remain a human review item.''',
    },
    {
        'dst': ROOT / 'raw/github/ROCmFPX/file-level-exceptions.capture.md',
        'title': 'Representative ROCmFPX file-level license exceptions',
        'source_url': 'https://github.com/charlie12345/ROCmFPX/tree/61f2f2d7bc4955e9bca821095ef69125837133b5',
        'repository': 'charlie12345/ROCmFPX', 'commit': '61f2f2d7bc4955e9bca821095ef69125837133b5',
        'path': 'representative files', 'blob': None, 'label': 'PRIMARY-FILE-SPDX',
        'body': '''Representative exact file evidence:\n\n- `ggml/src/ggml-sycl/common.cpp`, blob `ae08abad81bb8cbeffea19a90afa189f648653aa`: MIT-tagged material and an LLVM-derived section tagged `Apache-2.0 WITH LLVM-exception`.\n- `ggml/src/ggml-cpu/kleidiai/kleidiai.h`, blob `38eac58f7c207ded2a74db705c75f92e6e6eb66c`: Arm copyright and `MIT`.\n- `ggml/src/ggml-openvino/openvino/frontend.h`, blob `f1c6f0c3e3ce3eaebb8afbcbc91a01b236778f1b`: `Apache-2.0`.\n- `common/base64.hpp`, blob `563247a6e5f7dba837c07a509026d8b36e61387c`: Unlicense/public-domain dedication.\n- `examples/gguf-hash/deps/sha256/sha256.c`, blob `a7a87aeb20032c7f9e87491af927b9074037202a`: public-domain notices attributed to Igor Pavlov and public-domain Crypto++ code.\n\nThese are direct exceptions to any simplistic whole-tree “MIT only” inventory.''',
    },
    {
        'dst': ROOT / 'raw/github/CachyLLama/file-level-exceptions.capture.md',
        'title': 'Representative CachyLLama file-level license exceptions',
        'source_url': 'https://github.com/fewtarius/CachyLLama/tree/6be745998f568e379ea197fcf827baec73ff9940',
        'repository': 'fewtarius/CachyLLama', 'commit': '6be745998f568e379ea197fcf827baec73ff9940',
        'path': 'representative files', 'blob': None, 'label': 'PRIMARY-FILE-SPDX',
        'body': '''Representative exact file evidence:\n\n- `ggml/src/ggml-sycl/common.cpp`, blob `e1b6db13eb41575a57a1c60b3ed1a2a42bef3afd`: MIT-tagged material plus `Apache-2.0 WITH LLVM-exception` content.\n- `ggml/src/ggml-cpu/kleidiai/kleidiai.h`, blob `38eac58f7c207ded2a74db705c75f92e6e6eb66c`: `MIT`.\n- `ggml/src/ggml-openvino/openvino/frontend.h`, blob `72134a3e8cf2980729fbb0d73c71c5160af4dc30`: `Apache-2.0`.\n\nNo root `THIRD_PARTY_NOTICES.md` was present at the exact commit. The root MIT license is not a complete exception inventory.''',
    },
    {
        'dst': ROOT / 'raw/github/llama-ai/gpl-file-headers.capture.md',
        'title': 'llama-ai GPL operational-layer file headers',
        'source_url': 'https://github.com/fewtarius/llama-ai/tree/1017f3dfdce3ca2b06aa9007b23295db3bb35722',
        'repository': 'fewtarius/llama-ai', 'commit': '1017f3dfdce3ca2b06aa9007b23295db3bb35722',
        'path': 'sampled operational files', 'blob': None, 'label': 'PRIMARY-FILE-SPDX',
        'body': '''The following exact blobs carry the literal header `SPDX-License-Identifier: GPL-3.0-or-later` and 2026 fewtarius copyright text:\n\n- `llama-run.sh` — `3684a299361b3ca45ff57656a43073f8b9629047`\n- `scripts/rebuild.sh` — `dedf0d299169a2898331289bc0009554a75b1952`\n- `scripts/install-deps.sh` — `77ee08d914e597596255c189254ab462dc5da857`\n- `scripts/env.sh` — `4a6e94d04d55e2836e5a70cbdedba47d66fe4e1a`\n- `scripts/detect-gpu.sh` — `4f2a4bbedc82a610f1852b81759016347562e1ca`\n- `scripts/benchmark.sh` — `6d4743a25906a271f8eef17bbe5e153c3d430704`\n- `scripts/apply-ttm-kernel-params.sh` — `76b50c10cf0ec4dd2b76c0afcdd90851ede882ec`\n- `systemd/llama-server.service` — `f991dae4c5a5b73be725401984e7f7a54fa48eef`\n- backend build scripts: ROCm `170fbf8e9f4c1af4d74e71ae207dc98da7dae1ef`, Vulkan `a05384400a51e5e88bcfa4c5e867fb0f9829de72`, Metal `54a07bf0fdd4c1d2cc530d961bf495895688ae9b`.\n\nThis supports a GPL-licensed operational layer. It does not change the separately pinned CachyLLama engine's own license evidence.''',
    },
    {
        'dst': ROOT / 'raw/github/llama-ai/benchmark-gutenberg-fixture.capture.md',
        'title': 'llama-ai Project Gutenberg benchmark fixture reference',
        'source_url': 'https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/scripts/benchmark.sh',
        'repository': 'fewtarius/llama-ai', 'commit': '1017f3dfdce3ca2b06aa9007b23295db3bb35722',
        'path': 'scripts/benchmark.sh', 'blob': '6d4743a25906a271f8eef17bbe5e153c3d430704', 'label': 'PRIMARY-TEST-FIXTURE-REFERENCE',
        'body': '''The script identifies its long text fixture as *The Count of Monte Cristo* from Project Gutenberg and downloads `http://aleph.gutenberg.org/cache/epub/1184/pg1184.txt` into `scratch/pg1184.txt`. It does not pin the served bytes, record a digest, preserve a separate Project Gutenberg terms capture, or perform a jurisdiction check.''',
    },
    {
        'dst': ROOT / 'raw/github/llama-ai/model-downloader.capture.md',
        'title': 'llama-ai runtime model downloader behavior',
        'source_url': 'https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/llama-run.sh',
        'repository': 'fewtarius/llama-ai', 'commit': '1017f3dfdce3ca2b06aa9007b23295db3bb35722',
        'path': 'llama-run.sh', 'blob': '3684a299361b3ca45ff57656a43073f8b9629047', 'label': 'PRIMARY-RUNTIME-DOWNLOAD-BEHAVIOR',
        'body': '''The runner queries Hugging Face and permits selection/download of arbitrary model repositories and GGUF files at runtime. The script does not create a release record containing model-card license metadata, repository revision, file digest, tokenizer/template license, or publisher notice. Therefore runtime model downloads are distribution dependencies or user-supplied content, not automatically licensed by the parent GPL file.''',
    },
    {
        'dst': ROOT / 'raw/github/ROCmFPX/model-fixture-scripts.capture.md',
        'title': 'ROCmFPX local model fixture generation scripts',
        'source_url': 'https://github.com/charlie12345/ROCmFPX/tree/61f2f2d7bc4955e9bca821095ef69125837133b5/scripts',
        'repository': 'charlie12345/ROCmFPX', 'commit': '61f2f2d7bc4955e9bca821095ef69125837133b5',
        'path': 'scripts/build-rocmfpx-agent-fixtures.sh and scripts/run-deepseek-v4-rocmfp4-fixture.sh',
        'blob': None, 'label': 'PRIMARY-GENERATION-RECIPE',
        'body': '''- `scripts/build-rocmfpx-agent-fixtures.sh`, blob `ef695709e160abedc61d552703d16338fae6123d`, defaults to a maintainer-local Qwen3-0.6B Q4_K_M path and produces derived GGUF test assets.\n- `scripts/run-deepseek-v4-rocmfp4-fixture.sh`, blob `3b7329a757c8150499448eb19e02e1285455d086`, defaults to a maintainer-local `DeepSeek-V4-Flash-180B` source directory and generates/quantizes GGUF outputs.\n\nNeither script identifies a publisher revision, source license file, source hash, quantizer identity, or permission record for the local inputs. The scripts may be considered under the repository code assertion; their generated model outputs are separate and blocked pending exact input provenance.''',
    },
    {
        'dst': ROOT / 'raw/github/ROCmFPX/tokenizer-template-generation.capture.md',
        'title': 'ROCmFPX tokenizer and chat-template generation paths',
        'source_url': 'https://github.com/charlie12345/ROCmFPX/tree/61f2f2d7bc4955e9bca821095ef69125837133b5',
        'repository': 'charlie12345/ROCmFPX', 'commit': '61f2f2d7bc4955e9bca821095ef69125837133b5',
        'path': 'convert_hf_to_gguf_update.py; scripts/get_chat_template.py; tests/test-tokenizer-0.sh; models/templates',
        'blob': None, 'label': 'PRIMARY-GENERATION-RECIPE',
        'body': '''- `tests/test-tokenizer-0.sh`, blob `7024b00afe341eb8d8411085c8a7db963e25a15c`, consumes external tokenizer directories and generated `ggml-vocab-*.gguf` files.\n- `convert_hf_to_gguf_update.py`, blob `8d73b1f5546abf3b04bf3af85449513b941a5e4a`, retrieves many Hugging Face tokenizers and generates vocabulary fixtures without a release-time publisher revision record.\n- `scripts/get_chat_template.py`, blob `b4827b317e1c934b359388a578b534f114bdc282`, fetches `tokenizer_config.json` from a repository's mutable `main` branch and extracts `chat_template`.\n- `models/templates/README.md`, blob `3a649b8f4dbd9099e3a6ad49793b88f13530fa26`, documents checked-in templates; `models/templates/tencent-Hy3.jinja` is blob `f6185ccbfc00d25b4aaa7caeb26b3db6f4466d3b` and has no source/license header.\n\nA generated container does not erase obligations attached to source tokenizer/template expression. Pin source revision, capture source license, hash the source and result, and preserve attribution.''',
    },
    {
        'dst': ROOT / 'raw/github/ROCmFPX/webui-generation-and-lockfile.capture.md',
        'title': 'ROCmFPX Web UI generation, embedding, and lockfile evidence',
        'source_url': 'https://github.com/charlie12345/ROCmFPX/tree/61f2f2d7bc4955e9bca821095ef69125837133b5/tools/server/webui',
        'repository': 'charlie12345/ROCmFPX', 'commit': '61f2f2d7bc4955e9bca821095ef69125837133b5',
        'path': 'tools/server/webui and server build scripts', 'blob': None, 'label': 'PRIMARY-DEPENDENCY-DECLARATION',
        'body': '''Key blobs:\n\n- `tools/server/webui/package.json` — `2338c38402a0ad01fb85a24c646cd7ad316f21b4`.\n- `tools/server/webui/package-lock.json` — `bf23307b82c2489a680e9da3f11efed455149246`, lockfile v3.\n- `scripts/webui-download.cmake` — `47c4a784d2fd10aad077111002b020752115b433`.\n- `tools/server/CMakeLists.txt` — `9778e6341575858471aa28571ce658d6f62d3d00`.\n- `scripts/xxd.cmake` — `14d2753808a8e2fd7709b87d9568e982e7192fb1`.\n- Web UI publishing workflow — `bf0d707a23e33a6e65593cd4786afcd5bc6e49e3`.\n\nThe server build can obtain a cached/local npm build or a Hugging Face bucket build, then embed `index.html`, `bundle.js`, `bundle.css`, and `loading.html` as generated C++ headers. The download path does not itself acquire a third-party notice bundle. A binary release must retain exact asset hashes, source tree, lockfile, build command/environment, npm license inventory, and source-to-generated-header-to-binary mapping. Mutable `latest` bucket outputs are blocked unless pinned and verified.''',
    },
    {
        'dst': ROOT / 'raw/github/ROCmFPX/webui-direct-dependencies.capture.md',
        'title': 'ROCmFPX Web UI direct runtime dependencies from exact package-lock blob',
        'source_url': 'https://github.com/charlie12345/ROCmFPX/blob/61f2f2d7bc4955e9bca821095ef69125837133b5/tools/server/webui/package-lock.json',
        'repository': 'charlie12345/ROCmFPX', 'commit': '61f2f2d7bc4955e9bca821095ef69125837133b5',
        'path': 'tools/server/webui/package-lock.json', 'blob': 'bf23307b82c2489a680e9da3f11efed455149246', 'label': 'PRIMARY-DEPENDENCY-DECLARATION',
        'body': '''The exact resolved direct dependency records are exported separately in `manifests/webui-direct-dependencies.csv` and `.json`. License fields in package-lock are primary package metadata, not a substitute for retaining package license texts or checking bundled/transitive content.''',
    },
]

for spec in connector_specs:
    captures.append(write_connector_capture(
        spec['dst'], spec['title'], spec['source_url'], spec['repository'], spec['commit'], spec['path'],
        spec['blob'], spec['label'], spec['body']))

upstream_capture_specs = [
    (
        'Qwen3-0.6B model card license evidence',
        'https://huggingface.co/Qwen/Qwen3-0.6B',
        'OFFICIAL-PUBLISHER-MODEL-CARD',
        '''Publisher-controlled model page identifies `Qwen/Qwen3-0.6B` and labels the repository `apache-2.0`. The local fixture name `Qwen3-0.6B-Q4_K_M.gguf` does not identify which of many quantization repositories or revisions supplied its bytes. Base-model evidence is therefore insufficient to approve the local quantized file.'''
    ),
    (
        'DeepSeek-V4-Flash model card and license evidence',
        'https://huggingface.co/deepseek-ai/DeepSeek-V4-Flash',
        'OFFICIAL-PUBLISHER-MODEL-CARD',
        '''Publisher-controlled model page labels the repository/model release MIT and describes the current Flash model as 284B parameters. The ROCmFPX local fixture path says `DeepSeek-V4-Flash-180B`; that size mismatch prevents a source identification. Do not treat the current model card as proof for the local fixture bytes.'''
    ),
    (
        'Tencent Hy3 model card license evidence',
        'https://huggingface.co/tencent/Hy3',
        'OFFICIAL-PUBLISHER-MODEL-CARD',
        '''Publisher-controlled current Hy3 page states Apache License 2.0. An earlier `Hy3-preview` release used a distinct Tencent Hy Community License. Because the generator used mutable `main` and the checked-in Jinja template lacks a revision/source header, the exact template source and license remain version-sensitive.'''
    ),
    (
        'Project Gutenberg license and trademark policy evidence',
        'https://www.gutenberg.org/policy/license.html',
        'OFFICIAL-PUBLISHER-TERMS',
        '''Project Gutenberg explains that many ebook texts are unrestricted in the United States, while the Project Gutenberg name/license/trademark terms are separate; users outside the United States must check local law. The benchmark script downloads a specific served text but does not pin or hash it. Reference or independently reacquire the exact edition; do not promote an unrecorded local cache.'''
    ),
    (
        'AMD ROCm component licensing guidance',
        'https://rocm.docs.amd.com/en/latest/about/license.html',
        'OFFICIAL-VENDOR-LICENSING-GUIDANCE',
        '''AMD's ROCm licensing guidance states that ROCm is licensed per component and can include third-party/additional terms. A parent project's GPL or MIT file does not relicense downloaded ROCm SDK/toolchain/runtime packages. Capture package names, versions, installed license directories, and shipped shared objects in the release SBOM.'''
    ),
    (
        'Git submodule/gitlink semantics',
        'https://git-scm.com/book/en/v2/Git-Tools-Submodules',
        'OFFICIAL-VCS-DOCUMENTATION',
        '''Git documentation describes a submodule as an independent repository with its own history; the superproject records the exact commit as a gitlink. This is the technical boundary reflected by `llama-ai` pinning `CachyLLama@6be745...`. License scope and release combination effects remain a human determination.'''
    ),
    (
        'GNU GPL version 3 canonical terms',
        'https://www.gnu.org/licenses/gpl-3.0.en.html',
        'OFFICIAL-LICENSE-CANON',
        '''Canonical GPLv3 text defines source code, object code, Corresponding Source, conveyance of non-source forms, notice retention, and source-access mechanisms. The exact repository LICENSE is separately captured and hash-verified.'''
    ),
    (
        'CC BY-NC-SA 4.0 canonical terms',
        'https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.en',
        'OFFICIAL-LICENSE-CANON',
        '''Canonical CC BY-NC-SA 4.0 legal code includes attribution, NonCommercial, and ShareAlike conditions. The `llama-ai` README's separate documentation label is therefore not interchangeable with GPL code terms or MIT engine terms.'''
    ),
    (
        'SPDX 2.3 package and relationship data model',
        'https://spdx.github.io/spdx-spec/v2.3/',
        'OFFICIAL-STANDARD',
        '''SPDX provides package/file license fields, `NOASSERTION`, checksums, external references, and relationships. This dossier includes an SPDX seed but does not claim to be a release-complete SBOM.'''
    ),
    (
        'SLSA provenance v1.2',
        'https://slsa.dev/spec/v1.2/provenance',
        'OFFICIAL-STANDARD',
        '''SLSA provenance records where, when, and how artifacts were produced and binds attestations to artifact identities. Use it, or an equivalent signed build attestation, to bind source, generator, environment, and output hashes for release artifacts.'''
    ),
    (
        'CISA 2025 Minimum Elements for an SBOM',
        'https://www.cisa.gov/resources-tools/resources/2025-minimum-elements-software-bill-materials-sbom',
        'OFFICIAL-GOVERNMENT-GUIDANCE',
        '''CISA's 2025 minimum-elements resource is a baseline for release SBOM content. The exact release must still enumerate all distributed packages, generated assets, linked libraries, model/test data, and external components.'''
    ),
]

for i, (title, url, label, body) in enumerate(upstream_capture_specs, 1):
    captures.append(write_connector_capture(
        ROOT / f'raw/upstream/{i:02d}-{title.lower().replace(" ", "-").replace("/", "-")}.capture.md',
        title, url, None, None, None, None, label, body,
        fidelity='dated public-page text capture; page may be mutable'))

# ---------------------------------------------------------------------------
# Manifests
# ---------------------------------------------------------------------------
components: list[dict[str, Any]] = [
    {
        'component_id': 'rocmfpx-a5605a7',
        'name': 'ROCmFPX',
        'version_or_commit': 'a5605a72768c6562241b248e268e33dc92787394',
        'component_type': 'source-repository-snapshot',
        'repository': 'https://github.com/charlie12345/ROCmFPX',
        'root_license_assertion': 'MIT',
        'license_conclusion': 'NOASSERTION (mixed file-level exceptions; exact proposed tree not scanned here)',
        'primary_license_blob': 'e7dca554bcb802f98408383a864404e3aa4eacca',
        'notice_blob': '4b2f877fe5011ac7eca70b5409a1cb7b032109ea',
        'promotion_status': 'CANDIDATE-WITH-NOTICE-AND-FULL-TREE-SCAN',
        'boundary_note': 'Repository source only; models, generated GGUFs, Web UI bundles, toolchains, and downloaded assets are separate.',
    },
    {
        'component_id': 'rocmfpx-61f2f2d',
        'name': 'ROCmFPX',
        'version_or_commit': '61f2f2d7bc4955e9bca821095ef69125837133b5',
        'component_type': 'source-repository-snapshot',
        'repository': 'https://github.com/charlie12345/ROCmFPX',
        'root_license_assertion': 'MIT',
        'license_conclusion': 'NOASSERTION (mixed exceptions and two newly added unheadered HIP files)',
        'primary_license_blob': 'e7dca554bcb802f98408383a864404e3aa4eacca',
        'notice_blob': '4b2f877fe5011ac7eca70b5409a1cb7b032109ea',
        'promotion_status': 'CANDIDATE-WITH-NOTICE-AND-FULL-TREE-SCAN',
        'boundary_note': 'One commit after a560; changed-file origin/SPDX review required.',
    },
    {
        'component_id': 'cachyllama-6be7459',
        'name': 'CachyLLama',
        'version_or_commit': '6be745998f568e379ea197fcf827baec73ff9940',
        'component_type': 'source-repository-snapshot',
        'repository': 'https://github.com/fewtarius/CachyLLama',
        'root_license_assertion': 'MIT',
        'license_conclusion': 'NOASSERTION (file-level exceptions; no root third-party notice file at commit)',
        'primary_license_blob': 'e7dca554bcb802f98408383a864404e3aa4eacca',
        'notice_blob': 'ABSENT',
        'promotion_status': 'CANDIDATE-WITH-GENERATED-NOTICE-AND-FULL-TREE-SCAN',
        'boundary_note': 'Independent repository pinned by parent gitlink.',
    },
    {
        'component_id': 'llama-ai-code-1017f3d',
        'name': 'llama-ai operational code',
        'version_or_commit': '1017f3dfdce3ca2b06aa9007b23295db3bb35722',
        'component_type': 'source-repository-code-scope',
        'repository': 'https://github.com/fewtarius/llama-ai',
        'root_license_assertion': 'GPL-3.0-or-later',
        'license_conclusion': 'GPL-3.0-or-later for sampled operational code; exact file-scope scan still required',
        'primary_license_blob': 'f288702d2fa16d3cdf0035b15a9fcbc552cd88e7',
        'notice_blob': 'README scope statement + file SPDX headers',
        'promotion_status': 'GPL-RELEASE-MODEL-HUMAN-DECISION',
        'boundary_note': 'Do not relabel as MIT because it builds or invokes an MIT submodule.',
    },
    {
        'component_id': 'llama-ai-docs-1017f3d',
        'name': 'llama-ai documentation',
        'version_or_commit': '1017f3dfdce3ca2b06aa9007b23295db3bb35722',
        'component_type': 'repository-documentation-scope',
        'repository': 'https://github.com/fewtarius/llama-ai',
        'root_license_assertion': 'CC-BY-NC-SA-4.0 (README statement)',
        'license_conclusion': 'CC-BY-NC-SA-4.0 for documentation as asserted; per-file scope not exhaustively scanned',
        'primary_license_blob': 'README cb8f9dee239db6df865ee08d0b7008eb5f8b71be',
        'notice_blob': 'README license section',
        'promotion_status': 'REFERENCE-ONLY-BY-DEFAULT',
        'boundary_note': 'NonCommercial and ShareAlike terms are separate from code licensing.',
    },
    {
        'component_id': 'llama-ai-gitlink-cachyllama',
        'name': 'llama-ai/CachyLLama gitlink',
        'version_or_commit': '6be745998f568e379ea197fcf827baec73ff9940',
        'component_type': 'gitlink-submodule-boundary',
        'repository': 'git@github.com:fewtarius/CachyLLama.git',
        'root_license_assertion': 'NOASSERTION at gitlink; child repository separately asserts MIT',
        'license_conclusion': 'Technical pin only; combination/distribution analysis is release-model dependent',
        'primary_license_blob': 'gitlink object; .gitmodules c73a9ad6ca6261b42aea9c2d5107fd24298066bb',
        'notice_blob': 'none at boundary',
        'promotion_status': 'SEPARATE-COMPONENT-RECORD-REQUIRED',
        'boundary_note': 'Superproject does not contain child file blobs in its tree; release archives may materialize them.',
    },
    {
        'component_id': 'rocmfpx-webui',
        'name': 'ROCmFPX server Web UI source and generated bundle',
        'version_or_commit': '61f2f2d7bc4955e9bca821095ef69125837133b5',
        'component_type': 'source-plus-generated-embedded-assets',
        'repository': 'ROCmFPX/tools/server/webui',
        'root_license_assertion': 'Repository MIT assertion plus npm package-specific licenses',
        'license_conclusion': 'NOASSERTION until full npm inventory/license texts and exact bundle hash map are captured',
        'primary_license_blob': 'package-lock bf23307b82c2489a680e9da3f11efed455149246',
        'notice_blob': 'not generated by upstream workflow',
        'promotion_status': 'INDEPENDENT-REGENERATION-CANDIDATE',
        'boundary_note': 'Mutable bucket/latest output is blocked; local pinned rebuild is preferred.',
    },
    {
        'component_id': 'model-and-tokenizer-assets',
        'name': 'Models, GGUFs, tokenizers, chat templates, generated vocab fixtures',
        'version_or_commit': 'various/unpinned',
        'component_type': 'data-model-generated-assets',
        'repository': 'Hugging Face/local paths',
        'root_license_assertion': 'publisher/model/repository specific',
        'license_conclusion': 'NOASSERTION',
        'primary_license_blob': 'not captured for exact local bytes',
        'notice_blob': 'not captured',
        'promotion_status': 'BLOCKED-PENDING-EXACT-PROVENANCE-AND-PERMISSION',
        'boundary_note': 'Base model license evidence does not prove a third-party quantized artifact or extracted template.',
    },
    {
        'component_id': 'gutenberg-pg1184',
        'name': 'Project Gutenberg ebook 1184 benchmark text',
        'version_or_commit': 'served file unpinned by script',
        'component_type': 'test-corpus-reference',
        'repository': 'Project Gutenberg',
        'root_license_assertion': 'Project Gutenberg public-domain/terms framework; jurisdiction-sensitive',
        'license_conclusion': 'NOASSERTION for a redistributed local cache until exact bytes and terms record are captured',
        'primary_license_blob': 'none',
        'notice_blob': 'Project Gutenberg terms must be reviewed with exact edition',
        'promotion_status': 'REFERENCE-OR-INDEPENDENT-REACQUISITION',
        'boundary_note': 'Do not ship the unrecorded cached copy merely because the script labels it public domain.',
    },
    {
        'component_id': 'rocm-toolchains-and-runtime',
        'name': 'ROCm SDK, compiler, runtime, Vulkan loader, system packages',
        'version_or_commit': 'release-environment-specific',
        'component_type': 'distribution-dependencies',
        'repository': 'AMD/distro/vendor packages',
        'root_license_assertion': 'per component/package',
        'license_conclusion': 'NOASSERTION',
        'primary_license_blob': 'must be collected from exact installed/shipped packages',
        'notice_blob': 'must be collected',
        'promotion_status': 'BLOCKED-UNTIL-RELEASE-SBOM',
        'boundary_note': 'Parent repository licenses do not relicense externally downloaded toolchains.',
    },
]

files = [
    # ROCmFPX core and notices
    {'file_id':'f-001','component_id':'rocmfpx-a5605a7,rocmfpx-61f2f2d','repository':'charlie12345/ROCmFPX','commit':'a5605a7 and 61f2f2d','path':'LICENSE','git_blob':'e7dca554bcb802f98408383a864404e3aa4eacca','evidence_kind':'full license text','declared_license':'MIT','concluded_license':'MIT for repository assertion','claim_label':'PRIMARY-REPO-LICENSE-TEXT','capture_path':'raw/github/ROCmFPX/LICENSE.blob-e7dca554bcb802f98408383a864404e3aa4eacca.txt','promotion_status':'CANDIDATE-WITH-NOTICE','reviewer_action':'Do not use as a substitute for exception scan.'},
    {'file_id':'f-002','component_id':'rocmfpx-a5605a7,rocmfpx-61f2f2d','repository':'charlie12345/ROCmFPX','commit':'a5605a7 and 61f2f2d','path':'THIRD_PARTY_NOTICES.md','git_blob':'4b2f877fe5011ac7eca70b5409a1cb7b032109ea','evidence_kind':'notice register','declared_license':'MIT components listed','concluded_license':'Incomplete notice inventory','claim_label':'PRIMARY-FILE-NOTICE','capture_path':'raw/github/ROCmFPX/THIRD_PARTY_NOTICES.blob-4b2f877fe5011ac7eca70b5409a1cb7b032109ea.md','promotion_status':'RETAIN-AND-SUPPLEMENT','reviewer_action':'Add backend exceptions and exact release dependencies.'},
    {'file_id':'f-003','component_id':'rocmfpx-a5605a7,rocmfpx-61f2f2d','repository':'charlie12345/ROCmFPX','commit':'a5605a7 and 61f2f2d','path':'README.md','git_blob':'2a446c96ecb2bf7fc8510c09c325141bd0152061','evidence_kind':'repository provenance assertion','declared_license':'MIT credit to llama.cpp','concluded_license':'Repository assertion only','claim_label':'PRIMARY-REPOSITORY-ASSERTION','capture_path':'raw/github/ROCmFPX/README.blob-2a446c96ecb2bf7fc8510c09c325141bd0152061.md','promotion_status':'REFERENCE-EVIDENCE','reviewer_action':'Verify specific donor files against history.'},
    {'file_id':'f-004','component_id':'rocmfpx-61f2f2d','repository':'charlie12345/ROCmFPX','commit':'61f2f2d7bc4955e9bca821095ef69125837133b5','path':'docs/UPSTREAM-ATTRIBUTION.md','git_blob':'1870f3bbdb04432b0ec56d2d74a2fe2895d3cdfc','evidence_kind':'maintainer attribution register','declared_license':'n/a','concluded_license':'Provenance assertion, not independent conclusion','claim_label':'PRIMARY-UPSTREAM-ATTRIBUTION-ASSERTION','capture_path':'raw/github/ROCmFPX/UPSTREAM-ATTRIBUTION.blob-1870f3bbdb04432b0ec56d2d74a2fe2895d3cdfc.capture.md','promotion_status':'REFERENCE-EVIDENCE','reviewer_action':'Trace every cited commit and compare proposed files.'},
    {'file_id':'f-005','component_id':'rocmfpx-61f2f2d','repository':'charlie12345/ROCmFPX','commit':'61f2f2d7bc4955e9bca821095ef69125837133b5','path':'vendor/cpp-httplib/LICENSE','git_blob':'3e5ed359a2bb16f52c383745c25f14bb7a81c9e4','evidence_kind':'bundled license text','declared_license':'MIT','concluded_license':'MIT','claim_label':'PRIMARY-REPO-LICENSE-TEXT','capture_path':'not byte-captured; exact blob recorded','promotion_status':'CANDIDATE-WITH-NOTICE','reviewer_action':'Include copyright and text.'},
    {'file_id':'f-006','component_id':'rocmfpx-61f2f2d','repository':'charlie12345/ROCmFPX','commit':'61f2f2d7bc4955e9bca821095ef69125837133b5','path':'licenses/LICENSE-jsonhpp','git_blob':'b5a10275c1cdff6d2d1b4d84f51922d42499dec5','evidence_kind':'bundled license text','declared_license':'MIT','concluded_license':'MIT','claim_label':'PRIMARY-REPO-LICENSE-TEXT','capture_path':'not byte-captured; exact blob recorded','promotion_status':'CANDIDATE-WITH-NOTICE','reviewer_action':'Include copyright and text.'},
    {'file_id':'f-007','component_id':'rocmfpx-61f2f2d','repository':'charlie12345/ROCmFPX','commit':'61f2f2d7bc4955e9bca821095ef69125837133b5','path':'gguf-py/LICENSE','git_blob':'76f67efdc6470081b512a8db5bf2b1d4962d9c3c','evidence_kind':'bundled license text','declared_license':'MIT','concluded_license':'MIT','claim_label':'PRIMARY-REPO-LICENSE-TEXT','capture_path':'not byte-captured; exact blob recorded','promotion_status':'CANDIDATE-WITH-NOTICE','reviewer_action':'Include copyright and text.'},
    {'file_id':'f-008','component_id':'rocmfpx-61f2f2d','repository':'charlie12345/ROCmFPX','commit':'61f2f2d7bc4955e9bca821095ef69125837133b5','path':'ggml/src/ggml-sycl/common.cpp','git_blob':'ae08abad81bb8cbeffea19a90afa189f648653aa','evidence_kind':'file SPDX and embedded notice','declared_license':'MIT and Apache-2.0 WITH LLVM-exception sections','concluded_license':'Mixed within file','claim_label':'PRIMARY-FILE-SPDX','capture_path':'raw/github/ROCmFPX/file-level-exceptions.capture.md','promotion_status':'CANDIDATE-WITH-EXCEPTION-NOTICE','reviewer_action':'Preserve applicable notices/exception text; inspect full file boundaries.'},
    {'file_id':'f-009','component_id':'rocmfpx-61f2f2d,cachyllama-6be7459','repository':'both','commit':'exact commits','path':'ggml/src/ggml-cpu/kleidiai/kleidiai.h','git_blob':'38eac58f7c207ded2a74db705c75f92e6e6eb66c','evidence_kind':'file header','declared_license':'MIT','concluded_license':'MIT','claim_label':'PRIMARY-FILE-SPDX','capture_path':'raw/github/ROCmFPX/file-level-exceptions.capture.md','promotion_status':'CANDIDATE-WITH-NOTICE','reviewer_action':'Preserve Arm copyright.'},
    {'file_id':'f-010','component_id':'rocmfpx-61f2f2d','repository':'charlie12345/ROCmFPX','commit':'61f2f2d...','path':'ggml/src/ggml-openvino/openvino/frontend.h','git_blob':'f1c6f0c3e3ce3eaebb8afbcbc91a01b236778f1b','evidence_kind':'file SPDX','declared_license':'Apache-2.0','concluded_license':'Apache-2.0','claim_label':'PRIMARY-FILE-SPDX','capture_path':'raw/github/ROCmFPX/file-level-exceptions.capture.md','promotion_status':'CANDIDATE-WITH-APACHE-NOTICE','reviewer_action':'Include Apache text and any NOTICE if present upstream.'},
    {'file_id':'f-011','component_id':'rocmfpx-61f2f2d','repository':'charlie12345/ROCmFPX','commit':'61f2f2d...','path':'common/base64.hpp','git_blob':'563247a6e5f7dba837c07a509026d8b36e61387c','evidence_kind':'file dedication','declared_license':'Unlicense','concluded_license':'Unlicense/public-domain dedication','claim_label':'PRIMARY-FILE-NOTICE','capture_path':'raw/github/ROCmFPX/file-level-exceptions.capture.md','promotion_status':'CANDIDATE-WITH-TEXT','reviewer_action':'Retain dedication/Unlicense text.'},
    {'file_id':'f-012','component_id':'rocmfpx-61f2f2d','repository':'charlie12345/ROCmFPX','commit':'61f2f2d...','path':'examples/gguf-hash/deps/sha256/sha256.c','git_blob':'a7a87aeb20032c7f9e87491af927b9074037202a','evidence_kind':'file public-domain notice','declared_license':'Public domain','concluded_license':'Public-domain notices as stated','claim_label':'PRIMARY-FILE-NOTICE','capture_path':'raw/github/ROCmFPX/file-level-exceptions.capture.md','promotion_status':'CANDIDATE-WITH-NOTICE','reviewer_action':'Retain provenance notice.'},
    {'file_id':'f-013','component_id':'rocmfpx-61f2f2d','repository':'charlie12345/ROCmFPX','commit':'61f2f2d...','path':'ggml/src/ggml-hip/fattn-kv-batched.cu','git_blob':'0aa5ddb57f295df482ebbf4603641fc072b419b3','evidence_kind':'unheadered added source','declared_license':'Root MIT assertion only','concluded_license':'NOASSERTION pending origin/history review','claim_label':'UNRESOLVED-ORIGIN','capture_path':'raw/github/ROCmFPX/61f-delta-new-HIP-files.capture.md','promotion_status':'HUMAN-REVIEW-REQUIRED','reviewer_action':'Trace authorship/source; add accurate SPDX only after confirmation.'},
    {'file_id':'f-014','component_id':'rocmfpx-61f2f2d','repository':'charlie12345/ROCmFPX','commit':'61f2f2d...','path':'ggml/src/ggml-hip/fattn-vec-turbo-mixed.cu','git_blob':'5bb0577b1a2b0154b7bde487dc5b7fd21ff2ee78','evidence_kind':'unheadered added source','declared_license':'Root MIT assertion only','concluded_license':'NOASSERTION pending origin/history review','claim_label':'UNRESOLVED-ORIGIN','capture_path':'raw/github/ROCmFPX/61f-delta-new-HIP-files.capture.md','promotion_status':'HUMAN-REVIEW-REQUIRED','reviewer_action':'Trace authorship/source; add accurate SPDX only after confirmation.'},
    # Cachy
    {'file_id':'f-015','component_id':'cachyllama-6be7459','repository':'fewtarius/CachyLLama','commit':'6be745998f568e379ea197fcf827baec73ff9940','path':'LICENSE','git_blob':'e7dca554bcb802f98408383a864404e3aa4eacca','evidence_kind':'full license text','declared_license':'MIT','concluded_license':'MIT repository assertion','claim_label':'PRIMARY-REPO-LICENSE-TEXT','capture_path':'raw/github/CachyLLama/LICENSE.blob-e7dca554bcb802f98408383a864404e3aa4eacca.txt','promotion_status':'CANDIDATE-WITH-GENERATED-NOTICE','reviewer_action':'Full-tree exception scan required.'},
    {'file_id':'f-016','component_id':'cachyllama-6be7459','repository':'fewtarius/CachyLLama','commit':'6be745...','path':'README.md','git_blob':'2a19230f301821314d4b061a1ae9dfd00c0254e1','evidence_kind':'repository assertion','declared_license':'MIT, additions same unless noted','concluded_license':'Repository assertion only','claim_label':'PRIMARY-REPOSITORY-ASSERTION','capture_path':'raw/github/CachyLLama/README.blob-2a19230f301821314d4b061a1ae9dfd00c0254e1.md','promotion_status':'REFERENCE-EVIDENCE','reviewer_action':'Do not infer absence of exceptions.'},
    {'file_id':'f-017','component_id':'cachyllama-6be7459','repository':'fewtarius/CachyLLama','commit':'6be745...','path':'THIRD_PARTY_NOTICES.md','git_blob':'ABSENT','evidence_kind':'absence at exact path','declared_license':'n/a','concluded_license':'Notice gap','claim_label':'UNRESOLVED-NOTICE-GAP','capture_path':'raw/github/CachyLLama/file-level-exceptions.capture.md','promotion_status':'GENERATE-BEFORE-RELEASE','reviewer_action':'Create notice inventory from exact tree and upstream sources.'},
    {'file_id':'f-018','component_id':'cachyllama-6be7459','repository':'fewtarius/CachyLLama','commit':'6be745...','path':'ggml/src/ggml-sycl/common.cpp','git_blob':'e1b6db13eb41575a57a1c60b3ed1a2a42bef3afd','evidence_kind':'file SPDX and embedded notice','declared_license':'MIT and Apache-2.0 WITH LLVM-exception sections','concluded_license':'Mixed within file','claim_label':'PRIMARY-FILE-SPDX','capture_path':'raw/github/CachyLLama/file-level-exceptions.capture.md','promotion_status':'CANDIDATE-WITH-EXCEPTION-NOTICE','reviewer_action':'Preserve notices and exception text.'},
    {'file_id':'f-019','component_id':'cachyllama-6be7459','repository':'fewtarius/CachyLLama','commit':'6be745...','path':'ggml/src/ggml-openvino/openvino/frontend.h','git_blob':'72134a3e8cf2980729fbb0d73c71c5160af4dc30','evidence_kind':'file SPDX','declared_license':'Apache-2.0','concluded_license':'Apache-2.0','claim_label':'PRIMARY-FILE-SPDX','capture_path':'raw/github/CachyLLama/file-level-exceptions.capture.md','promotion_status':'CANDIDATE-WITH-APACHE-NOTICE','reviewer_action':'Include Apache text and upstream notice if applicable.'},
    # llama-ai
    {'file_id':'f-020','component_id':'llama-ai-code-1017f3d','repository':'fewtarius/llama-ai','commit':'1017f3dfdce3ca2b06aa9007b23295db3bb35722','path':'LICENSE','git_blob':'f288702d2fa16d3cdf0035b15a9fcbc552cd88e7','evidence_kind':'full license text','declared_license':'GPL-3.0','concluded_license':'GPL-3.0-or-later scope asserted by README/SPDX headers','claim_label':'PRIMARY-REPO-LICENSE-TEXT','capture_path':'raw/github/llama-ai/LICENSE.blob-f288702d2fa16d3cdf0035b15a9fcbc552cd88e7.txt','promotion_status':'GPL-RELEASE-MODEL-HUMAN-DECISION','reviewer_action':'Determine exact covered work and corresponding-source delivery.'},
    {'file_id':'f-021','component_id':'llama-ai-code-1017f3d,llama-ai-docs-1017f3d','repository':'fewtarius/llama-ai','commit':'1017f3d...','path':'README.md','git_blob':'cb8f9dee239db6df865ee08d0b7008eb5f8b71be','evidence_kind':'scope statement','declared_license':'GPL-3.0-or-later code; CC-BY-NC-SA-4.0 documentation','concluded_license':'Separate code/documentation scopes asserted','claim_label':'PRIMARY-REPOSITORY-ASSERTION','capture_path':'raw/github/llama-ai/README.blob-cb8f9dee239db6df865ee08d0b7008eb5f8b71be.md','promotion_status':'SPLIT-SCOPE','reviewer_action':'Classify each proposed file; do not merge terms.'},
    {'file_id':'f-022','component_id':'llama-ai-gitlink-cachyllama','repository':'fewtarius/llama-ai','commit':'1017f3d...','path':'.gitmodules','git_blob':'c73a9ad6ca6261b42aea9c2d5107fd24298066bb','evidence_kind':'submodule config','declared_license':'none','concluded_license':'Technical repository boundary only','claim_label':'PRIMARY-GITLINK-CONFIG','capture_path':'raw/github/llama-ai/.gitmodules.blob-c73a9ad6ca6261b42aea9c2d5107fd24298066bb','promotion_status':'SEPARATE-COMPONENT-RECORD-REQUIRED','reviewer_action':'Package child source/license separately and map binary interaction.'},
    {'file_id':'f-023','component_id':'llama-ai-code-1017f3d','repository':'fewtarius/llama-ai','commit':'1017f3d...','path':'llama-run.sh','git_blob':'3684a299361b3ca45ff57656a43073f8b9629047','evidence_kind':'file SPDX','declared_license':'GPL-3.0-or-later','concluded_license':'GPL-3.0-or-later','claim_label':'PRIMARY-FILE-SPDX','capture_path':'raw/github/llama-ai/gpl-file-headers.capture.md','promotion_status':'GPL-SOURCE-OR-CORRESPONDING-SOURCE','reviewer_action':'Retain SPDX/copyright; include modified-source notices if changed.'},
    {'file_id':'f-024','component_id':'llama-ai-code-1017f3d','repository':'fewtarius/llama-ai','commit':'1017f3d...','path':'scripts/benchmark.sh','git_blob':'6d4743a25906a271f8eef17bbe5e153c3d430704','evidence_kind':'file SPDX plus fixture URL','declared_license':'GPL-3.0-or-later for script','concluded_license':'Script GPL; downloaded corpus separate','claim_label':'PRIMARY-FILE-SPDX','capture_path':'raw/github/llama-ai/benchmark-gutenberg-fixture.capture.md','promotion_status':'SCRIPT-CANDIDATE-UNDER-GPL; CORPUS-SEPARATE','reviewer_action':'Pin/hash corpus if distributed.'},
    # Generated/assets
    {'file_id':'f-025','component_id':'rocmfpx-webui','repository':'charlie12345/ROCmFPX','commit':'61f2f2d...','path':'tools/server/webui/package.json','git_blob':'2338c38402a0ad01fb85a24c646cd7ad316f21b4','evidence_kind':'dependency declaration','declared_license':'private package; repo assertion','concluded_license':'NOASSERTION for aggregate bundle','claim_label':'PRIMARY-DEPENDENCY-DECLARATION','capture_path':'raw/github/ROCmFPX/webui-generation-and-lockfile.capture.md','promotion_status':'REGENERATE-WITH-PINNED-LOCKFILE','reviewer_action':'Capture complete runtime/transitive license inventory.'},
    {'file_id':'f-026','component_id':'rocmfpx-webui','repository':'charlie12345/ROCmFPX','commit':'61f2f2d...','path':'tools/server/webui/package-lock.json','git_blob':'bf23307b82c2489a680e9da3f11efed455149246','evidence_kind':'exact dependency graph','declared_license':'per npm entry','concluded_license':'Mixed; see direct dependency manifest; full transitive extraction pending','claim_label':'PRIMARY-DEPENDENCY-DECLARATION','capture_path':'raw/github/ROCmFPX/webui-direct-dependencies.capture.md','promotion_status':'RETAIN-WITH-BUILD-SOURCE','reviewer_action':'Archive package license texts and npm tarball integrity evidence.'},
    {'file_id':'f-027','component_id':'rocmfpx-webui','repository':'charlie12345/ROCmFPX','commit':'61f2f2d...','path':'generated index.html/bundle.js/bundle.css/loading.html and *.hpp','git_blob':'release-specific','evidence_kind':'generated outputs','declared_license':'inherits applicable source/dependency obligations','concluded_license':'NOASSERTION until exact build','claim_label':'GENERATED-FILE','capture_path':'raw/github/ROCmFPX/webui-generation-and-lockfile.capture.md','promotion_status':'INDEPENDENT-REGENERATION-CANDIDATE','reviewer_action':'Record input hashes, tool versions, commands, output hashes, and binary embedding map.'},
    {'file_id':'f-028','component_id':'model-and-tokenizer-assets','repository':'charlie12345/ROCmFPX','commit':'61f2f2d...','path':'models/templates/tencent-Hy3.jinja','git_blob':'f6185ccbfc00d25b4aaa7caeb26b3db6f4466d3b','evidence_kind':'generated/extracted template without source header','declared_license':'none in file','concluded_license':'NOASSERTION','claim_label':'UNRESOLVED-LICENSE','capture_path':'raw/github/ROCmFPX/tokenizer-template-generation.capture.md','promotion_status':'REFERENCE-ONLY','reviewer_action':'Identify exact publisher revision and template license; regenerate if permitted.'},
    {'file_id':'f-029','component_id':'model-and-tokenizer-assets','repository':'charlie12345/ROCmFPX','commit':'61f2f2d...','path':'generated ggml-vocab-*.gguf','git_blob':'release/local-specific','evidence_kind':'generated tokenizer fixture','declared_license':'source tokenizer specific','concluded_license':'NOASSERTION','claim_label':'UNRESOLVED-LICENSE','capture_path':'raw/github/ROCmFPX/tokenizer-template-generation.capture.md','promotion_status':'BLOCKED-PENDING-SOURCE-REVISION-LICENSE-HASH','reviewer_action':'Map tokenizer files and generator to output hash.'},
    {'file_id':'f-030','component_id':'model-and-tokenizer-assets','repository':'local fixture paths','commit':'not identified','path':'Qwen3-0.6B-Q4_K_M.gguf','git_blob':'not in public tree','evidence_kind':'local model fixture reference','declared_license':'unknown exact quant repository','concluded_license':'NOASSERTION','claim_label':'BLOCKED-HUMAN-PERMISSION','capture_path':'raw/github/ROCmFPX/model-fixture-scripts.capture.md','promotion_status':'BLOCKED','reviewer_action':'Resolve publisher/quantizer/revision/license/hash.'},
    {'file_id':'f-031','component_id':'model-and-tokenizer-assets','repository':'local fixture paths','commit':'not identified','path':'DeepSeek-V4-Flash-180B source/output','git_blob':'not in public tree','evidence_kind':'local model fixture reference','declared_license':'unknown exact source; current official release differs in size','concluded_license':'NOASSERTION','claim_label':'BLOCKED-HUMAN-PERMISSION','capture_path':'raw/github/ROCmFPX/model-fixture-scripts.capture.md','promotion_status':'BLOCKED','reviewer_action':'Resolve exact publisher artifact and permission.'},
    {'file_id':'f-032','component_id':'gutenberg-pg1184','repository':'Project Gutenberg','commit':'served bytes unpinned','path':'cache/epub/1184/pg1184.txt','git_blob':'n/a','evidence_kind':'runtime-downloaded test corpus','declared_license':'publisher terms/public-domain framework','concluded_license':'NOASSERTION for unrecorded local cache','claim_label':'REFERENCE-ONLY','capture_path':'raw/upstream/04-project-gutenberg-license-and-trademark-policy-evidence.capture.md','promotion_status':'REFERENCE-OR-INDEPENDENT-REACQUISITION','reviewer_action':'Capture exact bytes/hash/terms and jurisdiction analysis before redistribution.'},
]

claims = [
    {'claim_id':'CL-001','claim_label':'PRIMARY-REPO-LICENSE-TEXT','literal_claim':'ROCmFPX and CachyLLama root LICENSE blobs are byte-identical MIT texts at the exact commits.','evidence':'LICENSE blob e7dca554... exact-byte captures','confidence':'high','human_decision':'Determine file exceptions and scope over exact local tree.'},
    {'claim_id':'CL-002','claim_label':'PRIMARY-REPO-LICENSE-TEXT','literal_claim':'llama-ai root LICENSE blob is GPLv3 text; sampled operational files say GPL-3.0-or-later.','evidence':'LICENSE f288702d...; sampled SPDX blobs','confidence':'high','human_decision':'Determine release combination and corresponding-source method.'},
    {'claim_id':'CL-003','claim_label':'PRIMARY-REPOSITORY-ASSERTION','literal_claim':'llama-ai README separately labels documentation CC-BY-NC-SA-4.0.','evidence':'README cb8f9dee...','confidence':'high for assertion; file-level scope not exhaustively tested','human_decision':'Classify exact proposed documentation files and intended distribution context.'},
    {'claim_id':'CL-004','claim_label':'PRIMARY-GITLINK-PIN','literal_claim':'llama-ai at 1017f3d pins CachyLLama commit 6be7459 via a gitlink and .gitmodules entry.','evidence':'commit diff + .gitmodules c73a9ad...','confidence':'high','human_decision':'Determine combined/aggregate release treatment.'},
    {'claim_id':'CL-005','claim_label':'PRIMARY-FILE-SPDX','literal_claim':'Both MIT repositories contain representative Apache-2.0 and Apache-2.0 WITH LLVM-exception file sections.','evidence':'SYCL/OpenVINO exact file blobs','confidence':'high','human_decision':'Complete full-tree scan and notice bundle.'},
    {'claim_id':'CL-006','claim_label':'UNRESOLVED-NOTICE-GAP','literal_claim':'CachyLLama lacks a root THIRD_PARTY_NOTICES.md at the exact commit.','evidence':'exact path lookup returned absent','confidence':'high','human_decision':'Generate a complete notice file before release.'},
    {'claim_id':'CL-007','claim_label':'PRIMARY-GENERATION-RECIPE','literal_claim':'ROCmFPX Web UI assets can be built or downloaded and embedded into generated C++ headers.','evidence':'CMake/workflow/xxd/package blobs','confidence':'high','human_decision':'Choose pinned local build vs blocked bucket binary and complete dependency notices.'},
    {'claim_id':'CL-008','claim_label':'UNRESOLVED-LICENSE','literal_claim':'Checked-in chat templates and generated tokenizer assets are not proven MIT by repository location alone.','evidence':'mutable-main extraction script, missing template source header, external tokenizer inputs','confidence':'high as a provenance gap','human_decision':'Identify exact sources/licenses and permission.'},
    {'claim_id':'CL-009','claim_label':'BLOCKED-HUMAN-PERMISSION','literal_claim':'Local model fixture paths do not identify exact publisher/quantizer/revision/license/hash.','evidence':'fixture scripts and path names','confidence':'high','human_decision':'Obtain permission or exact licensed source record.'},
    {'claim_id':'CL-010','claim_label':'REFERENCE-ONLY','literal_claim':'GPL behavior, command names, interfaces, performance facts, and requirements may be documented separately from copying GPL expression, subject to human clean-room design.','evidence':'operational scripts + clean-room protocol in this dossier','confidence':'methodological, not a legal conclusion','human_decision':'Approve role separation and admissibility.'},
    {'claim_id':'CL-011','claim_label':'REGENERATE-WITH-PINNED-SOURCE','literal_claim':'Generated files may be independently regenerated only when all inputs, licenses, revisions, tools, and output hashes are recorded.','evidence':'generation recipes + SLSA/SPDX guidance','confidence':'high as release control','human_decision':'Approve exact inputs and build.'},
    {'claim_id':'CL-012','claim_label':'NOASSERTION','literal_claim':'This dossier is an external-evidence seed, not the local full-tree or proposed-artifact scan.','evidence':'scope and limitations','confidence':'high','human_decision':'Perform local twelve-package/exact release scan.'},
]

unresolved = [
    {'unresolved_id':'U-001','subject':'Exact locally proposed twelve-package tree','category':'scope','reason':'Not provided and expressly out of scope','blocking_status':'BLOCKED','required_evidence':'Full file inventory, hashes, symlinks, submodules, generated files, binaries, package metadata.'},
    {'unresolved_id':'U-002','subject':'ROCmFPX newly added HIP files','category':'origin/SPDX','reason':'No file header; only root MIT assertion','blocking_status':'HUMAN-REVIEW','required_evidence':'git history, author/source statement, donor comparison, accurate SPDX/copyright.'},
    {'unresolved_id':'U-003','subject':'ROCmFPX full file-license exceptions','category':'license inventory','reason':'Representative exceptions found; no complete scan performed','blocking_status':'BLOCKED FOR RELEASE','required_evidence':'Exact-tree ScanCode/ORT/FOSSology or equivalent plus human review.'},
    {'unresolved_id':'U-004','subject':'CachyLLama third-party notices','category':'notice gap','reason':'No root notice file; mixed backend licenses found','blocking_status':'BLOCKED FOR BINARY RELEASE','required_evidence':'Complete exception inventory, license texts, attribution/NOTICE.'},
    {'unresolved_id':'U-005','subject':'llama-ai exact documentation scope','category':'separate license scope','reason':'README labels documentation CC-BY-NC-SA but per-file map not exhaustive','blocking_status':'HUMAN-REVIEW','required_evidence':'File-by-file docs classification and intended commercial/noncommercial distribution decision.'},
    {'unresolved_id':'U-006','subject':'GPL covered-work/aggregate determination','category':'release model','reason':'Depends on exact packaging, linking, scripts, interfaces, and distribution','blocking_status':'HUMAN LEGAL DECISION','required_evidence':'Binary composition, dynamic/static links, process boundaries, source delivery plan.'},
    {'unresolved_id':'U-007','subject':'Corresponding Source completeness','category':'GPL release evidence','reason':'No concrete binary/source release supplied','blocking_status':'BLOCKED','required_evidence':'Build scripts, interface definitions, modified sources, dependency sources as applicable, installation information analysis.'},
    {'unresolved_id':'U-008','subject':'Qwen3-0.6B Q4_K_M fixture bytes','category':'model/quantization','reason':'Local filename does not identify quantizer repository/revision','blocking_status':'BLOCKED','required_evidence':'Publisher and quantizer, exact repo commit, model card/license, file SHA256, notices, chain from base model.'},
    {'unresolved_id':'U-009','subject':'DeepSeek-V4-Flash-180B fixture','category':'model','reason':'Current official V4-Flash card is 284B; local 180B path unresolved','blocking_status':'BLOCKED','required_evidence':'Exact artifact identity, publisher, revision, license, file hashes, transformation chain.'},
    {'unresolved_id':'U-010','subject':'Tencent Hy3 chat template','category':'template','reason':'Checked-in template lacks source revision/header; Hy3 licensing changed across versions','blocking_status':'REFERENCE ONLY','required_evidence':'Exact tokenizer_config revision, source license, extracted-value hash, generator hash.'},
    {'unresolved_id':'U-011','subject':'Tokenizer snapshots and ggml-vocab fixtures','category':'tokenizer/generated data','reason':'External source revisions and licenses not captured','blocking_status':'BLOCKED','required_evidence':'Repo revision, all source file hashes/licenses, conversion command/tool version, output hashes.'},
    {'unresolved_id':'U-012','subject':'Project Gutenberg cached text','category':'test corpus','reason':'Script does not pin served bytes or bundle terms/jurisdiction record','blocking_status':'REFERENCE OR REACQUIRE','required_evidence':'Exact bytes/hash, edition metadata, terms capture, trademark handling, jurisdiction approval.'},
    {'unresolved_id':'U-013','subject':'Web UI full transitive dependency inventory','category':'Web UI/SBOM','reason':'Direct dependencies extracted; complete transitive license texts/notice review not performed','blocking_status':'BLOCKED FOR EMBEDDED BUNDLE','required_evidence':'npm-ci exact install, full CycloneDX/SPDX, package tarball integrities, license files, bundle composition.'},
    {'unresolved_id':'U-014','subject':'Web UI mutable bucket latest artifacts','category':'generated binaries','reason':'Mutable source and optional checksum handling; no notice acquisition','blocking_status':'BLOCKED','required_evidence':'Immutable revision/URL, exact hashes, source correspondence, notices, builder provenance.'},
    {'unresolved_id':'U-015','subject':'Web UI authored fixture content','category':'test content','reason':'ai-tutorial.ts is substantial authored-looking content without source header','blocking_status':'HUMAN-REVIEW','required_evidence':'Origin, author permission, repository history, applicable license.'},
    {'unresolved_id':'U-016','subject':'ROCm SDK/toolchains/runtime packages','category':'distribution dependencies','reason':'Per-component/package licenses not captured','blocking_status':'BLOCKED IF DISTRIBUTED','required_evidence':'Exact package names/versions/files, installed license texts, shared-library mapping, SBOM.'},
    {'unresolved_id':'U-017','subject':'Generated source/header/binary mapping','category':'source-to-binary','reason':'No concrete release artifacts supplied','blocking_status':'BLOCKED','required_evidence':'Compiler/linker commands, object/archive map, embedded asset map, hashes, reproducible build/provenance.'},
    {'unresolved_id':'U-018','subject':'Clean-room role separation','category':'process','reason':'No human roles/access records supplied','blocking_status':'HUMAN DECISION','required_evidence':'Approved protocol, role assignments, access logs, dated requirements, independent implementation records.'},
]

promotion = [
    {'artifact_class':'ROCmFPX/CachyLLama MIT-origin source files with no contrary file notice','default_status':'CANDIDATE-WITH-NOTICE','may_reference':'yes','may_independently_regenerate':'n/a','may_copy_or_promote':'Only after exact-tree origin/license scan and notice preservation','blocking_condition':'Any unknown or contrary file-level provenance'},
    {'artifact_class':'MIT adaptations from identified upstream commits','default_status':'CANDIDATE-WITH-DONOR-TRACE','may_reference':'yes','may_independently_regenerate':'by clean re-port from pinned licensed source','may_copy_or_promote':'After source/diff/license/copyright record','blocking_condition':'Unresolved donor commit or incompatible file exception'},
    {'artifact_class':'llama-ai GPL operational scripts and service/build files','default_status':'GPL-SCOPE','may_reference':'yes','may_independently_regenerate':'Potentially from behavior/spec under approved clean-room process','may_copy_or_promote':'Only under a release model satisfying GPL obligations','blocking_condition':'Attempt to copy into non-GPL target without permission/compatible release model'},
    {'artifact_class':'GPL operational behavior, interfaces, command semantics, facts, measurements','default_status':'REFERENCE-ONLY-FOR-REQUIREMENTS','may_reference':'yes','may_independently_regenerate':'yes, with documented independent implementation and human approval','may_copy_or_promote':'Do not copy expressive code/text as “facts”','blocking_condition':'Role contamination or expression-level similarity'},
    {'artifact_class':'llama-ai documentation','default_status':'REFERENCE-ONLY-BY-DEFAULT','may_reference':'yes with attribution','may_independently_regenerate':'Create new documentation from independently verified facts','may_copy_or_promote':'Only under CC-BY-NC-SA-4.0-compatible distribution approved by reviewer','blocking_condition':'Commercial/permissive release conflict, missing attribution/share-alike handling'},
    {'artifact_class':'Schemas/grammars/protocol descriptions','default_status':'CLASSIFY-BY-EXPRESSION','may_reference':'yes','may_independently_regenerate':'facts/ideas and independently authored schemas may be regenerated','may_copy_or_promote':'Copied schema text requires source/license evidence','blocking_condition':'Unidentified copied metaschema or prose/code expression'},
    {'artifact_class':'Generated Web UI bundle and embedded headers','default_status':'INDEPENDENT-REGENERATION-CANDIDATE','may_reference':'yes','may_independently_regenerate':'yes from exact source+lockfile+toolchain if all licenses allow','may_copy_or_promote':'Only with bundle hash map, source, notices, and SBOM','blocking_condition':'Mutable bucket/latest or incomplete dependency notices'},
    {'artifact_class':'Chat templates extracted from tokenizer_config.json','default_status':'REFERENCE-ONLY','may_reference':'yes','may_independently_regenerate':'only from exact pinned source with compatible license','may_copy_or_promote':'Not approved on root-repo MIT assertion alone','blocking_condition':'Unknown source revision/license or version-sensitive publisher terms'},
    {'artifact_class':'Tokenizer snapshots and generated ggml-vocab GGUF','default_status':'BLOCKED-PENDING-PROVENANCE','may_reference':'yes','may_independently_regenerate':'yes after exact source/license/revision capture','may_copy_or_promote':'Only with full source-to-output chain','blocking_condition':'Unknown tokenizer files/license/revision'},
    {'artifact_class':'Model weights and quantized GGUF fixtures','default_status':'BLOCKED-PENDING-PERMISSION','may_reference':'yes','may_independently_regenerate':'only when base model, quantizer, terms, and transformation allow','may_copy_or_promote':'No blanket approval','blocking_condition':'Unknown exact publisher/quantizer/revision/hash or use restrictions'},
    {'artifact_class':'Project Gutenberg benchmark corpus','default_status':'REFERENCE-OR-REACQUIRE','may_reference':'yes','may_independently_regenerate':'reacquire exact edition and record hash/terms','may_copy_or_promote':'Only after jurisdiction/trademark/terms review','blocking_condition':'Unrecorded cache or non-US uncertainty'},
    {'artifact_class':'ROCm SDK/compiler/runtime/Vulkan/system packages','default_status':'DISTRIBUTION-DEPENDENCY','may_reference':'yes','may_independently_regenerate':'build from exact source packages where feasible','may_copy_or_promote':'Only under each package license with SBOM/notices','blocking_condition':'Parent license assumption or missing package license inventory'},
]

webui_deps = [
    {'package':'@modelcontextprotocol/sdk','version':'1.26.0','license_field':'MIT','integrity':'sha512-Y5RmPncpiDtTXDbLKswIJzTqu2hyBKxTNsgKqKclDbhIgg1wgtf1fRuvxgTnRfcnxtvvgbIEcqUOzZrJ6iSReg==','scope':'runtime direct','evidence_blob':'bf23307b82c2489a680e9da3f11efed455149246'},
    {'package':'highlight.js','version':'11.11.1','license_field':'BSD-3-Clause','integrity':'sha512-Xwwo44whKBVCYoliBQwaPvtd/2tYFkRQtXDWj1nackaV2JPXx3L0+Jvd8/qCJ2p+ML0/XVkJ2q+Mr+UVdpJK5w==','scope':'runtime direct','evidence_blob':'bf23307b82c2489a680e9da3f11efed455149246'},
    {'package':'mode-watcher','version':'1.1.0','license_field':'MIT','integrity':'sha512-mUT9RRGPDYenk59qJauN1rhsIMKBmWA3xMF+uRwE8MW/tjhaDSCCARqkSuDTq8vr4/2KcAxIGVjACxTjdk5C3g==','scope':'runtime direct','evidence_blob':'bf23307b82c2489a680e9da3f11efed455149246'},
    {'package':'pdfjs-dist','version':'5.4.54','license_field':'Apache-2.0','integrity':'sha512-TBAiTfQw89gU/Z4LW98Vahzd2/LoCFprVGvGbTgFt+QCB1F+woyOPmNNVgLa6djX9Z9GGTnj7qE1UzpOVJiINw==','scope':'runtime direct','evidence_blob':'bf23307b82c2489a680e9da3f11efed455149246'},
    {'package':'rehype-highlight','version':'7.0.2','license_field':'MIT','integrity':'sha512-k158pK7wdC2qL3M5NcZROZ2tR/l7zOzjxXd5VGdcfIyoijjQqpHd3JKtYSBDpDZ38UI2WJWuFAtkMDxmx5kstA==','scope':'runtime direct','evidence_blob':'bf23307b82c2489a680e9da3f11efed455149246'},
    {'package':'rehype-stringify','version':'10.0.1','license_field':'MIT','integrity':'sha512-k9ecfXHmIPuFVI61B9DeLPN0qFHfawM6RsuX48hoqlaKSF61RskNjSm1lI8PhBEM0MRdLxVVm4WmTqJQccH9mA==','scope':'runtime direct','evidence_blob':'bf23307b82c2489a680e9da3f11efed455149246'},
    {'package':'remark','version':'15.0.1','license_field':'MIT','integrity':'sha512-Eht5w30ruCXgFmxVUSlNWQ9iiimq07URKeFS3hNc8cUWy1llX4KDWfyEDZRycMc+znsN9Ux5/tJ/BFdgdOwA3A==','scope':'runtime direct','evidence_blob':'bf23307b82c2489a680e9da3f11efed455149246'},
    {'package':'remark-breaks','version':'4.0.0','license_field':'MIT','integrity':'sha512-IjEjJOkH4FuJvHZVIW0QCDWxcG96kCq7An/KVH2NfJe6rKZU2AsHeB3OEjPNRxi4QC34Xdx7I2KGYn6IpT7gxQ==','scope':'runtime direct','evidence_blob':'bf23307b82c2489a680e9da3f11efed455149246'},
    {'package':'remark-gfm','version':'4.0.1','license_field':'MIT','integrity':'sha512-1quofZ2RQ9EWdeN34S79+KExV1764+wCUGop5CPL1WGdD0ocPpu91lzPGbwWMECpEpd42kJGQwzRfyov9j4yNg==','scope':'runtime direct','evidence_blob':'bf23307b82c2489a680e9da3f11efed455149246'},
    {'package':'remark-html','version':'16.0.1','license_field':'MIT','integrity':'sha512-B9JqA5i0qZe0Nsf49q3OXyGvyXuZFDzAP2iOFLEumymuYJITVpiH1IgsTEwTpdptDmZlMDMWeDmSawdaJIGCXQ==','scope':'runtime direct','evidence_blob':'bf23307b82c2489a680e9da3f11efed455149246'},
    {'package':'remark-rehype','version':'11.1.2','license_field':'MIT','integrity':'sha512-Dh7l57ianaEoIpzbp0PC9UKAdCSVklD8E5Rpw7ETfbTl3FqcOOgq5q2LVDhgGCkaBv7p24JXikPdvhhmHvKMsw==','scope':'runtime direct','evidence_blob':'bf23307b82c2489a680e9da3f11efed455149246'},
    {'package':'svelte-sonner','version':'1.0.5','license_field':'MIT','integrity':'sha512-9dpGPFqKb/QWudYqGnEz93vuY+NgCEvyNvxoCLMVGw6sDN/3oVeKV1xiEirW2E1N3vJEyj5imSBNOGltQHA7mg==','scope':'runtime direct','evidence_blob':'bf23307b82c2489a680e9da3f11efed455149246'},
    {'package':'unist-util-visit','version':'5.0.0','license_field':'MIT','integrity':'sha512-MR04uvD+07cwl/yhVuVWAtw+3GOR/knlL55Nd/wAdblk27GCVt3lqpTivy/tkJcZoNPzTwS1Y+KMojlLDhoTzg==','scope':'runtime direct','evidence_blob':'bf23307b82c2489a680e9da3f11efed455149246'},
    {'package':'zod','version':'4.2.1','license_field':'MIT','integrity':'sha512-0wZ1IRqGGhMP76gLqz8EyfBXKk0J2qo2+H3fi4mcUP/KtTocoX08nmIAHl1Z2kJIZbZee8KOpBCSNPRgauucjw==','scope':'runtime direct','evidence_blob':'bf23307b82c2489a680e9da3f11efed455149246'},
]

write_csv(ROOT/'manifests/components.csv', components)
write_json(ROOT/'manifests/components.json', components)
write_csv(ROOT/'manifests/files.csv', files)
write_json(ROOT/'manifests/files.json', files)
write_csv(ROOT/'manifests/claims.csv', claims)
write_json(ROOT/'manifests/claims.json', claims)
write_csv(ROOT/'manifests/unresolved.csv', unresolved)
write_json(ROOT/'manifests/unresolved.json', unresolved)
write_csv(ROOT/'manifests/promotion-matrix.csv', promotion)
write_json(ROOT/'manifests/promotion-matrix.json', promotion)
write_csv(ROOT/'manifests/webui-direct-dependencies.csv', webui_deps)
write_json(ROOT/'manifests/webui-direct-dependencies.json', webui_deps)

# Capture index after all raw captures have been created.
# Include sidecars as indexed records but exclude the index itself.
raw_records = []
for p in sorted((ROOT/'raw').rglob('*')):
    if p.is_file():
        b = p.read_bytes()
        raw_records.append({
            'path': str(p.relative_to(ROOT)),
            'sha256': sha256_bytes(b),
            'size_bytes': len(b),
            'git_blob_sha1_if_exact_git_blob': git_blob_sha1(b) if not p.name.endswith('.capture.json') and '.capture.' not in p.name and not p.name.endswith('.md.capture.json') else '',
        })
write_csv(ROOT/'raw/capture-index.csv', raw_records)
write_json(ROOT/'raw/capture-index.json', raw_records)

# Provenance JSON-LD
prov = {
    '@context': {
        'prov': 'http://www.w3.org/ns/prov#',
        'spdx': 'https://spdx.org/rdf/terms#',
        'pf': 'https://example.invalid/pf-ir-04#',
        'type': '@type',
        'id': '@id',
    },
    'id': 'pf:PF-IR-04-provenance',
    'type': 'prov:Bundle',
    'generatedAtTime': CREATED_UTC,
    'accessDate': ACCESS_DATE,
    'entities': [
        {'id':'pf:rocmfpx-a560','type':'prov:Entity','commit':'a5605a72768c6562241b248e268e33dc92787394','licenseDeclared':'MIT','licenseConcluded':'NOASSERTION','status':'candidate-with-full-tree-scan'},
        {'id':'pf:rocmfpx-61f','type':'prov:Entity','commit':'61f2f2d7bc4955e9bca821095ef69125837133b5','licenseDeclared':'MIT','licenseConcluded':'NOASSERTION','status':'candidate-with-full-tree-scan'},
        {'id':'pf:cachyllama-6be','type':'prov:Entity','commit':'6be745998f568e379ea197fcf827baec73ff9940','licenseDeclared':'MIT','licenseConcluded':'NOASSERTION','status':'candidate-with-notice-generation'},
        {'id':'pf:llama-ai-1017-code','type':'prov:Entity','commit':'1017f3dfdce3ca2b06aa9007b23295db3bb35722','licenseDeclared':'GPL-3.0-or-later','status':'human-release-model-decision'},
        {'id':'pf:llama-ai-1017-docs','type':'prov:Entity','commit':'1017f3dfdce3ca2b06aa9007b23295db3bb35722','licenseDeclared':'CC-BY-NC-SA-4.0','status':'reference-only-by-default'},
        {'id':'pf:gitlink-cachyllama','type':'prov:Entity','objectName':'6be745998f568e379ea197fcf827baec73ff9940','status':'separate-component'},
        {'id':'pf:webui-source','type':'prov:Entity','commit':'61f2f2d7bc4955e9bca821095ef69125837133b5','packageJsonBlob':'2338c38402a0ad01fb85a24c646cd7ad316f21b4','lockfileBlob':'bf23307b82c2489a680e9da3f11efed455149246'},
        {'id':'pf:webui-bundle','type':'prov:Entity','status':'regenerate-with-pinned-source','licenseConcluded':'NOASSERTION'},
        {'id':'pf:webui-embedded-headers','type':'prov:Entity','status':'release-specific'},
        {'id':'pf:server-binary','type':'prov:Entity','status':'release-specific'},
        {'id':'pf:qwen-local-fixture','type':'prov:Entity','status':'blocked','licenseConcluded':'NOASSERTION'},
        {'id':'pf:deepseek-local-fixture','type':'prov:Entity','status':'blocked','licenseConcluded':'NOASSERTION'},
        {'id':'pf:hf-tokenizer-snapshot','type':'prov:Entity','status':'blocked-until-pinned'},
        {'id':'pf:chat-template','type':'prov:Entity','status':'reference-only'},
        {'id':'pf:ggml-vocab-fixture','type':'prov:Entity','status':'blocked-until-source-chain'},
        {'id':'pf:gutenberg-pg1184','type':'prov:Entity','status':'reference-or-reacquire'},
    ],
    'activities': [
        {'id':'pf:commit-delta-a560-to-61f','type':'prov:Activity','description':'One-commit ROCmFPX implementation delta'},
        {'id':'pf:npm-build','type':'prov:Activity','description':'Pinned npm build; release-specific environment required'},
        {'id':'pf:xxd-embed','type':'prov:Activity','description':'Convert Web UI assets to generated C++ headers'},
        {'id':'pf:link-server','type':'prov:Activity','description':'Compile/link server binary'},
        {'id':'pf:quantize-model','type':'prov:Activity','description':'Quantize exact licensed model input'},
        {'id':'pf:extract-chat-template','type':'prov:Activity','description':'Extract chat_template from pinned tokenizer_config.json'},
        {'id':'pf:generate-vocab-fixture','type':'prov:Activity','description':'Generate ggml-vocab fixture from exact tokenizer snapshot'},
    ],
    'relations': [
        {'type':'prov:wasRevisionOf','generatedEntity':'pf:rocmfpx-61f','usedEntity':'pf:rocmfpx-a560'},
        {'type':'pf:gitlinkPins','superproject':'pf:llama-ai-1017-code','gitlink':'pf:gitlink-cachyllama','target':'pf:cachyllama-6be'},
        {'type':'prov:wasGeneratedBy','entity':'pf:webui-bundle','activity':'pf:npm-build'},
        {'type':'prov:used','activity':'pf:npm-build','entity':'pf:webui-source'},
        {'type':'prov:wasGeneratedBy','entity':'pf:webui-embedded-headers','activity':'pf:xxd-embed'},
        {'type':'prov:used','activity':'pf:xxd-embed','entity':'pf:webui-bundle'},
        {'type':'prov:wasGeneratedBy','entity':'pf:server-binary','activity':'pf:link-server'},
        {'type':'prov:used','activity':'pf:link-server','entity':'pf:webui-embedded-headers'},
        {'type':'prov:used','activity':'pf:link-server','entity':'pf:cachyllama-6be'},
        {'type':'prov:used','activity':'pf:quantize-model','entity':'pf:qwen-local-fixture','comment':'Input identity unresolved; relation is a required template, not an established fact.'},
        {'type':'prov:wasGeneratedBy','entity':'pf:chat-template','activity':'pf:extract-chat-template'},
        {'type':'prov:used','activity':'pf:extract-chat-template','entity':'pf:hf-tokenizer-snapshot'},
        {'type':'prov:wasGeneratedBy','entity':'pf:ggml-vocab-fixture','activity':'pf:generate-vocab-fixture'},
        {'type':'prov:used','activity':'pf:generate-vocab-fixture','entity':'pf:hf-tokenizer-snapshot'},
    ],
}
prov['profileNote'] = 'W3C PROV-inspired evidence graph using custom pf terms; not a SLSA attestation or legal conclusion.'
prov['@graph'] = prov['entities'] + prov['activities'] + [
    {'id': f'pf:relation-{i:03d}', **relation} for i, relation in enumerate(prov['relations'], start=1)
]
write_json(ROOT/'manifests/provenance-chain.jsonld', prov)

# SPDX seed, deliberately conservative.
spdx = {
    'spdxVersion': 'SPDX-2.3',
    'dataLicense': 'CC0-1.0',
    'SPDXID': 'SPDXRef-DOCUMENT',
    'name': 'PF-IR-04 external evidence SBOM seed',
    'documentNamespace': f'https://example.invalid/spdx/pf-ir-04/{ACCESS_DATE}',
    'creationInfo': {
        'created': CREATED_UTC,
        'creators': ['Tool: OpenAI evidence dossier generator', 'Organization: human review required'],
        'comment': 'External-evidence seed only. Not a complete SBOM for any local or distributed artifact.',
    },
    'documentDescribes': ['SPDXRef-ROCmFPX-61f','SPDXRef-CachyLLama-6be','SPDXRef-llama-ai-code-1017','SPDXRef-llama-ai-docs-1017'],
    'packages': [
        {
            'name':'ROCmFPX','SPDXID':'SPDXRef-ROCmFPX-61f','versionInfo':'61f2f2d7bc4955e9bca821095ef69125837133b5','downloadLocation':'git+https://github.com/charlie12345/ROCmFPX.git@61f2f2d7bc4955e9bca821095ef69125837133b5','filesAnalyzed':False,'licenseDeclared':'MIT','licenseConcluded':'NOASSERTION','copyrightText':'NOASSERTION','externalRefs':[{'referenceCategory':'OTHER','referenceType':'vcs','referenceLocator':'git+https://github.com/charlie12345/ROCmFPX.git@61f2f2d7bc4955e9bca821095ef69125837133b5','comment':'Exact VCS commit locator; not an archive/package checksum.'}],
            'comment':'Root MIT assertion; representative Apache/LLVM/Unlicense/public-domain exceptions found; exact tree scan required.'
        },
        {
            'name':'CachyLLama','SPDXID':'SPDXRef-CachyLLama-6be','versionInfo':'6be745998f568e379ea197fcf827baec73ff9940','downloadLocation':'git+https://github.com/fewtarius/CachyLLama.git@6be745998f568e379ea197fcf827baec73ff9940','filesAnalyzed':False,'licenseDeclared':'MIT','licenseConcluded':'NOASSERTION','copyrightText':'NOASSERTION','externalRefs':[{'referenceCategory':'OTHER','referenceType':'vcs','referenceLocator':'git+https://github.com/fewtarius/CachyLLama.git@6be745998f568e379ea197fcf827baec73ff9940','comment':'Exact VCS commit locator; not an archive/package checksum.'}],
            'comment':'No root third-party notice file at exact commit; representative exceptions found.'
        },
        {
            'name':'llama-ai operational code','SPDXID':'SPDXRef-llama-ai-code-1017','versionInfo':'1017f3dfdce3ca2b06aa9007b23295db3bb35722','downloadLocation':'git+https://github.com/fewtarius/llama-ai.git@1017f3dfdce3ca2b06aa9007b23295db3bb35722','filesAnalyzed':False,'licenseDeclared':'GPL-3.0-or-later','licenseConcluded':'NOASSERTION','copyrightText':'NOASSERTION','externalRefs':[{'referenceCategory':'OTHER','referenceType':'vcs','referenceLocator':'git+https://github.com/fewtarius/llama-ai.git@1017f3dfdce3ca2b06aa9007b23295db3bb35722','comment':'Exact VCS commit locator; not an archive/package checksum.'}],
            'comment':'Sampled operational files have GPL-3.0-or-later SPDX headers; exact file scope and release composition require review.'
        },
        {
            'name':'llama-ai documentation','SPDXID':'SPDXRef-llama-ai-docs-1017','versionInfo':'1017f3dfdce3ca2b06aa9007b23295db3bb35722','downloadLocation':'git+https://github.com/fewtarius/llama-ai.git@1017f3dfdce3ca2b06aa9007b23295db3bb35722','filesAnalyzed':False,'licenseDeclared':'CC-BY-NC-SA-4.0','licenseConcluded':'NOASSERTION','copyrightText':'NOASSERTION',
            'comment':'README scope statement; exact documentation file map not analyzed.'
        },
        {
            'name':'ROCmFPX Web UI npm dependency graph','SPDXID':'SPDXRef-WebUI-NPM','versionInfo':'package-lock blob bf23307b82c2489a680e9da3f11efed455149246','downloadLocation':'NOASSERTION','filesAnalyzed':False,'licenseDeclared':'NOASSERTION','licenseConcluded':'NOASSERTION','copyrightText':'NOASSERTION','comment':'Direct dependency seed included; full transitive package/file SBOM pending.'
        },
        {
            'name':'Models-tokenizers-templates-fixtures','SPDXID':'SPDXRef-ModelAssets','versionInfo':'unresolved','downloadLocation':'NOASSERTION','filesAnalyzed':False,'licenseDeclared':'NOASSERTION','licenseConcluded':'NOASSERTION','copyrightText':'NOASSERTION','comment':'Blocked pending exact publisher, revision, license, file hash, and transformation chain.'
        },
    ],
    'relationships': [
        {'spdxElementId':'SPDXRef-llama-ai-code-1017','relationshipType':'DEPENDS_ON','relatedSpdxElement':'SPDXRef-CachyLLama-6be','comment':'Superproject gitlink pins exact child commit; technical relationship only.'},
        {'spdxElementId':'SPDXRef-ROCmFPX-61f','relationshipType':'DEPENDS_ON','relatedSpdxElement':'SPDXRef-WebUI-NPM','comment':'Web UI source/build depends on the package-lock dependency graph; exact release output not supplied.'},
        {'spdxElementId':'SPDXRef-ROCmFPX-61f','relationshipType':'OTHER','relatedSpdxElement':'SPDXRef-ModelAssets','comment':'Test/generation scripts reference external model/tokenizer/template assets; no distribution approval.'},
    ],
}
write_json(ROOT/'manifests/release-sbom-seed.spdx.json', spdx)

# Templates
source_to_binary_fields = [
    'release_artifact_path','release_artifact_sha256','artifact_type','source_component','source_commit','source_file_or_glob','source_file_sha256_or_git_blob','generator_or_compiler','generator_version','command_and_flags','build_environment_digest','intermediate_object_or_asset','intermediate_sha256','linked_or_embedded_as','license_concluded','notice_ids','corresponding_source_location','reviewer','review_date','decision'
]
write_csv(ROOT/'manifests/source-to-binary-map.template.csv', [], source_to_binary_fields)

cleanroom_rows = [
    {'role':'Source observer / requirements author','may_access':'GPL source, public docs, public behavior','must_not_access_or_do':'Target implementation repository; copy expressive code/text into requirements','required_records':'Access log, dated requirement document, source identifiers, expression-exclusion review'},
    {'role':'Independent implementer','may_access':'Approved functional requirements, public standards, black-box interfaces, test vectors','must_not_access_or_do':'GPL donor source or restricted documentation if protocol excludes it','required_records':'Repository ACL evidence, independent design notes, implementation commits, attestation'},
    {'role':'Validation team','may_access':'Donor executable/service and independent implementation binaries, approved tests','must_not_access_or_do':'Transfer donor source snippets to implementers','required_records':'Black-box comparison logs, test data provenance, defect reports stripped of donor expression'},
    {'role':'Build/release engineer','may_access':'Approved source, build inputs, dependency packages','must_not_access_or_do':'Change legal conclusions or introduce unrecorded assets','required_records':'SBOM, source-to-binary map, build provenance, hashes, notice bundle'},
    {'role':'Maintainer/legal reviewer','may_access':'All evidence as authorized','must_not_access_or_do':'Delegate final admissibility to this dossier','required_records':'Signed decisions on roles, release model, permissions, unresolved items, exceptions'},
]
write_csv(ROOT/'manifests/clean-room-role-matrix.template.csv', cleanroom_rows)
write_json(ROOT/'manifests/clean-room-role-matrix.template.json', cleanroom_rows)

# ---------------------------------------------------------------------------
# Wiki content
# ---------------------------------------------------------------------------
readme = f'''# PF-IR-04 — Donor, test-asset, and release-artifact licensing evidence

**Evidence date:** {ACCESS_DATE}  
**Priority:** P0  
**Decision target:** external-evidence portion of OPEN-LIC-01  
**Disposition:** evidence package only; human admissibility and release decisions remain open

This folder is a self-contained, offline “LLM Wiki” dossier for the exact public revisions:

- `charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394`
- `charlie12345/ROCmFPX@61f2f2d7bc4955e9bca821095ef69125837133b5`
- `fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940`
- `fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722`

Open `index.html` for the styled navigation. Machine-readable records are in `manifests/`; raw and line-normalized captures are in `raw/`; release-control templates are in `release/`.

## Literal disposition labels

- `CANDIDATE-WITH-NOTICE`: public evidence identifies a permissive license, but exact-tree scanning and notice assembly remain mandatory.
- `REFERENCE-ONLY`: use as evidence or requirements input; no copying approval is made.
- `INDEPENDENT-REGENERATION-CANDIDATE`: rebuild from pinned, licensed inputs with a complete provenance record.
- `BLOCKED-PENDING-EXACT-PROVENANCE-AND-PERMISSION`: do not promote the bytes without human resolution.
- `NOASSERTION`: available evidence is insufficient for a license conclusion.

## Scope limit

This is not legal advice, not a permission grant, not a replacement for a local full-tree/proposed-artifact scan, and not a release-complete SBOM. It assumes no access to the local twelve-package intake. The exact locally proposed source tree, generated assets, binaries, package metadata, models, and distribution method must be reviewed separately.
'''
write_text(ROOT/'README.md', readme)

wiki_pages: dict[str, tuple[str,str]] = {}
wiki_pages['00-scope-and-limits'] = ('Scope, method, and limits', f'''# Scope, method, and limits

## Question answered

This dossier assembles public primary-source evidence relevant to donor code, test assets, generated artifacts, submodule boundaries, and concrete source/binary release controls for the exact revisions listed in the root README.

## Evidence hierarchy

1. Exact-byte repository captures verified against Git blob IDs.
2. Exact commit metadata, gitlink targets, and file blob IDs returned by the public repository interface.
3. File-level SPDX/copyright/notice text.
4. Repository-level license and provenance assertions.
5. Publisher-controlled model cards and official license/standards pages, dated {ACCESS_DATE}.
6. Explicit inference, always labeled and never treated as permission.

## Deliberate exclusions

- No local twelve-package intake was available.
- No exact release binary, archive, container, package, model, or locally proposed artifact tree was supplied.
- No complete full-tree ScanCode/ORT/FOSSology scan was performed.
- No originality, substantial-similarity, derivative-work, aggregation, linking, or jurisdiction conclusion is made.
- No unknown-license asset is approved for copying.

## Capture fidelity

Exact raw files are accompanied by sidecar `.capture.json` records and verified Git blob IDs. Connector captures are line-normalized extracts with exact commit/path/blob metadata where available. Mutable public pages are labeled as dated captures rather than immutable source revisions.
''')

wiki_pages['01-decision-summary'] = ('Decision summary', '''# Decision summary

## What the external evidence unblocks

- Candidate MIT donor records for the two ROCmFPX snapshots and the CachyLLama snapshot.
- Concrete evidence that repository-level MIT labels are incomplete without file-level exception and notice handling.
- A technical and licensing evidence boundary between the GPL `llama-ai` operational layer and the separately pinned MIT `CachyLLama` gitlink target.
- A conservative promotion matrix for templates, tokenizers, models, test corpora, Web UI outputs, and distribution dependencies.
- Release-control requirements for SBOM, source-to-binary mapping, corresponding source, notices, generated assets, and clean-room roles.

## High-impact findings

| Finding | Evidence status | Default disposition |
|---|---|---|
| ROCmFPX root license is MIT at both commits | exact-byte, same Git blob | candidate, subject to exceptions/full-tree scan |
| CachyLLama root license is MIT | exact-byte Git blob | candidate, but create notice inventory |
| `llama-ai` operational files are GPL-3.0-or-later | root license + repeated file SPDX | GPL release-model decision |
| `llama-ai` documentation is separately asserted CC-BY-NC-SA-4.0 | exact README | reference-only by default |
| `CachyLLama` is a gitlink to exact commit `6be745...` | exact `.gitmodules` + commit diff | separate component record |
| SYCL/OpenVINO backend files include Apache/LLVM exceptions | exact file blobs/headers | preserve terms and supplement notices |
| Models/tokenizers/templates are not licensed by repository placement alone | generation/download scripts | blocked or reference-only |
| Web UI is generated and embedded; direct dependencies are mixed MIT/BSD/Apache | exact package-lock blob and build files | pinned regeneration with SBOM/notices |

## Human decisions still required

Final admissibility, release aggregation/linking, GPL corresponding-source method, clean-room role design, documentation distribution, model/test-data permission, and the exact source/binary release model remain human decisions over the locally proposed tree.
''')

wiki_pages['02-component-boundaries'] = ('Component and gitlink boundaries', '''# Component and gitlink boundaries

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
''')

wiki_pages['03-code-license-evidence'] = ('Code, SPDX, and notice evidence', '''# Code, SPDX, and notice evidence

## ROCmFPX

The root MIT license, README, and third-party notice files are unchanged between the two requested revisions. The later revision introduces two unheadered HIP files; those files have only the repository-wide MIT assertion unless a history/origin review establishes more specific evidence.

The tree also contains representative exceptions:

- Apache-2.0 WITH LLVM-exception sections in SYCL material.
- Apache-2.0 OpenVINO files.
- MIT KleidiAI material with Arm copyright.
- Unlicense/public-domain and separate public-domain notices.

The existing third-party notice file lists major MIT dependencies but does not fully enumerate these backend exceptions.

## CachyLLama

The root MIT license and README assertion are clear. The exact commit has no root `THIRD_PARTY_NOTICES.md`, while the same representative backend exceptions are present. A release notice must be generated from the exact tree.

## llama-ai

The root GPLv3 text and repeated `GPL-3.0-or-later` file headers support a GPL operational-code scope. The README separately labels documentation CC-BY-NC-SA-4.0 and references the MIT engine. These scopes must remain separate in manifests and notices.

## File-level manifest rule

Use repository-level declarations as one evidence layer. For each proposed file, record the strongest applicable evidence in this order: explicit SPDX/header, colocated license/notice, documented donor file history, repository assertion, then `NOASSERTION`.
''')

wiki_pages['04-fixtures-models-tokenizers'] = ('Fixtures, models, tokenizers, and templates', '''# Fixtures, models, tokenizers, templates, schemas, and corpora

## Models and quantized GGUFs

ROCmFPX fixture scripts refer to maintainer-local Qwen and DeepSeek paths and generate derived GGUFs. They do not record publisher revision, source license, file hash, quantizer identity, or permission. Current publisher pages can narrow candidate families but cannot prove the local bytes.

- Current Qwen3-0.6B page: Apache-2.0. Local `Q4_K_M` producer/revision remains unknown.
- Current DeepSeek-V4-Flash page: MIT, but current model size conflicts with the local `180B` path.
- No model or quantized fixture is approved for promotion by this dossier.

## Tokenizers and generated vocabulary fixtures

Tokenizer tests and conversion scripts consume external tokenizer repositories and generate `ggml-vocab-*.gguf`. Required record:

`publisher repo → exact revision → source files/hashes → source license/notice → generator blob/version/command → output hash → proposed release path`.

## Chat templates

The extraction script pulls `chat_template` from mutable `main` `tokenizer_config.json` files. Checked-in Jinja output must be treated as publisher-specific expression until the exact source revision and terms are captured. Tencent Hy3 is specifically version-sensitive because current Hy3 and an earlier preview used different terms.

## Schemas and grammars

Implementation ideas, protocol facts, field names required for interoperability, and independently authored tests should be recorded separately from copied source/schema/prose expression. Any copied JSON Schema, metaschema, tutorial, or fixture needs exact source and license evidence.

## Project Gutenberg corpus

The benchmark script downloads ebook 1184 at runtime. Project Gutenberg's public-domain and trademark/terms framework is jurisdiction-sensitive. Reference the fixture or independently reacquire and hash the exact edition; do not promote an unidentified cache.
''')

wiki_pages['05-webui-generated-assets'] = ('Web UI and generated assets', '''# Web UI, npm dependencies, and generated embedded assets

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
''')

wiki_pages['06-release-evidence'] = ('Concrete release evidence', '''# Evidence required for a concrete source or binary release

## Artifact identity

Record every distributed file, archive, package, container layer, installer payload, model/data file, shared library, generated asset, documentation file, and license/notice file with path, size, SHA-256, media type, and origin.

## SBOM

Generate both source and binary SBOM views. At minimum include component name, supplier, exact version/commit, checksums, download/source locator, license declared, license concluded, copyright, relationship, and reviewer annotation. Use `NOASSERTION` rather than guessing.

This dossier's `release-sbom-seed.spdx.json` is only a seed for the public components.

## Source-to-binary mapping

Populate `manifests/source-to-binary-map.template.csv` for:

- each compiled object/archive/shared library;
- generated Web UI asset and generated header;
- statically embedded license/notice resources;
- scripts/configuration/service files installed beside binaries;
- model/tokenizer/template/corpus files included in packages.

## GPL corresponding-source evidence

For any release the human reviewer determines is subject to GPL object-code conveyance, preserve the preferred form for modification and the scripts/interface definitions/build controls required by the applicable terms. Record the chosen source-delivery method next to the binary offer/access point. Determine separately whether installation information is applicable.

## Notices

Ship exact license texts and attribution/NOTICE material required by each included file/package. Do not rely on a root MIT file to cover Apache, LLVM exception, BSD, ISC, Unlicense, public-domain, model, documentation, or package-specific records.

## Build provenance

Bind source commits, submodule pins, toolchain packages, build parameters, environment digest, outputs, and attestations. A reproducible build comparison is preferred; discrepancies must be explained.
''')

wiki_pages['07-gpl-separation-clean-room'] = ('GPL separation and clean-room controls', '''# GPL operational behavior and clean-room role separation

## Distinguish expression from requirements

The GPL scripts are primary evidence of executable operational behavior and source expression. A clean-room effort may use approved observations, interoperability facts, command semantics, test results, and independently written requirements without silently copying GPL code, comments, or documentation expression. Whether a particular process is adequate is a human legal/maintainer decision.

## Minimum role controls

1. **Source observer / requirements author:** documents behavior and facts, with exact donor citations, while excluding code/text expression from the requirements package.
2. **Independent implementer:** works only from approved requirements, standards, and test vectors; has no donor-source access under the protocol.
3. **Validator:** performs black-box or approved comparative tests without passing donor snippets to implementers.
4. **Build/release:** maps approved source to binaries and assembles SBOM/notices.
5. **Maintainer/legal reviewer:** approves roles, access, exceptions, final admissibility, and release model.

## Evidence to retain

- Repository ACLs and access logs.
- Named role assignments and conflict declarations.
- Dated source-observation notes and expression-exclusion review.
- Requirements/test-vector provenance.
- Independent design and commit history.
- Communication channel separation.
- Black-box validation logs.
- Final reviewer signoff.

The role matrix is in `manifests/clean-room-role-matrix.template.csv`.
''')

wiki_pages['08-unresolved-blocked'] = ('Unresolved and blocked items', '''# Unresolved-license and permission register

The machine-readable register is `manifests/unresolved.csv` and `.json`. The following remain blocked or require human review:

- Exact local twelve-package/proposed-artifact tree.
- Full exception scans for ROCmFPX and CachyLLama.
- Origin/SPDX of the two new ROCmFPX HIP files.
- Complete CachyLLama notice inventory.
- GPL covered-work, aggregation/linking, corresponding-source, and installation-information decisions.
- Exact scope of CC-BY-NC-SA documentation.
- Qwen/DeepSeek model fixture identity and permission.
- Tencent Hy3 template source revision and terms.
- Tokenizer and generated vocabulary provenance.
- Project Gutenberg exact bytes, terms, and jurisdiction handling.
- Full Web UI transitive dependency and bundled-content review.
- Mutable bucket `latest` assets.
- ROCm/toolchain/runtime package licensing.
- Concrete source-to-binary and reproducible-build evidence.
- Approved clean-room roles and access controls.

Unknown does not mean permissive. Every unresolved item remains `NOASSERTION` or blocked until a human reviewer resolves it.
''')

wiki_pages['09-maintainer-runbook'] = ('Maintainer and reviewer runbook', '''# Maintainer and reviewer runbook

## Intake

1. Freeze the exact proposed artifact tree and assign a release identifier.
2. Hash every file and record symlinks, gitlinks, submodules, LFS pointers, archives, and generated outputs.
3. Export exact Git status, commits, submodule status, and build environment.

## Scan and reconcile

4. Run a full-tree license/origin scan and compare it with `manifests/files.*`.
5. Reconcile every exception, dual license, generated file, copied fixture, and missing header.
6. Resolve each row in `manifests/unresolved.*`; never replace `NOASSERTION` with a guess.

## Build and map

7. Rebuild from clean, pinned inputs.
8. Generate source and binary SBOMs.
9. Populate the source-to-binary map, including embedded Web UI assets.
10. Capture build provenance and reproduce outputs where feasible.

## Obligations and notices

11. Determine the release/distribution model and all corresponding-source obligations.
12. Assemble license texts, copyright notices, attribution, Apache/LLVM exception material, npm package notices, model/data terms, and documentation notices.
13. Verify that source-access links/offers remain adjacent to corresponding binaries where required.

## Approval

14. Approve clean-room roles and review contamination evidence.
15. Record maintainer/legal signoffs for admissibility, permissions, release model, and residual risk.
16. Archive the final SBOM, hashes, notices, sources, binaries, provenance, and decisions together.
''')

wiki_pages['10-source-register'] = ('Source and capture register', '''# Source and capture register

## Repository captures

See `raw/capture-index.csv` for every capture's SHA-256 and size. Exact-byte Git files have `.capture.json` sidecars recording source URL, commit/path, Git blob SHA-1, access date, and literal claim label.

## Primary public repositories

- `charlie12345/ROCmFPX` at `a5605a7...` and `61f2f2d...`
- `fewtarius/CachyLLama` at `6be7459...`
- `fewtarius/llama-ai` at `1017f3d...`

## Official upstream/public sources

Dated captures include publisher-controlled Hugging Face pages, Project Gutenberg policy, AMD ROCm licensing guidance, Git submodule documentation, canonical GPL and Creative Commons terms, SPDX, SLSA, and CISA SBOM guidance.

## Literal claim labels

Every raw or normalized capture has a label such as `PRIMARY-REPO-LICENSE-TEXT`, `PRIMARY-FILE-SPDX`, `PRIMARY-GITLINK-CONFIG`, `OFFICIAL-PUBLISHER-MODEL-CARD`, `UNRESOLVED-LICENSE`, or `NOASSERTION`. Labels describe the nature of evidence; they are not legal conclusions.
''')

for slug, (title, content) in wiki_pages.items():
    write_text(ROOT/f'wiki/{slug}.md', content)

# ---------------------------------------------------------------------------
# Release templates and checklists
# ---------------------------------------------------------------------------
write_text(ROOT/'release/NOTICE.template.md', '''# NOTICE — [RELEASE NAME / VERSION]

> Populate only from the exact distributed artifact tree and approved license conclusions.

## Main project notices

- Component:
- Version/commit:
- Copyright:
- License:
- Source location:

## File-level and bundled exceptions

For each exception, record component, path(s), exact version/blob, copyright, license/exception, required notice text, and shipped license-file path.

## Web UI and JavaScript packages

Attach the generated npm license report, exact lockfile hash, bundle asset hashes, and source location.

## Models, tokenizers, templates, schemas, and test corpora

List only assets actually distributed. Include publisher, exact revision, file hashes, license/terms, attribution, use restrictions, and transformation chain.

## Build/runtime distribution dependencies

List shipped compiler/runtime/shared-library/package components and their notices. Do not list host-only tools unless the release policy requires them.

## Modifications

State material modifications and dates where required by applicable terms.
''')

write_text(ROOT/'release/RELEASE-REVIEW-CHECKLIST.md', '''# Release review checklist

## Identity and scope

- [ ] Exact proposed source/binary artifact tree frozen and hashed.
- [ ] Submodule/gitlink commits and recursive materialization recorded.
- [ ] Generated files and downloaded assets identified.
- [ ] Local twelve-package intake scanned; external dossier reconciled.

## Licensing and provenance

- [ ] File-level license/SPDX/origin scan complete.
- [ ] Every `NOASSERTION` resolved or excluded.
- [ ] Donor commits and adaptation diffs reviewed.
- [ ] Models/tokenizers/templates/corpora have publisher revision, license, hash, and permission record.
- [ ] Documentation files classified separately from code.

## Build and binary

- [ ] Source and binary SBOMs generated.
- [ ] Source-to-binary map complete.
- [ ] Web UI source/lockfile/assets/generated headers mapped and hashed.
- [ ] Toolchains/runtimes/shared libraries inventoried.
- [ ] Reproducibility or variance report complete.

## Obligations

- [ ] GPL release model and Corresponding Source plan approved.
- [ ] Installation Information applicability reviewed.
- [ ] License texts and notices assembled.
- [ ] Modified-source notices and dates added where required.
- [ ] Source access/offer placed next to binaries where applicable.

## Process

- [ ] Clean-room roles and access restrictions approved where used.
- [ ] Contamination review and validation logs complete.
- [ ] Maintainer and legal reviewer decisions signed and archived.
''')

write_text(ROOT/'release/CORRESPONDING-SOURCE-CHECKLIST.md', '''# Corresponding Source evidence checklist

This checklist records evidence; it does not decide whether a particular artifact is covered.

- [ ] Preferred form for modification of every covered source file.
- [ ] Exact versions of generated source inputs and generators.
- [ ] Build/install/control scripts.
- [ ] Interface definition files and specifically required linked subprogram source as determined by reviewer.
- [ ] Configuration needed to generate, install, and run the object code.
- [ ] Modified source notices and dates.
- [ ] License text and warranty notices.
- [ ] Source delivery mechanism matching the chosen GPL option.
- [ ] Availability period/operations owner documented.
- [ ] Installation Information analysis for any User Product scenario.
- [ ] Verification that recipients can obtain and build the source without undisclosed private dependencies.
''')

write_text(ROOT/'release/CLEAN-ROOM-PROTOCOL.md', '''# Clean-room protocol template

## Purpose

Define a controlled process for independently implementing approved functional requirements without copying restricted source or documentation expression.

## Approved sources and prohibited sources

List exact URLs/commits, public standards, executable endpoints, allowed observations, and prohibited repositories/files for each role.

## Roles

Use `manifests/clean-room-role-matrix.template.csv`. Name individuals and alternates; record prior exposure.

## Information flow

- Observer produces dated requirements and test vectors.
- Reviewer removes source expression and approves the package.
- Implementer receives only the approved package.
- Validator reports behavioral discrepancies without donor snippets.
- Release engineer handles only approved source and evidence.

## Controls

Repository ACLs, separate accounts/channels, access logging, source-snippet scanning, code-similarity review, signed attestations, and exception escalation.

## Completion evidence

Approved requirements, access logs, independent design, commit history, test results, similarity review, and final human signoff.
''')

write_text(ROOT/'release/ARTIFACT-ACCEPTANCE-FORM.md', '''# Proposed artifact acceptance form

- Release identifier:
- Artifact path/name:
- SHA-256:
- Source component and commit:
- Source file(s)/generator:
- License declared:
- License concluded:
- Copyright/attribution:
- Required license/notice files:
- Source-to-binary map row:
- SBOM package/file IDs:
- Corresponding Source location (if applicable):
- Model/data/template terms record (if applicable):
- Clean-room record (if applicable):
- Unresolved item IDs closed:
- Maintainer decision:
- Legal reviewer decision:
- Decision date:
- Conditions/expiration:
''')

write_text(ROOT/'DATA_DICTIONARY.md', '''# Data dictionary

## Common status fields

- `promotion_status`: conservative operational disposition, not a legal determination.
- `license_declared`: license asserted by the producer/repository/package metadata.
- `license_concluded`: evidence reviewer conclusion; `NOASSERTION` when unresolved.
- `claim_label`: literal classification of the evidence source or gap.
- `git_blob`: Git SHA-1 object identity for exact file content where available.
- `capture_path`: local dossier path to exact or line-normalized evidence.

## Hashes

- Git blob SHA-1 binds a capture to a repository file object.
- SHA-256 binds dossier files and proposed release artifacts.
- `hashes/SHA256SUMS` excludes only the hash index files themselves to avoid circularity.
''')

# ---------------------------------------------------------------------------
# Static LLM Wiki HTML
# ---------------------------------------------------------------------------
css = r'''
:root { --bg:#090b13; --panel:#111522; --panel2:#171c2c; --text:#e9edff; --muted:#99a2bd; --line:#2a3148; --accent:#8d7bff; --accent2:#52d6c7; --warn:#ffca68; --bad:#ff758f; --good:#7ee2a8; --code:#0a0e18; }
* { box-sizing:border-box; }
html { scroll-behavior:smooth; }
body { margin:0; background:radial-gradient(circle at 85% -10%,#27215c 0,transparent 28%),var(--bg); color:var(--text); font:15px/1.65 Inter,ui-sans-serif,system-ui,-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif; }
a { color:#b9afff; text-decoration:none; } a:hover { color:white; text-decoration:underline; }
.shell { display:grid; grid-template-columns:290px minmax(0,1fr); min-height:100vh; }
.sidebar { position:sticky; top:0; height:100vh; overflow:auto; padding:24px 18px; background:rgba(9,11,19,.92); border-right:1px solid var(--line); backdrop-filter:blur(15px); }
.brand { font-weight:800; letter-spacing:.04em; font-size:18px; margin-bottom:4px; }
.brand span { color:var(--accent2); }
.kicker { color:var(--muted); font-size:12px; text-transform:uppercase; letter-spacing:.12em; margin-bottom:18px; }
.search { width:100%; background:var(--panel); color:var(--text); border:1px solid var(--line); border-radius:10px; padding:10px 12px; margin-bottom:18px; outline:none; }
.search:focus { border-color:var(--accent); box-shadow:0 0 0 3px rgba(141,123,255,.15); }
.nav-title { color:var(--muted); font-size:11px; text-transform:uppercase; letter-spacing:.14em; margin:17px 8px 7px; }
.nav a { display:block; padding:8px 10px; border-radius:8px; color:#cbd2ea; font-size:13px; }
.nav a:hover,.nav a.active { background:var(--panel2); color:#fff; text-decoration:none; }
.main { padding:42px min(6vw,76px) 90px; max-width:1500px; width:100%; }
.hero { border:1px solid var(--line); background:linear-gradient(135deg,rgba(141,123,255,.13),rgba(82,214,199,.06)); border-radius:20px; padding:34px; margin-bottom:28px; box-shadow:0 20px 50px rgba(0,0,0,.2); }
h1 { margin:0 0 12px; font-size:clamp(30px,4vw,54px); line-height:1.08; letter-spacing:-.035em; }
h2 { margin-top:42px; padding-top:8px; font-size:26px; letter-spacing:-.02em; }
h3 { margin-top:28px; font-size:19px; }
p,li { color:#d7dcef; }
.muted { color:var(--muted); }
.grid { display:grid; grid-template-columns:repeat(auto-fit,minmax(240px,1fr)); gap:14px; margin:20px 0 28px; }
.card { background:var(--panel); border:1px solid var(--line); border-radius:15px; padding:18px; min-width:0; }
.card h3 { margin:0 0 8px; }
.metric { font-size:30px; font-weight:800; line-height:1; margin-bottom:7px; }
.badge { display:inline-flex; align-items:center; gap:6px; font:700 11px/1 ui-monospace,SFMono-Regular,Menlo,monospace; letter-spacing:.035em; border:1px solid var(--line); border-radius:999px; padding:7px 9px; margin:2px 4px 2px 0; background:#131828; }
.badge.good { color:var(--good); border-color:rgba(126,226,168,.35); }
.badge.warn { color:var(--warn); border-color:rgba(255,202,104,.35); }
.badge.bad { color:var(--bad); border-color:rgba(255,117,143,.35); }
.badge.info { color:#b9afff; border-color:rgba(141,123,255,.4); }
.callout { border-left:4px solid var(--accent); background:var(--panel); padding:14px 18px; border-radius:0 12px 12px 0; margin:20px 0; }
.callout.warn { border-color:var(--warn); } .callout.bad { border-color:var(--bad); }
.table-wrap { overflow:auto; border:1px solid var(--line); border-radius:12px; margin:18px 0; }
table { width:100%; border-collapse:collapse; min-width:760px; background:var(--panel); }
th,td { padding:10px 12px; border-bottom:1px solid var(--line); vertical-align:top; text-align:left; }
th { position:sticky; top:0; background:#171c2c; color:#fff; font-size:12px; text-transform:uppercase; letter-spacing:.06em; }
tr:last-child td { border-bottom:0; }
code { background:var(--code); border:1px solid #242b41; border-radius:5px; padding:2px 5px; color:#d6d0ff; font:13px ui-monospace,SFMono-Regular,Menlo,Consolas,monospace; overflow-wrap:anywhere; }
pre { background:var(--code); border:1px solid var(--line); border-radius:12px; padding:16px; overflow:auto; }
pre code { border:0; padding:0; }
hr { border:0; border-top:1px solid var(--line); margin:36px 0; }
.doc-meta { display:flex; gap:10px; flex-wrap:wrap; margin:14px 0 24px; }
.search-hit { outline:2px solid var(--accent); outline-offset:3px; }
.footer { margin-top:60px; padding-top:24px; border-top:1px solid var(--line); color:var(--muted); font-size:12px; }
@media(max-width:900px){ .shell{grid-template-columns:1fr}.sidebar{position:relative;height:auto;border-right:0;border-bottom:1px solid var(--line)}.main{padding:28px 18px 60px}.nav{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr))}.nav-title{grid-column:1/-1} }
@media print { .sidebar{display:none}.shell{display:block}.main{padding:0;max-width:none}.hero,.card,table{break-inside:avoid}body{background:#fff;color:#000}p,li{color:#111}a{color:#000}.card,.hero,table{border-color:#aaa;background:#fff} }
'''
write_text(ROOT/'assets/style.css', css)

js = r'''
const input = document.querySelector('#wiki-search');
if (input) {
  input.addEventListener('input', () => {
    const q = input.value.trim().toLowerCase();
    document.querySelectorAll('[data-searchable]').forEach(el => {
      const hit = q && el.textContent.toLowerCase().includes(q);
      el.classList.toggle('search-hit', !!hit);
      if (q && !hit && el.matches('tr')) el.style.display = 'none'; else if (el.matches('tr')) el.style.display = '';
    });
  });
}
'''
write_text(ROOT/'assets/wiki.js', js)

markdown = mistune.create_markdown(plugins=['table','strikethrough','task_lists','url'])

NAV = [
    ('index.html','Overview'),
    ('wiki/00-scope-and-limits.html','Scope & limits'),
    ('wiki/01-decision-summary.html','Decision summary'),
    ('wiki/02-component-boundaries.html','Component boundaries'),
    ('wiki/03-code-license-evidence.html','Code & SPDX'),
    ('wiki/04-fixtures-models-tokenizers.html','Models & fixtures'),
    ('wiki/05-webui-generated-assets.html','Web UI & generated'),
    ('wiki/06-release-evidence.html','Release evidence'),
    ('wiki/07-gpl-separation-clean-room.html','GPL & clean room'),
    ('wiki/08-unresolved-blocked.html','Unresolved items'),
    ('wiki/09-maintainer-runbook.html','Reviewer runbook'),
    ('wiki/10-source-register.html','Source register'),
]

page_tpl = Template('''<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>{{ title }} · PF-IR-04</title><link rel="stylesheet" href="{{ prefix }}assets/style.css"></head>
<body><div class="shell"><aside class="sidebar"><div class="brand">PF-IR-04 <span>Wiki</span></div><div class="kicker">Licensing & provenance dossier</div><input id="wiki-search" class="search" placeholder="Search this page…"><div class="nav"><div class="nav-title">Navigation</div>{% for href,label in nav %}<a href="{{ prefix }}{{ href }}" class="{% if label == active %}active{% endif %}">{{ label }}</a>{% endfor %}<div class="nav-title">Machine records</div><a href="{{ prefix }}manifests/components.csv">Components CSV</a><a href="{{ prefix }}manifests/files.csv">Files CSV</a><a href="{{ prefix }}manifests/unresolved.csv">Unresolved CSV</a><a href="{{ prefix }}manifests/provenance-chain.jsonld">Provenance JSON-LD</a><a href="{{ prefix }}manifests/release-sbom-seed.spdx.json">SPDX seed</a><a href="{{ prefix }}hashes/SHA256SUMS">SHA256SUMS</a></div></aside><main class="main"><div class="hero"><div class="kicker">Evidence date {{ access_date }} · P0</div><h1>{{ title }}</h1><div class="doc-meta"><span class="badge info">EXTERNAL EVIDENCE</span><span class="badge warn">HUMAN DECISION REQUIRED</span><span class="badge bad">NO UNKNOWN-LICENSE APPROVAL</span></div></div><article data-searchable>{{ body|safe }}</article><div class="footer">PF-IR-04 dossier · Generated {{ created }} · Not legal advice · Not a local full-tree scan.</div></main></div><script src="{{ prefix }}assets/wiki.js"></script></body></html>''')

for slug, (title, content) in wiki_pages.items():
    body = markdown(content)
    # add search marker to table rows/headings/paragraphs through broad article marker; no need HTML mutation
    html = page_tpl.render(title=title, prefix='../', nav=NAV, active=title, access_date=ACCESS_DATE, created=CREATED_UTC, body=body)
    write_text(ROOT/f'wiki/{slug}.html', html)

# Dashboard tables
status_counts = {
    'components': len(components),
    'file_records': len(files),
    'unresolved': len(unresolved),
    'exact_captures': sum(1 for c in captures if c.get('byte_fidelity') == 'exact'),
}

def html_escape(s: Any) -> str:
    import html
    return html.escape(str(s))

comp_rows = ''.join('<tr data-searchable>' + ''.join(f'<td>{html_escape(r.get(k,""))}</td>' for k in ['name','version_or_commit','root_license_assertion','license_conclusion','promotion_status']) + '</tr>' for r in components)
unres_rows = ''.join('<tr data-searchable>' + ''.join(f'<td>{html_escape(r.get(k,""))}</td>' for k in ['unresolved_id','subject','category','blocking_status','required_evidence']) + '</tr>' for r in unresolved)

index_body = f'''
<div class="grid">
  <div class="card"><div class="metric">{status_counts['components']}</div><div class="muted">component/boundary records</div></div>
  <div class="card"><div class="metric">{status_counts['file_records']}</div><div class="muted">representative file records</div></div>
  <div class="card"><div class="metric">{status_counts['exact_captures']}</div><div class="muted">exact-byte Git captures</div></div>
  <div class="card"><div class="metric">{status_counts['unresolved']}</div><div class="muted">unresolved/blocking entries</div></div>
</div>
<div class="callout bad"><strong>Disposition boundary.</strong> This package does not approve copying where a license remains unknown and does not replace a scan of the exact locally proposed artifact tree.</div>
<h2>Core conclusions</h2>
<div class="grid">
 <div class="card" data-searchable><span class="badge good">MIT EVIDENCE</span><h3>ROCmFPX and CachyLLama</h3><p>Root MIT texts are exact-byte verified, but representative Apache/LLVM/Unlicense/public-domain exceptions require file-level inventory and notices.</p></div>
 <div class="card" data-searchable><span class="badge warn">GPL SEPARATION</span><h3>llama-ai operational layer</h3><p>Root GPL text and repeated GPL-3.0-or-later headers are distinct from the separately pinned MIT engine and CC-licensed documentation.</p></div>
 <div class="card" data-searchable><span class="badge bad">BLOCKED ASSETS</span><h3>Models, tokenizers, templates</h3><p>Local paths and mutable downloads do not establish exact publisher revision, quantizer, license, permission, or hashes.</p></div>
 <div class="card" data-searchable><span class="badge info">REGENERATE</span><h3>Web UI assets</h3><p>Prefer an independently reproducible build from the exact lockfile; ship sources, SBOM, notices, and source-to-binary mapping.</p></div>
</div>
<h2>Component register</h2><div class="table-wrap"><table><thead><tr><th>Name</th><th>Version/commit</th><th>Declared</th><th>Conclusion</th><th>Status</th></tr></thead><tbody>{comp_rows}</tbody></table></div>
<h2>Unresolved register</h2><div class="table-wrap"><table><thead><tr><th>ID</th><th>Subject</th><th>Category</th><th>Status</th><th>Required evidence</th></tr></thead><tbody>{unres_rows}</tbody></table></div>
<h2>Start here</h2>
<div class="grid">
 <div class="card"><h3><a href="wiki/01-decision-summary.html">Decision summary</a></h3><p>What the evidence unblocks and what remains human-controlled.</p></div>
 <div class="card"><h3><a href="wiki/06-release-evidence.html">Release evidence</a></h3><p>SBOM, source-to-binary, Corresponding Source, notices, and provenance.</p></div>
 <div class="card"><h3><a href="wiki/08-unresolved-blocked.html">Blocked items</a></h3><p>Exact permission and provenance gaps that cannot be inferred away.</p></div>
 <div class="card"><h3><a href="wiki/09-maintainer-runbook.html">Reviewer runbook</a></h3><p>Concrete sequence for the exact local release tree.</p></div>
</div>
'''
index_html = page_tpl.render(title='Donor, test-asset, and release-artifact licensing evidence', prefix='', nav=NAV, active='Overview', access_date=ACCESS_DATE, created=CREATED_UTC, body=index_body)
write_text(ROOT/'index.html', index_html)

# Graph file
write_text(ROOT/'manifests/provenance-graph.mmd', '''flowchart LR
  L[llama-ai@1017f3d<br/>GPL code / CC docs] -->|gitlink pin| C[CachyLLama@6be7459<br/>MIT root + exceptions]
  R1[ROCmFPX@a5605a7] -->|one commit| R2[ROCmFPX@61f2f2d]
  W[Web UI source + lockfile] --> N[npm build]
  N --> A[index.html / bundle.js / bundle.css / loading.html]
  A --> X[generated C++ headers]
  X --> B[server binary]
  M[exact licensed model source] --> Q[quantizer + command]
  Q --> G[GGUF fixture]
  T[pinned tokenizer_config] --> E[template extractor]
  E --> J[Jinja template]
  T --> V[vocab generator]
  V --> F[ggml-vocab fixture]
  U[unknown local model/template/tokenizer bytes]:::blocked
  classDef blocked fill:#511b2b,stroke:#ff758f,color:#fff;
''')

# Include build script itself for reproducibility.
shutil.copy2('/tmp/build_pf_ir_04.py', ROOT/'tools/build_pf_ir_04.py')

# ---------------------------------------------------------------------------
# Validation and hashes
# ---------------------------------------------------------------------------
write_text(ROOT/'hashes/SHA256SUMS', '# placeholder; replaced after validation')

# Validate JSON files.
for p in ROOT.rglob('*.json'):
    json.loads(p.read_text(encoding='utf-8'))
for p in ROOT.rglob('*.jsonld'):
    json.loads(p.read_text(encoding='utf-8'))

# Validate internal HTML hrefs that point to local files, allowing anchors.
from html.parser import HTMLParser
class LinkParser(HTMLParser):
    def __init__(self): super().__init__(); self.links=[]
    def handle_starttag(self, tag, attrs):
        if tag == 'a':
            for k,v in attrs:
                if k == 'href' and v: self.links.append(v)

broken=[]
for hp in ROOT.rglob('*.html'):
    parser=LinkParser(); parser.feed(hp.read_text(encoding='utf-8'))
    for href in parser.links:
        if href.startswith(('http://','https://','mailto:','#')): continue
        target=(hp.parent / href.split('#')[0]).resolve()
        if not target.exists(): broken.append((str(hp.relative_to(ROOT)),href))
if broken:
    raise RuntimeError(f'Broken internal links: {broken[:20]}')

# Create hash indexes excluding hash index files themselves.
entries=[]
for p in sorted(ROOT.rglob('*')):
    if not p.is_file(): continue
    rel=str(p.relative_to(ROOT))
    if rel in {'hashes/SHA256SUMS','hashes/file-manifest.json'}: continue
    b=p.read_bytes()
    entries.append({'path':rel,'sha256':sha256_bytes(b),'size_bytes':len(b)})
write_text(ROOT/'hashes/SHA256SUMS', '\n'.join(f"{e['sha256']}  {e['path']}" for e in entries))
write_json(ROOT/'hashes/file-manifest.json', {
    'schema':'pf-ir-04-file-manifest-1.0',
    'created':CREATED_UTC,
    'access_date':ACCESS_DATE,
    'coverage':'All dossier files except hashes/SHA256SUMS and hashes/file-manifest.json to avoid circularity.',
    'files':entries,
})

# Validation report
validation = {
    'created': CREATED_UTC,
    'json_files_validated': len(list(ROOT.rglob('*.json'))) + len(list(ROOT.rglob('*.jsonld'))),
    'html_files_validated': len(list(ROOT.rglob('*.html'))),
    'broken_internal_links': broken,
    'exact_git_captures': [c for c in captures if c.get('byte_fidelity') == 'exact'],
    'total_files_before_zip': len([p for p in ROOT.rglob('*') if p.is_file()]),
}
write_json(ROOT/'hashes/validation-report.json', validation)

# Refresh SHA256SUMS after adding validation report, still excluding indexes.
entries=[]
for p in sorted(ROOT.rglob('*')):
    if not p.is_file(): continue
    rel=str(p.relative_to(ROOT))
    if rel in {'hashes/SHA256SUMS','hashes/file-manifest.json'}: continue
    b=p.read_bytes(); entries.append({'path':rel,'sha256':sha256_bytes(b),'size_bytes':len(b)})
write_text(ROOT/'hashes/SHA256SUMS', '\n'.join(f"{e['sha256']}  {e['path']}" for e in entries))
write_json(ROOT/'hashes/file-manifest.json', {
    'schema':'pf-ir-04-file-manifest-1.0','created':CREATED_UTC,'access_date':ACCESS_DATE,
    'coverage':'All dossier files except hashes/SHA256SUMS and hashes/file-manifest.json to avoid circularity.','files':entries})

# Zip archive
shutil.make_archive(str(ZIP.with_suffix('')), 'zip', ROOT.parent, ROOT.name)
print(json.dumps({
    'root': str(ROOT),
    'zip': str(ZIP),
    'file_count': len([p for p in ROOT.rglob('*') if p.is_file()]),
    'zip_size': ZIP.stat().st_size,
    'components': len(components),
    'file_records': len(files),
    'unresolved': len(unresolved),
    'raw_capture_records': len(raw_records),
}, indent=2))
