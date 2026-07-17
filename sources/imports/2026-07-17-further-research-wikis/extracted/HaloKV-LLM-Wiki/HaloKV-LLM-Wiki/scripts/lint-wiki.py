#!/usr/bin/env python3
"""Structural lint for the HaloKV LLM Wiki package."""

from __future__ import annotations

import csv
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
REQUIRED_FRONTMATTER = {"title", "tags", "created", "updated", "status", "sources", "related"}
WIKILINK_RE = re.compile(r"\[\[([^\]|#]+)(?:#[^\]|]+)?(?:\|[^\]]+)?\]\]")


def fail(errors: list[str], message: str) -> None:
    errors.append(message)


def parse_frontmatter(path: Path, errors: list[str]) -> dict[str, str]:
    text = path.read_text(encoding="utf-8")
    if not text.startswith("---\n"):
        fail(errors, f"{path.relative_to(ROOT)}: missing YAML frontmatter")
        return {}
    end = text.find("\n---\n", 4)
    if end < 0:
        fail(errors, f"{path.relative_to(ROOT)}: unterminated YAML frontmatter")
        return {}
    result: dict[str, str] = {}
    for line in text[4:end].splitlines():
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        if ":" not in line:
            fail(errors, f"{path.relative_to(ROOT)}: malformed frontmatter line {line!r}")
            continue
        key, value = line.split(":", 1)
        result[key.strip()] = value.strip()
    missing = REQUIRED_FRONTMATTER - result.keys()
    if missing:
        fail(errors, f"{path.relative_to(ROOT)}: missing frontmatter keys {sorted(missing)}")
    return result


def lint_frontmatter(errors: list[str]) -> None:
    for path in [ROOT / "Home.md", *sorted((ROOT / "wiki").glob("*.md"))]:
        parse_frontmatter(path, errors)


def lint_wikilinks(errors: list[str]) -> None:
    markdown = list(ROOT.rglob("*.md"))
    targets = {p.stem for p in markdown}
    targets.update({str(p.relative_to(ROOT).with_suffix("")) for p in markdown})
    for path in markdown:
        text = path.read_text(encoding="utf-8")
        for raw in WIKILINK_RE.findall(text):
            target = raw.strip()
            target_stem = Path(target).stem
            if target not in targets and target_stem not in targets:
                fail(errors, f"{path.relative_to(ROOT)}: dangling wikilink [[{raw}]]")


def load_json(path: Path, errors: list[str]) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:
        fail(errors, f"{path.relative_to(ROOT)}: invalid JSON: {exc}")
        return None


def lint_json(errors: list[str]) -> None:
    objects = {path: load_json(path, errors) for path in sorted(ROOT.rglob("*.json"))}
    try:
        import jsonschema  # type: ignore
    except ImportError:
        print("note: jsonschema not installed; parsed JSON but skipped schema-instance validation")
        return

    pairs = [
        ("protocol/json-schema/checkpoint-manifest.schema.json", "protocol/examples/normal-global-manifest.json"),
        ("protocol/json-schema/halokv-envelope.schema.json", "protocol/examples/hello.json"),
        ("protocol/json-schema/reconnect-plan.schema.json", "protocol/examples/reconnect-delta.json"),
        ("protocol/json-schema/reconnect-plan.schema.json", "protocol/examples/reconfigure-single-node-conditional.json"),
    ]
    for schema_rel, instance_rel in pairs:
        schema = objects.get(ROOT / schema_rel)
        instance = objects.get(ROOT / instance_rel)
        if schema is None or instance is None:
            continue
        try:
            jsonschema.Draft202012Validator.check_schema(schema)
            jsonschema.validate(instance=instance, schema=schema)
        except Exception as exc:
            fail(errors, f"{instance_rel}: does not validate against {schema_rel}: {exc}")


