#!/usr/bin/env python3
"""Deterministically validate HaloFPX experiment cards and alias coverage."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import jsonschema
import yaml

EXPECTED_IDS = [f"HLX-EXP-20260717-{number}" for number in range(841, 851)]
EXPECTED_ALIASES = (
    [f"M82-{number:02d}" for number in range(1, 13)]
    + [f"M83-{number:02d}" for number in range(1, 11)]
    + [f"EX85-{number:02d}" for number in range(1, 7)]
    + [f"EXP-86-{number:02d}" for number in range(1, 5)]
)


def parse_args() -> argparse.Namespace:
    here = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("paths", nargs="*", type=Path, help="card YAML files or directories; defaults to cards/")
    parser.add_argument("--schema", type=Path, default=here / "experiment-card.schema.json")
    parser.add_argument("--mapping", type=Path, default=here / "experiment-aliases.yaml")
    parser.add_argument("--format", choices=("text", "json"), default="text", dest="output_format")
    parser.add_argument("--skip-mapping", action="store_true", help="do not validate alias mapping")
    return parser.parse_args()


def load_yaml(path: Path):
    with path.open("r", encoding="utf-8") as handle:
        return yaml.safe_load(handle)


def card_paths(inputs: list[Path], default: Path) -> list[Path]:
    found: set[Path] = set()
    for item in inputs or [default]:
        if item.is_dir():
            found.update(item.glob("*.yaml"))
        else:
            found.add(item)
    return sorted((path.resolve() for path in found), key=lambda path: path.as_posix())


def error_record(path: Path, code: str, message: str, location: list | None = None) -> dict:
    return {"path": str(path), "code": code, "location": location or [], "message": message}


def validate_mapping(path: Path, known_ids: set[str]) -> list[dict]:
    errors: list[dict] = []
    try:
        document = load_yaml(path)
    except Exception as exc:
        return [error_record(path, "mapping-parse", str(exc))]
    if not isinstance(document, dict) or set(document) != {"schema_version", "aliases"}:
        return [error_record(path, "mapping-shape", "mapping must contain only schema_version and aliases")]
    aliases = document.get("aliases")
    if not isinstance(aliases, list):
        return [error_record(path, "mapping-shape", "aliases must be a list")]
    seen: set[str] = set()
    for index, entry in enumerate(aliases):
        location = ["aliases", index]
        if not isinstance(entry, dict) or set(entry) != {"alias", "title", "canonical_cards", "coverage"}:
            errors.append(error_record(path, "mapping-entry", "entry requires alias, title, canonical_cards, coverage only", location))
            continue
        alias = entry["alias"]
        if alias in seen:
            errors.append(error_record(path, "mapping-duplicate", f"duplicate alias {alias}", location))
        seen.add(alias)
        cards = entry["canonical_cards"]
        if not isinstance(cards, list) or not cards:
            errors.append(error_record(path, "mapping-cards", f"{alias} has no canonical card", location))
        else:
            for card_id in cards:
                if card_id not in known_ids:
                    errors.append(error_record(path, "mapping-card-unknown", f"{alias} references unknown {card_id}", location))
        if entry["coverage"] not in ("planned", "conditional"):
            errors.append(error_record(path, "mapping-coverage", f"{alias} coverage must be planned or conditional", location))
    missing = sorted(set(EXPECTED_ALIASES) - seen)
    extra = sorted(seen - set(EXPECTED_ALIASES))
    if missing:
        errors.append(error_record(path, "mapping-missing", f"missing aliases: {', '.join(missing)}"))
    if extra:
        errors.append(error_record(path, "mapping-extra", f"unexpected aliases: {', '.join(extra)}"))
    return errors


def main() -> int:
    args = parse_args()
    here = Path(__file__).resolve().parent
    errors: list[dict] = []
    try:
        schema = json.loads(args.schema.read_text(encoding="utf-8"))
        validator_class = jsonschema.validators.validator_for(schema)
        validator_class.check_schema(schema)
        validator = validator_class(schema, format_checker=jsonschema.FormatChecker())
    except Exception as exc:
        errors.append(error_record(args.schema, "schema", str(exc)))
        validator = None

    paths = card_paths(args.paths, here / "cards")
    loaded_ids: list[str] = []
    for path in paths:
        try:
            card = load_yaml(path)
        except Exception as exc:
            errors.append(error_record(path, "yaml-parse", str(exc)))
            continue
        if validator is not None:
            for problem in sorted(validator.iter_errors(card), key=lambda item: (list(item.absolute_path), item.message)):
                errors.append(error_record(path, "schema-validation", problem.message, list(problem.absolute_path)))
        if isinstance(card, dict) and isinstance(card.get("experiment_id"), str):
            loaded_ids.append(card["experiment_id"])
            expected_name = f"{card['experiment_id']}.yaml"
            if path.name != expected_name:
                errors.append(error_record(path, "filename", f"expected filename {expected_name}"))

    duplicates = sorted({item for item in loaded_ids if loaded_ids.count(item) > 1})
    if duplicates:
        errors.append(error_record(here / "cards", "duplicate-id", f"duplicate IDs: {', '.join(duplicates)}"))
    if not args.paths:
        missing = sorted(set(EXPECTED_IDS) - set(loaded_ids))
        extra = sorted(set(loaded_ids) - set(EXPECTED_IDS))
        if missing:
            errors.append(error_record(here / "cards", "missing-card", f"missing cards: {', '.join(missing)}"))
        if extra:
            errors.append(error_record(here / "cards", "unexpected-card", f"unexpected cards: {', '.join(extra)}"))
    if not args.skip_mapping:
        errors.extend(validate_mapping(args.mapping.resolve(), set(EXPECTED_IDS)))

    report = {"valid": not errors, "card_count": len(paths), "error_count": len(errors), "errors": errors}
    if args.output_format == "json":
        print(json.dumps(report, indent=2, sort_keys=True))
    elif errors:
        for item in errors:
            location = "/".join(map(str, item["location"]))
            suffix = f" [{location}]" if location else ""
            print(f"ERROR {item['code']}: {item['path']}{suffix}: {item['message']}")
        print(f"INVALID: {len(errors)} error(s) across {len(paths)} card(s)")
    else:
        print(f"OK: {len(paths)} card(s); alias mapping complete")
    return 0 if not errors else 1


if __name__ == "__main__":
    sys.exit(main())
