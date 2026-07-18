#!/usr/bin/env python3
"""Validate the deliberately small JSON Schema subset used by PF-IR-10.

SPDX-License-Identifier: CC0-1.0
"""
from __future__ import annotations
import argparse, json
from pathlib import Path
from typing import Any


def validate(value: Any, schema: dict[str, Any], path: str = "$") -> list[str]:
    errors: list[str] = []
    typ = schema.get("type")
    types = {"object": dict, "array": list, "string": str, "integer": int, "number": (int,float), "boolean": bool, "null": type(None)}
    if typ and (typ not in types or not isinstance(value, types[typ]) or (typ in {"integer","number"} and isinstance(value, bool))):
        return [f"{path}: expected {typ}"]
    if "const" in schema and value != schema["const"]: errors.append(f"{path}: const mismatch")
    if isinstance(value, str):
        if len(value) < schema.get("minLength", 0): errors.append(f"{path}: shorter than minLength")
        if "maxLength" in schema and len(value) > schema["maxLength"]: errors.append(f"{path}: longer than maxLength")
    if isinstance(value, dict):
        required = schema.get("required", [])
        for key in required:
            if key not in value: errors.append(f"{path}: missing {key}")
        props = schema.get("properties", {})
        for key, item in value.items():
            if key in props: errors.extend(validate(item, props[key], f"{path}/{key}"))
            elif schema.get("additionalProperties") is False: errors.append(f"{path}: unexpected {key}")
    return errors


def main() -> int:
    ap = argparse.ArgumentParser(); ap.add_argument("cases", type=Path); ap.add_argument("--schema-dir", type=Path, required=True)
    ns = ap.parse_args(); failures=[]; count=0
    for n,line in enumerate(ns.cases.read_text(encoding="utf-8").splitlines(),1):
        if not line: continue
        row=json.loads(line); count+=1; schema=json.loads((ns.schema_dir/row["schema"]).read_text(encoding="utf-8"))
        try: value=json.loads(row["json_text"]); errors=validate(value,schema)
        except json.JSONDecodeError as exc: errors=[str(exc)]
        actual=not errors
        if actual != row["valid"]: failures.append({"line":n,"id":row["id"],"expected":row["valid"],"actual":actual,"errors":errors})
    print(json.dumps({"match":not failures,"cases":count,"failures":failures},ensure_ascii=False,sort_keys=True,indent=2))
    return 0 if not failures else 1
if __name__ == "__main__": raise SystemExit(main())
