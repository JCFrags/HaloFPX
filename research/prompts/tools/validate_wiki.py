#!/usr/bin/env python3
"""Validate HaloFPX Wiki structure and the required section.yaml contract."""

from __future__ import annotations

import argparse
from dataclasses import dataclass, field
from datetime import date
import json
from pathlib import Path
import re
import sys
from typing import Any

import yaml


REQUIRED_FILES = (
    "README.md",
    "facts_and_constraints.md",
    "design_implications.md",
    "procedures_and_checks.md",
    "open_questions.md",
    "sources.md",
    "section.yaml",
)

ALLOWED_STATUSES = {
    "draft",
    "verified",
    "needs-machine-validation",
    "superseded",
}

REQUIRED_SECTION_KEYS = (
    "section_id",
    "title",
    "status",
    "last_verified",
    "source_count",
    "open_question_count",
    "required_machine_experiments",
    "related_sections",
    "applicability",
)


@dataclass
class SectionResult:
    item: dict[str, Any]
    section_exists: bool = True
    missing_files: list[str] = field(default_factory=list)
    metadata_errors: list[str] = field(default_factory=list)


def _nonempty_string(value: Any) -> bool:
    return isinstance(value, str) and bool(value.strip())


def _valid_date(value: Any) -> bool:
    if not isinstance(value, str) or not re.fullmatch(r"\d{4}-\d{2}-\d{2}", value):
        return False
    try:
        date.fromisoformat(value)
    except ValueError:
        return False
    return True


def _markdown_tables(text: str) -> list[tuple[list[str], list[list[str]]]]:
    """Return simple pipe tables; enough for the Wiki source/question registers."""
    lines = text.splitlines()
    separator = re.compile(r"^\s*\|(?:\s*:?-{3,}:?\s*\|)+\s*$")
    tables: list[tuple[list[str], list[list[str]]]] = []
    index = 0
    while index + 1 < len(lines):
        if lines[index].lstrip().startswith("|") and separator.match(lines[index + 1]):
            header = [cell.strip().lower() for cell in lines[index].strip().strip("|").split("|")]
            rows: list[list[str]] = []
            index += 2
            while index < len(lines) and lines[index].lstrip().startswith("|"):
                rows.append([cell.strip() for cell in lines[index].strip().strip("|").split("|")])
                index += 1
            tables.append((header, rows))
            continue
        index += 1
    return tables


def count_source_records(text: str, section_id: str) -> int:
    """Count declared source records across the Wiki's table and heading formats."""
    identifiers: set[str] = set()
    for header, rows in _markdown_tables(text):
        is_source_register = (
            header
            and "id" in header[0]
            and any(
                any(word in cell for word in ("source", "authority", "publisher", "origin"))
                for cell in header[1:]
            )
        )
        if is_source_register:
            identifiers.update(row[0].strip("`* ") for row in rows if row and row[0].strip())

    escaped_id = re.escape(section_id)
    declaration_patterns = (
        re.compile(
            rf"^\s*#{{2,6}}\s+`?((?:S{escaped_id}|SRC[-_]?{escaped_id})[-_][A-Za-z0-9_-]+)`?(?:\s|$|—)",
            re.IGNORECASE,
        ),
        re.compile(
            rf"^\s*-\s+\*\*`?((?:S{escaped_id}|SRC[-_]?{escaped_id})[-_][A-Za-z0-9_-]+)`?:?\*\*",
            re.IGNORECASE,
        ),
    )
    for line in text.splitlines():
        for pattern in declaration_patterns:
            match = pattern.match(line)
            if match:
                identifiers.add(match.group(1))
    return len(identifiers)


def count_open_question_records(text: str) -> int:
    """Count the primary question table, or the numbered OPEN register."""
    table_counts = [
        len(rows)
        for header, rows in _markdown_tables(text)
        if any("question" in cell for cell in header)
    ]
    if table_counts:
        # Alias/crosswalk tables may follow the primary register. The primary
        # question register is the largest question-bearing table.
        return max(table_counts)
    numbered_open = re.compile(r"^\s*\d+\.\s+\*{0,2}\[OPEN\]\*{0,2}", re.IGNORECASE)
    return sum(bool(numbered_open.match(line)) for line in text.splitlines())


def _validate_experiments(value: Any, errors: list[str]) -> None:
    if not isinstance(value, list):
        errors.append("required_machine_experiments must be a list")
        return
    seen_ids: set[str] = set()
    for index, experiment in enumerate(value):
        label = f"required_machine_experiments[{index}]"
        if isinstance(experiment, str):
            if not experiment.strip():
                errors.append(f"{label} must not be empty")
            continue
        if not isinstance(experiment, dict):
            errors.append(f"{label} must be a non-empty string or mapping")
            continue
        for key in ("id", "title"):
            if not _nonempty_string(experiment.get(key)):
                errors.append(f"{label}.{key} must be a non-empty string")
        experiment_id = experiment.get("id")
        if _nonempty_string(experiment_id):
            if experiment_id in seen_ids:
                errors.append(f"duplicate required machine experiment id: {experiment_id}")
            seen_ids.add(experiment_id)


def _validate_string_list(name: str, value: Any, errors: list[str]) -> None:
    if not isinstance(value, list):
        errors.append(f"{name} must be a list")
        return
    if any(not _nonempty_string(item) for item in value):
        errors.append(f"{name} entries must be non-empty strings")
    strings = [item for item in value if isinstance(item, str)]
    if len(strings) != len(set(strings)):
        errors.append(f"{name} entries must be unique")


