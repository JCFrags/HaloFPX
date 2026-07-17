#!/usr/bin/env python3
"""Create a local fixture lock without changing the suite's source manifest."""
from __future__ import annotations
import argparse, hashlib, json
from pathlib import Path

def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda:f.read(1024*1024), b""):
            h.update(chunk)
    return h.hexdigest()

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--manifest", type=Path, default=Path("fixtures/models/model-manifest.json"))
    ap.add_argument("--model", action="append", default=[], metavar="ID=PATH")
    ap.add_argument("--output", type=Path, required=True)
    ns = ap.parse_args()
    manifest = json.loads(ns.manifest.read_text(encoding="utf-8"))
    by_id = {m["id"]:m for m in manifest["models"]}
    resolved = []
    for assignment in ns.model:
        if "=" not in assignment:
            raise SystemExit(f"invalid --model {assignment!r}; expected ID=PATH")
        model_id, raw_path = assignment.split("=",1)
        if model_id not in by_id:
            raise SystemExit(f"unknown model ID {model_id!r}")
        path = Path(raw_path).resolve()
        digest = sha256(path)
        expected = by_id[model_id].get("sha256")
        if expected and digest != expected:
            raise SystemExit(f"{model_id}: SHA-256 mismatch: expected {expected}, got {digest}")
        resolved.append({
            "id":model_id,
            "path":str(path),
            "sha256":digest,
            "bytes":path.stat().st_size,
            "expected_sha256":expected,
        })
    out = {
        "schema_version":"1.0",
        "source_manifest":str(ns.manifest),
        "models":sorted(resolved, key=lambda x:x["id"]),
    }
    ns.output.parent.mkdir(parents=True, exist_ok=True)
    ns.output.write_text(json.dumps(out, indent=2)+"\n", encoding="utf-8")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
