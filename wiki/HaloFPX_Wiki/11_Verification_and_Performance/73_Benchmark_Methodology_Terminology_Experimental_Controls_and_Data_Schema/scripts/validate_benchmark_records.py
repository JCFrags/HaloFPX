#!/usr/bin/env python3
"""Validate HaloFPX benchmark records against the Section 73 schema."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from datetime import datetime
from pathlib import Path
from typing import Any

from jsonschema import Draft202012Validator, FormatChecker


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def json_path(parts: Any) -> str:
    result = "$"
    for part in parts:
        result += f"[{part}]" if isinstance(part, int) else f".{part}"
    return result


def semantic_errors(record: dict[str, Any]) -> list[dict[str, str]]:
    errors: list[dict[str, str]] = []
    record_type = record.get("record_type")

    recorded_at = record.get("recorded_at")
    rfc3339 = re.compile(
        r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(?:\.\d+)?(?:Z|[+-]\d{2}:\d{2})$"
    )
    try:
        if not isinstance(recorded_at, str) or not rfc3339.fullmatch(recorded_at):
            raise ValueError
        datetime.fromisoformat(recorded_at.replace("Z", "+00:00"))
    except ValueError:
        errors.append({
            "kind": "semantic",
            "path": "$.recorded_at",
            "message": "recorded_at must be an RFC 3339 date-time with an explicit UTC offset",
        })

    def require_order(first: str, second: str) -> None:
        left, right = record.get(first), record.get(second)
        if isinstance(left, int) and isinstance(right, int) and left > right:
            errors.append({
                "kind": "semantic",
                "path": f"$.{second}",
                "message": f"{second} must be greater than or equal to {first}",
            })

    if record_type == "request":
        require_order("request_start_ns", "first_output_observed_ns")
        require_order("request_start_ns", "final_response_observed_ns")
        require_order("first_output_observed_ns", "final_response_observed_ns")
        cache = record.get("cache", {})
        if cache.get("restored_tokens", 0) > cache.get("eligible_tokens", 0):
            errors.append({
                "kind": "semantic",
                "path": "$.cache.restored_tokens",
                "message": "restored_tokens must not exceed eligible_tokens",
            })
        speculation = record.get("speculation", {})
        if speculation.get("accepted_tokens", 0) > speculation.get("proposed_tokens", 0):
            errors.append({
                "kind": "semantic",
                "path": "$.speculation.accepted_tokens",
                "message": "accepted_tokens must not exceed proposed_tokens",
            })
    elif record_type == "collective_event":
        require_order("issue_ns", "complete_ns")
    elif record_type == "cache_event":
        if record.get("restored_tokens", 0) > record.get("eligible_tokens", 0):
            errors.append({
                "kind": "semantic",
                "path": "$.restored_tokens",
                "message": "restored_tokens must not exceed eligible_tokens",
            })
    elif record_type == "run_summary":
        for metric_name, metric in record.get("metrics", {}).items():
            interval = metric.get("confidence_interval")
            if interval and interval["lower"] > interval["upper"]:
                errors.append({
                    "kind": "semantic",
                    "path": f"$.metrics.{metric_name}.confidence_interval.upper",
                    "message": "confidence interval upper must be greater than or equal to lower",
                })
    return errors


def validate(schema_path: Path, input_path: Path) -> dict[str, Any]:
    schema = json.loads(schema_path.read_text(encoding="utf-8"))
    Draft202012Validator.check_schema(schema)
    validator = Draft202012Validator(schema, format_checker=FormatChecker())
    instance = json.loads(input_path.read_text(encoding="utf-8"))
    records = instance if isinstance(instance, list) else [instance]
    errors: list[dict[str, Any]] = []

    for index, record in enumerate(records):
        record_type = record.get("record_type") if isinstance(record, dict) else None
        if record_type in schema["$defs"]:
            record_schema = {
                "$schema": schema["$schema"],
                "$ref": f"#/$defs/{record_type}",
                "$defs": schema["$defs"],
            }
            record_validator = Draft202012Validator(
                record_schema, format_checker=FormatChecker()
            )
        else:
            record_validator = validator
        for error in record_validator.iter_errors(record):
            errors.append({
                "record_index": index,
                "kind": "schema",
                "path": json_path(error.absolute_path),
                "message": error.message,
            })
        if isinstance(record, dict):
            for error in semantic_errors(record):
                errors.append({"record_index": index, **error})

    errors.sort(key=lambda item: (
        item["record_index"], item["path"], item["kind"], item["message"]
    ))
    return {
        "schema": str(schema_path.as_posix()),
        "schema_sha256": sha256_file(schema_path),
        "input": str(input_path.as_posix()),
        "input_sha256": sha256_file(input_path),
        "record_count": len(records),
        "valid": not errors,
        "error_count": len(errors),
        "errors": errors,
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Validate a JSON record or array of records against the HaloFPX Section 73 schema."
    )
    parser.add_argument("schema", type=Path, help="Path to benchmark_record.schema.json")
    parser.add_argument("input", type=Path, help="Path to a JSON record or JSON array")
    parser.add_argument(
        "--expect",
        choices=("valid", "invalid"),
        help="Exit successfully only when actual validity matches this expectation.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        result = validate(args.schema.resolve(), args.input.resolve())
    except (OSError, json.JSONDecodeError, ValueError) as error:
        print(json.dumps({"validator_error": str(error)}, sort_keys=True), file=sys.stderr)
        return 2

    expected_valid = None if args.expect is None else args.expect == "valid"
    result["expected"] = args.expect
    result["expectation_met"] = expected_valid is None or result["valid"] == expected_valid
    print(json.dumps(result, indent=2, sort_keys=True))
    if not result["expectation_met"]:
        return 1
    if args.expect is None and not result["valid"]:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
