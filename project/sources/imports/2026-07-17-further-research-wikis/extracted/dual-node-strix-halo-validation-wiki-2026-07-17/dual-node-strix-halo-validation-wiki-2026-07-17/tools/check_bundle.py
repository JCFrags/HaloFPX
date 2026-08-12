#!/usr/bin/env python3
"""Offline structural and schema checks for the validation wiki bundle.

This tool verifies documentation structure and synthetic fixtures. A clean result is
DESIGN_COMPLETE/S0 tooling evidence only; it is never machine validation.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path
from urllib.parse import unquote

try:
    import yaml
    from jsonschema import Draft202012Validator
except ImportError as exc:  # pragma: no cover
    raise SystemExit("Install tools/requirements.txt before running checks") from exc

REQUIRED = [
    "README.md", "Home.md", "_Sidebar.md", "_Footer.md", "EVIDENCE-STATUS.md",
    "wiki/Program-Charter.md", "wiki/Benchmark-Methodology.md", "wiki/Metric-Definitions.md",
    "wiki/Release-Gates.md", "wiki/Regression-Policy.md", "wiki/Upstream-Watch.md",
    "wiki/Implementation-Roadmap.md", "experiments/README.md", "schemas/README.md",
    "config/release-gates.yaml", "config/regression-thresholds.yaml", "config/upstream-watch.yaml",
    "config/provenance-requirements.yaml", "config/slo-policy.yaml",
    "config/freshness-policy.yaml", "config/sut.example.yaml", "raw-data/README.md",
    "tools/evaluate_gates.py", "tools/aggregate_requests.py", "tools/upstream_watch.py", "tools/check_upstream_freshness.py", "tools/audit_provenance.py",
]
SCHEMA_FIXTURES = {
    "manifest.json": "run-manifest.schema.json",
    "summary.json": "summary.schema.json",
    "release-decision.json": "release-decision.schema.json",
    "upstream-event.json": "upstream-event.schema.json",
    "requests.jsonl": "request-trace.schema.json",
    "tokens.jsonl": "token-event.schema.json",
    "telemetry.jsonl": "telemetry-sample.schema.json",
    "faults.jsonl": "fault-event.schema.json",
    "correctness.jsonl": "correctness-record.schema.json",
}
LINK_RE = re.compile(r"(?<!!)\[[^\]]*\]\(([^)]+)\)")
SRC_RE = re.compile(r"SRC-[0-9]{3}")


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def check_markdown_links(root: Path, errors: list[str], warnings: list[str]) -> None:
    for path in root.rglob("*.md"):
        text = path.read_text(encoding="utf-8")
        for raw in LINK_RE.findall(text):
            raw = raw.strip().split(maxsplit=1)[0].strip("<>")
            if raw.startswith(("http://", "https://", "mailto:", "#")):
                continue
            target_text = unquote(raw.split("#", 1)[0])
            if not target_text:
                continue
            target = (path.parent / target_text).resolve()
            try:
                target.relative_to(root.resolve())
            except ValueError:
                warnings.append(f"External/local link escapes bundle: {path.relative_to(root)} -> {raw}")
                continue
            if not target.exists():
                errors.append(f"Broken Markdown link: {path.relative_to(root)} -> {raw}")


def load_sources(root: Path, errors: list[str]) -> set[str]:
    path = root / "references/sources.yaml"
    try:
        data = yaml.safe_load(path.read_text(encoding="utf-8"))
    except Exception as exc:
        errors.append(f"Cannot parse {path.relative_to(root)}: {exc}")
        return set()
    ids = [s.get("id") for s in data.get("sources", [])]
    if len(ids) != len(set(ids)):
        errors.append("Duplicate source IDs in references/sources.yaml")
    return set(ids)


def check_source_refs(root: Path, source_ids: set[str], errors: list[str]) -> None:
    for path in root.rglob("*.md"):
        for sid in set(SRC_RE.findall(path.read_text(encoding="utf-8"))):
            if sid not in source_ids:
                errors.append(f"Unknown source reference {sid} in {path.relative_to(root)}")


def validate_schemas(root: Path, errors: list[str]) -> dict[str, dict]:
    schemas: dict[str, dict] = {}
    for path in sorted((root / "schemas").glob("*.schema.json")):
        try:
            schema = json.loads(path.read_text(encoding="utf-8"))
            Draft202012Validator.check_schema(schema)
            schemas[path.name] = schema
        except Exception as exc:
            errors.append(f"Invalid JSON Schema {path.relative_to(root)}: {exc}")
    return schemas


def validate_fixture(path: Path, schema: dict, errors: list[str]) -> None:
    validator = Draft202012Validator(schema)
    try:
        if path.suffix == ".jsonl":
            for lineno, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
                if not line.strip():
                    continue
                item = json.loads(line)
                for err in validator.iter_errors(item):
                    errors.append(f"Schema error {path.name}:{lineno} {err.json_path}: {err.message}")
        else:
            item = json.loads(path.read_text(encoding="utf-8"))
            for err in validator.iter_errors(item):
                errors.append(f"Schema error {path.name} {err.json_path}: {err.message}")
    except Exception as exc:
        errors.append(f"Cannot validate {path}: {exc}")


def validate_examples(root: Path, schemas: dict[str, dict], errors: list[str]) -> None:
    ex = root / "examples/synthetic-non-machine"
    for fixture, schema_name in SCHEMA_FIXTURES.items():
        path = ex / fixture
        if not path.exists():
            errors.append(f"Missing synthetic fixture {path.relative_to(root)}")
            continue
        if schema_name not in schemas:
            errors.append(f"Missing schema {schema_name} for {fixture}")
            continue
        validate_fixture(path, schemas[schema_name], errors)

    manifest_path = ex / "manifest.json"
    if manifest_path.exists():
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        if manifest.get("evidence_origin") != "synthetic" or manifest.get("evidence_level") != "S0":
            errors.append("Synthetic manifest must be evidence_origin=synthetic and evidence_level=S0")
        for entry in manifest.get("raw_files", []):
            target = ex / entry["path"]
            if not target.exists():
                errors.append(f"Synthetic manifest references missing file: {entry['path']}")
                continue
            if target.stat().st_size != entry["bytes"] or sha256_file(target) != entry["sha256"]:
                errors.append(f"Synthetic manifest hash/size mismatch: {entry['path']}")

    summary_path = ex / "summary.json"
    if summary_path.exists():
        summary = json.loads(summary_path.read_text(encoding="utf-8"))
        if summary.get("evidence_origin") == "machine":
            errors.append("Synthetic summary falsely claims machine origin")


def check_yaml(root: Path, errors: list[str]) -> None:
    for path in sorted(root.rglob("*.yaml")) + sorted(root.rglob("*.yml")):
        try:
            yaml.safe_load(path.read_text(encoding="utf-8"))
        except Exception as exc:
            errors.append(f"Invalid YAML {path.relative_to(root)}: {exc}")


def check_experiments(root: Path, errors: list[str]) -> None:
    found: dict[str, Path] = {}
    for path in (root / "experiments").glob("EXP-*.md"):
        match = re.match(r"(EXP-[0-9]{3})-", path.name)
        if match:
            found[match.group(1)] = path
    expected = {f"EXP-{i:03d}" for i in range(1, 21)}
    for exp in sorted(expected - set(found)):
        errors.append(f"Missing experiment card {exp}")
    for exp in sorted(set(found) - expected):
        errors.append(f"Unexpected experiment ID {exp}")
    for exp, path in found.items():
        text = path.read_text(encoding="utf-8")
        if "Machine-validation status:** Not run" not in text:
            errors.append(f"Experiment lacks nonvalidation banner: {path.relative_to(root)}")
        for section in ["Decision question", "Hypotheses", "Procedure", "Acceptance and regression rules", "Interpretation limits"]:
            if f"## {section}" not in text:
                errors.append(f"Experiment {exp} missing section: {section}")

    gates = yaml.safe_load((root / "config/release-gates.yaml").read_text(encoding="utf-8"))
    for stage, spec in gates.get("stages", {}).items():
        for exp in spec.get("required_experiments", []):
            if exp not in found:
                errors.append(f"Gate {stage} references missing experiment {exp}")


def check_cross_config(root: Path, errors: list[str]) -> None:
    """Verify that duplicated policy identifiers remain exact across config files."""
    try:
        faults = yaml.safe_load((root / "config/fault-matrix.yaml").read_text(encoding="utf-8"))
        gates = yaml.safe_load((root / "config/release-gates.yaml").read_text(encoding="utf-8"))
        regressions = yaml.safe_load((root / "config/regression-thresholds.yaml").read_text(encoding="utf-8"))
        evidence = yaml.safe_load((root / "config/evidence-levels.yaml").read_text(encoding="utf-8"))
    except Exception as exc:
        errors.append(f"Cannot perform cross-config checks: {exc}")
        return

    mandatory_faults = [item.get("id") for item in faults.get("mandatory", [])]
    if len(mandatory_faults) != len(set(mandatory_faults)):
        errors.append("Duplicate mandatory fault IDs in config/fault-matrix.yaml")
    gate_faults = gates.get("global_stable", {}).get("mandatory_fault_scenario_ids", [])
    if mandatory_faults != gate_faults:
        errors.append("Stable gate mandatory fault IDs do not exactly match config/fault-matrix.yaml order/content")

    expected_experiments = [f"EXP-{i:03d}" for i in range(1, 21)]
    g4_experiments = gates.get("stages", {}).get("G4", {}).get("required_experiments", [])
    if g4_experiments != expected_experiments:
        errors.append("G4 must require the exact EXP-001 through EXP-020 matrix")

    metric_ids = [item.get("metric") for item in regressions.get("thresholds", [])]
    if len(metric_ids) != len(set(metric_ids)):
        errors.append("Duplicate metric rules in config/regression-thresholds.yaml")

    levels = [item.get("id") for item in evidence.get("levels", [])]
    level_set = set(levels)
    for stage, spec in gates.get("stages", {}).items():
        if spec.get("minimum_evidence_level") not in level_set:
            errors.append(f"Gate {stage} references unknown evidence level {spec.get('minimum_evidence_level')}")
    if evidence.get("policy", {}).get("minimum_stable_level") != gates.get("stages", {}).get("G4", {}).get("minimum_evidence_level"):
        errors.append("Evidence policy minimum_stable_level does not match G4 minimum evidence level")


def check_upstream_config(root: Path, errors: list[str], warnings: list[str]) -> None:
    policy = yaml.safe_load((root / "config/freshness-policy.yaml").read_text(encoding="utf-8"))
    classes = {x["class"] for x in policy.get("budgets", [])}
    watch = yaml.safe_load((root / "config/upstream-watch.yaml").read_text(encoding="utf-8"))
    ids: list[str] = []
    for src in watch.get("sources", []):
        ids.append(src.get("id"))
        if src.get("freshness_class") not in classes:
            errors.append(f"Upstream source {src.get('id')} has unknown freshness class")
        if src.get("type") == "manual_or_web_hash" and not src.get("url"):
            warnings.append(f"Upstream source {src.get('id')} requires a deployment-time URL")
    if len(ids) != len(set(ids)):
        errors.append("Duplicate upstream watch source IDs")


def render_report(root: Path, errors: list[str], warnings: list[str]) -> str:
    status = "DESIGN_CHECK_PASS" if not errors else "DESIGN_CHECK_FAIL"
    lines = [
        "# Bundle Check Report", "", f"- **Status:** `{status}`",
        "- **Scope:** Offline structure, schema, links, configuration, and synthetic fixtures only.",
        "- **Machine validation:** Not performed.", "",
        f"## Errors ({len(errors)})", "",
    ]
    lines.extend([f"- {x}" for x in errors] or ["- None."])
    lines.extend(["", f"## Warnings ({len(warnings)})", ""])
    lines.extend([f"- {x}" for x in warnings] or ["- None."])
    lines.extend(["", "A clean report authorizes `DESIGN_COMPLETE`; it does not authorize M1, M2, R1, RC, or stable status.", ""])
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--report", type=Path, help="Write Markdown report")
    args = parser.parse_args()
    root = args.root.resolve()
    errors: list[str] = []
    warnings: list[str] = []

    for rel in REQUIRED:
        if not (root / rel).exists():
            errors.append(f"Missing required path: {rel}")
    check_yaml(root, errors)
    schemas = validate_schemas(root, errors)
    validate_examples(root, schemas, errors)
    check_markdown_links(root, errors, warnings)
    source_ids = load_sources(root, errors)
    check_source_refs(root, source_ids, errors)
    check_experiments(root, errors)
    check_cross_config(root, errors)
    check_upstream_config(root, errors, warnings)

    report = render_report(root, errors, warnings)
    print(report)
    if args.report:
        report_path = args.report if args.report.is_absolute() else root / args.report
        report_path.write_text(report, encoding="utf-8")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
