#!/usr/bin/env python3
"""Run HaloKV structural, schema, trace, and optional formal checks.

Optional environment variables:
  TLA2TOOLS_JAR=/path/to/tla2tools.jar  Run SANY and TLC.

Optional Python packages:
  jsonschema, PyYAML, grpcio-tools
"""

from __future__ import annotations

import importlib.util
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def run(name: str, argv: list[str], *, cwd: Path = ROOT) -> None:
    print(f"[RUN ] {name}: {' '.join(argv)}")
    proc = subprocess.run(argv, cwd=cwd, text=True)
    if proc.returncode:
        raise SystemExit(f"[FAIL] {name}: exit {proc.returncode}")
    print(f"[PASS] {name}")


def validate_yaml() -> None:
    try:
        import yaml  # type: ignore
    except ImportError:
        print("[SKIP] YAML structural validation: PyYAML not installed")
        return
    path = ROOT / "fuzz/fault-campaigns.yaml"
    data = yaml.safe_load(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict) or not isinstance(data.get("campaigns"), list):
        raise SystemExit("[FAIL] YAML: expected mapping with campaigns list")
    ids = [c.get("id") for c in data["campaigns"] if isinstance(c, dict)]
    if len(ids) != len(set(ids)) or any(not item for item in ids):
        raise SystemExit("[FAIL] YAML: campaign ids must be nonempty and unique")
    print(f"[PASS] YAML: {len(ids)} campaigns")


def validate_traces() -> None:
    sys.path.insert(0, str(ROOT))
    from fuzz.reference_model import run_trace  # pylint: disable=import-outside-toplevel

    paths = sorted((ROOT / "fuzz/traces").glob("*.json"))
    if not paths:
        raise SystemExit("[FAIL] traces: no trace files")
    for path in paths:
        results = run_trace(path)
        print(f"[PASS] trace {path.name}: {len(results)} events")


def validate_proto() -> None:
    proto = ROOT / "protocol/halokv.proto"
    with tempfile.TemporaryDirectory(prefix="halokv-proto-") as td:
        out = Path(td) / "halokv.pb"
        protoc = shutil.which("protoc")
        if protoc:
            argv = [protoc, f"--proto_path={proto.parent}", f"--descriptor_set_out={out}", str(proto)]
        elif importlib.util.find_spec("grpc_tools.protoc"):
            argv = [
                sys.executable,
                "-m",
                "grpc_tools.protoc",
                f"--proto_path={proto.parent}",
                f"--descriptor_set_out={out}",
                "--include_imports",
                str(proto),
            ]
        else:
            print("[SKIP] protobuf compilation: protoc/grpcio-tools not installed")
            return
        run("protobuf compilation", argv)
        if not out.is_file() or out.stat().st_size == 0:
            raise SystemExit("[FAIL] protobuf compilation produced no descriptor")
        print(f"[PASS] protobuf descriptor: {out.stat().st_size} bytes")


def validate_tla() -> None:
    jar_value = os.environ.get("TLA2TOOLS_JAR")
    if not jar_value:
        print("[SKIP] SANY/TLC: TLA2TOOLS_JAR not set")
        return
    jar = Path(jar_value).expanduser().resolve()
    if not jar.is_file():
        raise SystemExit(f"[FAIL] TLA2TOOLS_JAR not found: {jar}")
    cwd = ROOT / "formal/tla"
    run("SANY", ["java", "-cp", str(jar), "tla2sany.SANY", "HaloKV.tla"], cwd=cwd)
    run(
        "TLC",
        ["java", "-XX:+UseParallelGC", "-Xmx2g", "-cp", str(jar), "tlc2.TLC", "-workers", "8", "-config", "HaloKV.cfg", "HaloKV.tla"],
        cwd=cwd,
    )


def main() -> int:
    run("wiki lint", [sys.executable, "scripts/lint-wiki.py"])
    run("reference-model tests", [sys.executable, "-m", "unittest", "fuzz/test_reference_model.py"])
    validate_traces()
    validate_yaml()
    validate_proto()
    validate_tla()
    print("[PASS] deep validation complete")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