def validate_section_metadata(
    data: Any,
    item: dict[str, Any],
    section_dir: Path,
) -> list[str]:
    """Validate the required core contract while permitting extension keys."""
    errors: list[str] = []
    if not isinstance(data, dict):
        return ["section.yaml must contain a mapping"]

    for key in REQUIRED_SECTION_KEYS:
        if key not in data:
            errors.append(f"missing required key: {key}")

    expected_id = str(item["section_id"])
    expected_title = str(item["title"])
    expected_category_id = str(item["category_id"])
    expected_category_path = str(item["target_path"]).replace("\\", "/").split("/", 1)[0]

    if data.get("section_id") != expected_id:
        errors.append(f"section_id must be string {expected_id!r}")
    if data.get("title") != expected_title:
        errors.append("title must equal the section registry title")
    if data.get("status") not in ALLOWED_STATUSES:
        errors.append(f"status must be one of {sorted(ALLOWED_STATUSES)}")
    if not _valid_date(data.get("last_verified")):
        errors.append("last_verified must be a real YYYY-MM-DD date string")

    category = data.get("category")
    category_id = data.get("category_id")
    if category is None and category_id is None:
        errors.append("one of category or category_id is required")
    if category is not None and category != expected_category_path:
        errors.append(f"category must equal {expected_category_path!r}")
    if category_id is not None and category_id != expected_category_id:
        errors.append(f"category_id must be string {expected_category_id!r}")
    if "category_title" in data and not _nonempty_string(data["category_title"]):
        errors.append("category_title must be a non-empty string when present")

    for key in ("source_count", "open_question_count"):
        value = data.get(key)
        if isinstance(value, bool) or not isinstance(value, int) or value < 0:
            errors.append(f"{key} must be a non-negative integer")

    _validate_experiments(data.get("required_machine_experiments"), errors)
    _validate_string_list("related_sections", data.get("related_sections"), errors)

    applicability = data.get("applicability")
    if not isinstance(applicability, dict) or not applicability:
        errors.append("applicability must be a non-empty mapping")
    elif any(not _nonempty_string(key) for key in applicability):
        errors.append("applicability keys must be non-empty strings")

    sources_path = section_dir / "sources.md"
    questions_path = section_dir / "open_questions.md"
    if sources_path.is_file() and isinstance(data.get("source_count"), int):
        actual = count_source_records(sources_path.read_text(encoding="utf-8"), expected_id)
        if data["source_count"] != actual:
            errors.append(f"source_count is {data['source_count']}, but sources.md declares {actual} records")
    if questions_path.is_file() and isinstance(data.get("open_question_count"), int):
        actual = count_open_question_records(questions_path.read_text(encoding="utf-8"))
        if data["open_question_count"] != actual:
            errors.append(
                f"open_question_count is {data['open_question_count']}, "
                f"but open_questions.md declares {actual} records"
            )

    return errors


def validate_wiki(wiki_root: Path, registry_path: Path) -> list[SectionResult]:
    registry = json.loads(registry_path.read_text(encoding="utf-8"))
    if not isinstance(registry, list):
        raise ValueError("registry must contain a list of section records")

    results: list[SectionResult] = []
    for item in registry:
        section_dir = wiki_root / item["target_path"]
        result = SectionResult(item=item)
        if not section_dir.is_dir():
            result.section_exists = False
            result.missing_files = list(REQUIRED_FILES)
            results.append(result)
            continue
        result.missing_files = [name for name in REQUIRED_FILES if not (section_dir / name).is_file()]
        manifest_path = section_dir / "section.yaml"
        if manifest_path.is_file():
            try:
                data = yaml.safe_load(manifest_path.read_text(encoding="utf-8"))
                result.metadata_errors = validate_section_metadata(data, item, section_dir)
            except (OSError, UnicodeError, yaml.YAMLError) as exc:
                result.metadata_errors = [f"cannot parse section.yaml: {exc}"]
        results.append(result)
    return results


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("wiki_root", type=Path, help="Path to the assembled HaloFPX_Wiki directory")
    parser.add_argument(
        "--registry",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "section_index.json",
        help="Path to section_index.json",
    )
    parser.add_argument("--show-complete", action="store_true")
    args = parser.parse_args()

    try:
        results = validate_wiki(args.wiki_root, args.registry)
    except (OSError, UnicodeError, json.JSONDecodeError, KeyError, TypeError, ValueError) as exc:
        print(f"ERROR: cannot validate wiki: {exc}", file=sys.stderr)
        return 2

    complete = [result for result in results if not result.missing_files]
    incomplete = [result for result in results if result.missing_files]
    schema_valid = [result for result in complete if not result.metadata_errors]
    schema_invalid = [result for result in complete if result.metadata_errors]

    print(f"Wiki root: {args.wiki_root}")
    print(f"Expected sections: {len(results)}")
    print(f"Complete: {len(complete)}")
    print(f"Incomplete: {len(incomplete)}")
    print(f"Missing: {sum(not result.section_exists for result in results)}")
    print(f"Schema-valid: {len(schema_valid)}")
    print(f"Schema-invalid: {len(schema_invalid)}")

    if args.show_complete:
        for result in schema_valid:
            print(f"OK {result.item['section_id']} {result.item['title']}")
    for result in incomplete:
        print(
            f"INCOMPLETE {result.item['section_id']} {result.item['target_path']}: "
            f"missing {', '.join(result.missing_files)}"
        )
    for result in schema_invalid:
        for error in result.metadata_errors:
            print(f"SCHEMA {result.item['section_id']} {result.item['target_path']}: {error}")

    return 1 if incomplete or schema_invalid else 0


if __name__ == "__main__":
    raise SystemExit(main())
