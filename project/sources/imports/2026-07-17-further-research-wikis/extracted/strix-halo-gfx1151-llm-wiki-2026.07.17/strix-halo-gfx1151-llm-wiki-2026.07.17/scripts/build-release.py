#!/usr/bin/env python3
from __future__ import annotations

import gzip
import hashlib
import json
import os
from pathlib import Path
import shutil
import stat
import tarfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
PARENT = ROOT.parent
VERSION = (ROOT / "VERSION").read_text().strip()
FIXED_EPOCH = 1784289600  # 2026-07-17T12:00:00Z
FIXED_ZIP_TIME = (2026, 7, 17, 12, 0, 0)

ORDERED_DOCS = [
    "docs/index.md", "docs/decision-tree.md", "docs/evidence-model.md",
    "docs/compatibility-matrix.md", "docs/official-support.md", "docs/community-validation.md",
    "docs/regressions.md", "docs/unsupported-combinations.md", "docs/build-flags.md",
    "docs/environment-variables.md", "docs/diagnostics.md", "docs/containers.md",
    "docs/reproducibility.md", "docs/rocmfpx.md", "docs/usb4-networking.md",
    "docs/recipes/host-official-rocm-714.md", "docs/recipes/rocm714-tarball.md",
    "docs/recipes/llama-hip-rocm721.md", "docs/recipes/community-fedora-rocm724.md",
    "docs/recipes/llama-vulkan-radv.md", "docs/recipes/mesa-2615-source.md",
    "docs/recipes/prebuilt-llama-b10064.md", "docs/recipes/rocmfpx-pinned.md",
    "docs/recipes/usb4-ip-network.md", "docs/recipes/usb4-rdma-experimental.md",
    "docs/glossary.md", "docs/sources.md", "docs/versions/2026.07.17.md",
]


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def iter_files():
    for p in sorted(ROOT.rglob("*")):
        if not p.is_file(): continue
        rel = p.relative_to(ROOT)
        if "__pycache__" in rel.parts or p.suffix == ".pyc": continue
        if rel.as_posix() in {"MANIFEST.sha256", "MANIFEST.json", "FILELIST.txt"}: continue
        yield p


def build_llm_files():
    lines = [
        "# Strix Halo / gfx1151 Build and Software Compatibility Wiki",
        "",
        "> Version 2026.07.17. Evidence-scoped Linux, ROCm, HIP, LLVM, Mesa/Vulkan, CMake, llama.cpp, ROCmFPX and USB4 compatibility research.",
        "",
        "## Primary pages",
        "",
        "- [Overview](docs/index.md): Current conclusions and lane selection.",
        "- [Compatibility matrix](docs/compatibility-matrix.md): Official, community, experimental, unsupported and candidate profiles.",
        "- [Official support](docs/official-support.md): AMD and upstream support scopes.",
        "- [Community validation](docs/community-validation.md): Maintained and first-hand Strix Halo reports.",
        "- [Regressions](docs/regressions.md): Kernel, firmware, ROCm, Mesa and llama.cpp failures.",
        "- [Build flags](docs/build-flags.md): Exact CMake flags for HIP, Vulkan and ROCmFPX.",
        "- [Diagnostics](docs/diagnostics.md): Layered collection and acceptance commands.",
        "- [Reproducibility](docs/reproducibility.md): Pins, checksums, containers and host provenance.",
        "- [Source registry](docs/sources.md): Stable source IDs and direct source links.",
        "",
        "## Machine-readable data",
        "",
        f"- [Compatibility JSON](data/compatibility-matrix-{VERSION}.json)",
        f"- [Compatibility CSV](data/compatibility-matrix-{VERSION}.csv)",
        f"- [Regressions JSON](data/regressions-{VERSION}.json)",
        "- [Environment variables JSON](data/environment-variables.json)",
        "- [Source registry JSON](sources/source-registry.json)",
        "",
        "## Reproducible recipes",
        "",
        "- [ROCm 7.14 host](docs/recipes/host-official-rocm-714.md)",
        "- [llama.cpp HIP / ROCm 7.2.1](docs/recipes/llama-hip-rocm721.md)",
        "- [Fedora / ROCm 7.2.4](docs/recipes/community-fedora-rocm724.md)",
        "- [llama.cpp RADV](docs/recipes/llama-vulkan-radv.md)",
        "- [ROCmFPX](docs/recipes/rocmfpx-pinned.md)",
        "- [USB4 IP](docs/recipes/usb4-ip-network.md)",
        "- [USB4 RDMA research](docs/recipes/usb4-rdma-experimental.md)",
        "",
        "The package was statically validated but not executed on physical Strix Halo hardware.",
    ]
    (ROOT / "llms.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")
    full = ["# Strix Halo / gfx1151 Wiki — full text", "", f"Snapshot: {VERSION}", ""]
    for rel in ORDERED_DOCS:
        p = ROOT / rel
        full.extend(["", "---", "", f"<!-- source-file: {rel} -->", "", p.read_text(encoding="utf-8")])
    (ROOT / "llms-full.txt").write_text("\n".join(full).rstrip() + "\n", encoding="utf-8")