def lint_csv(errors: list[str]) -> None:
    required_headers = {
        "fault-matrix.csv": {"Fault", "Required protocol response", "Single-node continuation"},
        "recovery-options.csv": {"Mechanism", "Network transfer", "Single-node continuation"},
        "degraded-modes.csv": {"Mode", "Recovery", "Single-node continuation"},
        "rpc-validation-matrix.csv": {"Field or object", "Rule", "Failure"},
        "threat-matrix.csv": {"STRIDE class", "Threat", "Controls", "Residual risk"},
    }
    identity_column = {
        "fault-matrix.csv": "Fault",
        "recovery-options.csv": "Mechanism",
        "degraded-modes.csv": "Mode",
        "rpc-validation-matrix.csv": "Field or object",
        "threat-matrix.csv": "Threat",
    }
    for path in sorted((ROOT / "tables").glob("*.csv")):
        with path.open(newline="", encoding="utf-8") as f:
            rows = list(csv.reader(f))
        if len(rows) < 2:
            fail(errors, f"{path.relative_to(ROOT)}: expected header and data rows")
            continue
        width = len(rows[0])
        if width == 0 or any(len(row) != width for row in rows[1:]):
            fail(errors, f"{path.relative_to(ROOT)}: inconsistent CSV row width")
        if len(set(rows[0])) != width:
            fail(errors, f"{path.relative_to(ROOT)}: duplicate header names")
        missing = required_headers.get(path.name, set()) - set(rows[0])
        if missing:
            fail(errors, f"{path.relative_to(ROOT)}: missing required columns {sorted(missing)}")
            continue
        with path.open(newline="", encoding="utf-8") as f:
            dict_rows = list(csv.DictReader(f))
        key = identity_column.get(path.name)
        if key:
            values = [row.get(key, "").strip() for row in dict_rows]
            if any(not value for value in values) or len(values) != len(set(values)):
                fail(errors, f"{path.relative_to(ROOT)}: {key} values must be nonempty and unique")
        if path.name in {"fault-matrix.csv", "recovery-options.csv", "degraded-modes.csv"}:
            for row_number, row in enumerate(dict_rows, start=2):
                verdict = row.get("Single-node continuation", "").strip().lower()
                if not verdict.startswith(("yes", "no", "conditional", "n/a")):
                    fail(errors, f"{path.relative_to(ROOT)}:{row_number}: explicit single-node verdict required")


def lint_proto(errors: list[str]) -> None:
    proto = ROOT / "protocol/halokv.proto"
    text = proto.read_text(encoding="utf-8")
    for left, right, name in [("{", "}", "braces"), ("(", ")", "parentheses")]:
        if text.count(left) != text.count(right):
            fail(errors, f"{proto.relative_to(ROOT)}: unbalanced {name}")
    protoc = shutil.which("protoc")
    if protoc:
        out = ROOT / ".lint-proto"
        out.mkdir(exist_ok=True)
        proc = subprocess.run(
            [protoc, f"--proto_path={proto.parent}", f"--descriptor_set_out={out / 'halokv.pb'}", str(proto)],
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        if proc.returncode != 0:
            fail(errors, f"{proto.relative_to(ROOT)}: protoc failed: {proc.stderr.strip()}")
        shutil.rmtree(out, ignore_errors=True)
    else:
        print("note: protoc not installed; performed delimiter checks only")


def lint_tla(errors: list[str]) -> None:
    path = ROOT / "formal/tla/HaloKV.tla"
    text = path.read_text(encoding="utf-8")
    if "MODULE HaloKV" not in text or "TypeOK" not in text or "AcceptedReadIsValid" not in text:
        fail(errors, f"{path.relative_to(ROOT)}: missing expected model declarations")
    if text.count("<<") != text.count(">>"):
        fail(errors, f"{path.relative_to(ROOT)}: unbalanced tuple delimiters")


def lint_required_files(errors: list[str]) -> None:
    required = [
        "README.md", "Home.md", "_Sidebar.md", "_Footer.md", "AGENTS.md", "log.md",
        "VALIDATION.md", "wiki/index.md", "wiki/Executive-Summary.md", "wiki/Formal-Modeling.md",
        "wiki/Fuzzing-and-Fault-Injection.md", "wiki/Degraded-Mode-Behavior.md",
        "wiki/Fault-and-Recovery-Tables.md", "wiki/Validation-Evidence.md", "wiki/Requirements-Coverage.md",
        "protocol/halokv.proto", "formal/tla/HaloKV.tla", "formal/tla/HaloKV.cfg",
        "tables/fault-matrix.csv", "tables/recovery-options.csv", "tables/degraded-modes.csv",
        "tables/rpc-validation-matrix.csv", "tables/threat-matrix.csv",
        "raw/processed/source-catalog.md", "scripts/deep-validate.py",
    ]
    for rel in required:
        if not (ROOT / rel).is_file():
            fail(errors, f"missing required file: {rel}")


def main() -> int:
    errors: list[str] = []
    lint_required_files(errors)
    lint_frontmatter(errors)
    lint_wikilinks(errors)
    lint_json(errors)
    lint_csv(errors)
    lint_proto(errors)
    lint_tla(errors)

    if errors:
        print(f"FAIL: {len(errors)} issue(s)")
        for item in errors:
            print(f" - {item}")
        return 1

    md_count = len(list(ROOT.rglob("*.md")))
    json_count = len(list(ROOT.rglob("*.json")))
    csv_count = len(list(ROOT.rglob("*.csv")))
    print(f"OK: {md_count} Markdown, {json_count} JSON, {csv_count} CSV files validated")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
