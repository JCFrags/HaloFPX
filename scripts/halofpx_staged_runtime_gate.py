#!/usr/bin/env python3
"""Fail-closed validation for the relocatable L77 staged Linux runtime."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import stat
import subprocess
from pathlib import Path, PurePosixPath

try:
    import pwd
except ImportError:  # Windows-only focused unit-test import.
    pwd = None


SCHEMA = "halofpx.l96.staged-runtime.v1"
EXACT_ROOT = Path("/var/tmp/halofpx-l48-source-nimo2")
EXACT_ARCHIVE = Path("/var/tmp/halofpx-l48-source-nimo2.tar")
APPROVED_EXTERNAL_ROOTS = (
    Path("/usr/lib"), Path("/usr/lib64"), Path("/lib"), Path("/lib64"),
    Path("/opt/rocm"),
)
ELF_NAMES = ("worker", "canary", "placement")


class GateError(RuntimeError):
    pass


def owner_name(uid: int) -> str:
    if pwd is None:
        raise GateError("POSIX owner authority is unavailable")
    return pwd.getpwuid(uid).pw_name


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def validate_runpath(dynamic: str) -> list[str]:
    if re.search(r"\(RPATH\)", dynamic):
        raise GateError("ELF DT_RPATH is forbidden")
    entries = re.findall(r"\(RUNPATH\).*?\[(.*?)\]", dynamic)
    if len(entries) != 1:
        raise GateError("ELF must contain exactly one DT_RUNPATH entry")
    paths = entries[0].split(":")
    if paths != ["$ORIGIN", "/opt/rocm/lib"]:
        raise GateError("ELF RUNPATH is not the exact closed relocatable value")
    for path in paths:
        if path == "$ORIGIN":
            continue
        pure = PurePosixPath(path)
        if not pure.is_absolute() or ".." in pure.parts:
            raise GateError("ELF RUNPATH is relative, traversing, or malformed")
        if pure != PurePosixPath("/opt/rocm/lib"):
            raise GateError("ELF RUNPATH escapes approved runtime authority")
    return paths


def parse_needed(dynamic: str) -> list[str]:
    needed = re.findall(r"\(NEEDED\).*?\[(.*?)\]", dynamic)
    if not needed or any(
            not name or "/" in name or name in {".", ".."} for name in needed):
        raise GateError("ELF DT_NEEDED set is absent or malformed")
    if len(needed) != len(set(needed)):
        raise GateError("ELF DT_NEEDED contains a duplicate")
    return needed


def parse_provenance(
        stdout: str, stderr: str, source_root: str,
        build_id: str, binary: str) -> dict[str, str]:
    if stderr or stdout.count("\n") != 1 or not stdout.endswith("\n"):
        raise GateError("canary provenance output is not one canonical line")
    fields = stdout[:-1].split("|")
    if len(fields) != 4 or any("=" not in field for field in fields):
        raise GateError("canary provenance record is malformed")
    parsed = dict(field.split("=", 1) for field in fields)
    expected = {
        "schema": "halofpx.l57.binary-provenance.v1",
        "source_root": source_root,
        "build_id": build_id,
        "binary": binary,
    }
    if parsed != expected or list(parsed) != list(expected):
        raise GateError("canary provenance does not match manifest authority")
    return parsed


def parse_ldd(text: str, needed: list[str]) -> dict[str, Path]:
    resolved: dict[str, Path] = {}
    for line in text.splitlines():
        match = re.fullmatch(
            r"\s*(\S+)\s+=>\s+(\S+)\s+\(0x[0-9a-fA-F]+\)\s*", line)
        if match:
            name, path = match.groups()
            if path == "not":
                raise GateError(f"unresolved dependency: {name}")
            resolved[name] = Path(path)
    if set(needed) - set(resolved):
        raise GateError(
            f"ldd omitted dependencies: {sorted(set(needed) - set(resolved))}")
    return {name: resolved[name] for name in needed}


def under(path: Path, root: Path) -> bool:
    try:
        path.relative_to(root)
        return True
    except ValueError:
        return False


def validate_dependency_path(path: Path, staged_bin: Path) -> Path:
    if not path.is_absolute() or ".." in path.parts:
        raise GateError("dependency path is not absolute and normalized")
    resolved = path.resolve(strict=True)
    if under(resolved, staged_bin):
        return resolved
    if any(under(resolved, root) for root in APPROVED_EXTERNAL_ROOTS):
        return resolved
    raise GateError(f"dependency escapes closed roots: {resolved}")


def symlink_receipt(path: Path, staged_bin: Path) -> dict[str, object]:
    links = []
    current = path
    seen: set[Path] = set()
    while current.is_symlink():
        if current in seen:
            raise GateError("dependency symlink cycle")
        seen.add(current)
        target = os.readlink(current)
        if os.path.isabs(target):
            raise GateError("staged dependency symlink target is absolute")
        next_path = current.parent / target
        normalized = Path(os.path.normpath(str(next_path)))
        if not under(normalized, staged_bin):
            raise GateError("staged dependency symlink escapes package")
        links.append({"path": str(current), "target": target})
        current = next_path
    final = current.resolve(strict=True)
    if not under(final, staged_bin) or not final.is_file():
        raise GateError("staged dependency final target is invalid")
    metadata = final.stat()
    return {
        "links": links,
        "final": str(final),
        "sha256": sha256_file(final),
        "mode": f"{stat.S_IMODE(metadata.st_mode):04o}",
        "owner": owner_name(metadata.st_uid),
        "bytes": metadata.st_size,
    }


def run_checked(argv: list[str]) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        argv, text=True, capture_output=True, env={
            "PATH": "/usr/bin:/bin", "HOME": "/nonexistent", "LC_ALL": "C"})
    if result.returncode != 0:
        raise GateError(
            f"command failed ({result.returncode}): {argv!r}: "
            f"{result.stderr[-512:]}")
    return result


def validate_elf(
        name: str, path: Path, expected_sha256: str,
        staged_bin: Path) -> dict[str, object]:
    metadata = path.stat()
    if (
        not path.is_file() or path.is_symlink()
        or stat.S_IMODE(metadata.st_mode) != 0o755
        or owner_name(metadata.st_uid) != "connorb"
        or sha256_file(path) != expected_sha256
    ):
        raise GateError(f"{name} binary identity mismatch")
    dynamic = run_checked(["/usr/bin/readelf", "-d", str(path)]).stdout
    runpaths = validate_runpath(dynamic)
    needed = parse_needed(dynamic)
    ldd = run_checked(["/usr/bin/ldd", str(path)]).stdout
    dependencies = parse_ldd(ldd, needed)
    dependency_receipts = {}
    for dependency, reported in dependencies.items():
        resolved = validate_dependency_path(reported, staged_bin)
        item: dict[str, object] = {
            "reported": str(reported), "resolved": str(resolved)}
        if under(reported, staged_bin):
            item["staged_identity"] = symlink_receipt(reported, staged_bin)
        dependency_receipts[dependency] = item
    return {
        "path": str(path),
        "sha256": expected_sha256,
        "bytes": metadata.st_size,
        "mode": "0755",
        "owner": "connorb",
        "runpath": runpaths,
        "needed": needed,
        "dependencies": dependency_receipts,
        "readelf_sha256": hashlib.sha256(dynamic.encode()).hexdigest(),
        "ldd_sha256": hashlib.sha256(
            re.sub(r"\(0x[0-9a-fA-F]+\)", "(0xADDR)", ldd).encode()
        ).hexdigest(),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", required=True, type=Path)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--archive", required=True, type=Path)
    parser.add_argument("--archive-sha256", required=True)
    parser.add_argument("--receipt", required=True, type=Path)
    args = parser.parse_args()
    if args.root != EXACT_ROOT or args.root.resolve(strict=True) != EXACT_ROOT:
        raise GateError("staged root is not the exact closed nimo-2 root")
    if re.fullmatch(r"[0-9a-f]{64}", args.archive_sha256) is None:
        raise GateError("archive identity is malformed")
    if (
        args.archive != EXACT_ARCHIVE
        or not args.archive.is_file() or args.archive.is_symlink()
        or sha256_file(args.archive) != args.archive_sha256
    ):
        raise GateError("archive identity does not match exact staged bytes")
    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    hashes = manifest["executable_sha256"]
    provenance = manifest["authority_contract"]["provenance"]
    staged_bin = args.root / "build-l48" / "bin"
    paths = {
        "worker": staged_bin / "rpc-server",
        "canary": staged_bin / "test-halofpx-distributed-state-canary",
        "placement": staged_bin / "test-halofpx-placement-probe",
    }
    binaries = {
        name: validate_elf(name, paths[name], hashes[name], staged_bin)
        for name in ELF_NAMES
    }
    probes = {}
    for label, binary_name, argv in (
        ("canary_provenance", "canary",
         [str(paths["canary"]), "--halofpx-provenance"]),
        ("worker_provenance", "rpc-server",
         [str(paths["worker"]), "--halofpx-provenance"]),
    ):
        result = run_checked(argv)
        probes[label] = {
            "argv": argv,
            "record": parse_provenance(
                result.stdout, result.stderr,
                provenance["source_root"], provenance["build_id"],
                binary_name),
            "stdout_sha256": hashlib.sha256(result.stdout.encode()).hexdigest(),
            "stderr_sha256": hashlib.sha256(result.stderr.encode()).hexdigest(),
        }
    receipt = {
        "schema": SCHEMA,
        "status": "pass",
        "root": str(args.root),
        "archive_sha256": args.archive_sha256,
        "archive_bytes": args.archive.stat().st_size,
        "manifest_sha256": sha256_file(args.manifest),
        "gate_sha256": sha256_file(Path(__file__)),
        "source_root": provenance["source_root"],
        "build_id": provenance["build_id"],
        "binaries": binaries,
        "probes": probes,
    }
    encoded = (
        json.dumps(receipt, indent=2, sort_keys=True) + "\n").encode("utf-8")
    if args.receipt.exists() or args.receipt.is_symlink():
        raise GateError("receipt path already exists")
    pending = args.receipt.with_name(f".{args.receipt.name}.{os.getpid()}.pending")
    with pending.open("xb") as output:
        output.write(encoded)
        output.flush()
        os.fsync(output.fileno())
    os.link(pending, args.receipt)
    pending.unlink()
    directory = os.open(args.receipt.parent, os.O_RDONLY)
    try:
        os.fsync(directory)
    finally:
        os.close(directory)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