def build_manifests():
    entries = []
    for p in iter_files():
        rel = p.relative_to(ROOT).as_posix()
        entries.append({"path": rel, "sha256": sha256(p), "size": p.stat().st_size, "mode": oct(stat.S_IMODE(p.stat().st_mode))})

    # FILELIST is generated first so its content can be integrity-protected.
    listed_paths = sorted([e["path"] for e in entries] + ["FILELIST.txt", "MANIFEST.json", "MANIFEST.sha256"])
    (ROOT / "FILELIST.txt").write_text("".join(f"{path}\n" for path in listed_paths), encoding="utf-8")
    entries.append({"path": "FILELIST.txt", "sha256": sha256(ROOT / "FILELIST.txt"), "size": (ROOT / "FILELIST.txt").stat().st_size, "mode": oct(stat.S_IMODE((ROOT / "FILELIST.txt").stat().st_mode))})

    # MANIFEST.json intentionally excludes MANIFEST.sha256 to avoid a circular hash.
    manifest_entries = sorted(entries, key=lambda x: x["path"])
    (ROOT / "MANIFEST.json").write_text(json.dumps({"version": VERSION, "files": manifest_entries}, indent=2) + "\n", encoding="utf-8")
    checksum_entries = manifest_entries + [{"path": "MANIFEST.json", "sha256": sha256(ROOT / "MANIFEST.json"), "size": (ROOT / "MANIFEST.json").stat().st_size, "mode": oct(stat.S_IMODE((ROOT / "MANIFEST.json").stat().st_mode))}]
    (ROOT / "MANIFEST.sha256").write_text("".join(f"{e['sha256']}  {e['path']}\n" for e in sorted(checksum_entries, key=lambda x: x['path'])), encoding="utf-8")


def archive_files():
    files = [p for p in sorted(ROOT.rglob("*")) if p.is_file() and "__pycache__" not in p.parts and p.suffix != ".pyc"]
    zip_path = PARENT / f"{ROOT.name}.zip"
    tar_path = PARENT / f"{ROOT.name}.tar.gz"
    zip_path.unlink(missing_ok=True); tar_path.unlink(missing_ok=True)
    with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
        for p in files:
            arc = (Path(ROOT.name) / p.relative_to(ROOT)).as_posix()
            info = zipfile.ZipInfo(arc, FIXED_ZIP_TIME)
            mode = stat.S_IMODE(p.stat().st_mode)
            info.external_attr = (mode & 0xFFFF) << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            info.create_system = 3
            zf.writestr(info, p.read_bytes(), compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)
    with tar_path.open("wb") as raw:
        with gzip.GzipFile(filename="", mode="wb", fileobj=raw, mtime=FIXED_EPOCH, compresslevel=9) as gz:
            with tarfile.open(fileobj=gz, mode="w") as tf:
                for p in files:
                    arc = (Path(ROOT.name) / p.relative_to(ROOT)).as_posix()
                    ti = tf.gettarinfo(str(p), arcname=arc)
                    ti.uid = ti.gid = 0; ti.uname = ti.gname = ""; ti.mtime = FIXED_EPOCH
                    with p.open("rb") as f: tf.addfile(ti, f)
    return zip_path, tar_path


def main():
    for p in ROOT.rglob("__pycache__"):
        if p.is_dir(): shutil.rmtree(p)
    build_llm_files()
    build_manifests()
    z, t = archive_files()
    print(f"Created {z} ({z.stat().st_size} bytes)")
    print(f"Created {t} ({t.stat().st_size} bytes)")

if __name__ == "__main__":
    main()
