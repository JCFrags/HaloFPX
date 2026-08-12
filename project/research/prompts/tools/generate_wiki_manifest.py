#!/usr/bin/env python3
"""Generate or check the canonical HaloFPX wiki path/artifact manifest."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys

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


def normalized_text_bytes(path: Path) -> bytes:
    """Return UTF-8 text with canonical LF endings for cross-platform hashes."""
    text = path.read_text(encoding="utf-8").replace("\r\n", "\n").replace("\r", "\n")
    return text.encode("utf-8")


def build_manifest(wiki_root: Path, registry_path: Path) -> dict:
    registry_bytes = normalized_text_bytes(registry_path)
    registry = yaml.safe_load(registry_bytes)
    records = registry.get("sections", [])
    if not isinstance(records, list):
        raise ValueError("registry 'sections' must be a list")

    seen_ids: set[str] = set()
    seen_paths: set[str] = set()
    categories: dict[str, dict] = {}
    input_hasher = hashlib.sha256(registry_bytes)

    for item in records:
        section_id = str(item["section_id"])
        target_path = str(item["target_path"]).replace("\\", "/")
        category_id = str(item["category_id"])
        category_title = str(item["category_title"])
        if section_id in seen_ids:
            raise ValueError(f"duplicate section_id in registry: {section_id}")
        if target_path in seen_paths:
            raise ValueError(f"duplicate target_path in registry: {target_path}")
        seen_ids.add(section_id)
        seen_paths.add(target_path)

        section_dir = wiki_root / Path(target_path)
        section_manifest_path = section_dir / "section.yaml"
        missing_files = [name for name in REQUIRED_FILES if not (section_dir / name).is_file()]
        if not section_dir.is_dir():
            artifact_state = "missing"
        elif missing_files:
            artifact_state = "incomplete"
        else:
            artifact_state = "complete"

        declared_status = None
        last_verified = None
        source_count = None
        open_question_count = None
        metadata_errors: list[str] = []
        if section_manifest_path.is_file():
            section_bytes = normalized_text_bytes(section_manifest_path)
            input_hasher.update(target_path.encode("utf-8"))
            input_hasher.update(section_bytes)
            section_data = yaml.safe_load(section_bytes) or {}
            if str(section_data.get("section_id")) != section_id:
                metadata_errors.append(f"section_id must equal {section_id}")
            if str(section_data.get("category_id")) != category_id:
                metadata_errors.append(f"category_id must equal {category_id}")
            if str(section_data.get("title")) != str(item["title"]):
                metadata_errors.append("title must equal the supplied section registry title")
            declared_status = section_data.get("status")
            last_verified = str(section_data.get("last_verified")) if section_data.get("last_verified") else None
            source_count = section_data.get("source_count")
            open_question_count = section_data.get("open_question_count")
        else:
            metadata_errors.append("section.yaml is missing")

        category_path = target_path.split("/", 1)[0]
        category = categories.setdefault(
            category_id,
            {
                "category_id": category_id,
                "title": category_title,
                "canonical_path": category_path,
                "sections": [],
            },
        )
        if category["canonical_path"] != category_path or category["title"] != category_title:
            raise ValueError(f"inconsistent category metadata for {category_id}")
        category["sections"].append(
            {
                "section_id": section_id,
                "title": str(item["title"]),
                "canonical_path": target_path,
                "section_manifest": f"{target_path}/section.yaml",
                "artifact_state": artifact_state,
                "missing_required_files": missing_files,
                "metadata_state": "valid" if not metadata_errors else "invalid",
                "metadata_errors": metadata_errors,
                "declared_status": declared_status,
                "last_verified": last_verified,
                "source_count": source_count,
                "open_question_count": open_question_count,
            }
        )

    category_list = [categories[key] for key in sorted(categories)]
    for category in category_list:
        states = [section["artifact_state"] for section in category["sections"]]
        category["artifact_state"] = "complete" if states and all(s == "complete" for s in states) else "incomplete"
        category["section_count"] = len(category["sections"])

    return {
        "manifest_version": "1.0",
        "wiki_name": "HaloFPX LLM Wiki",
        "authority_scope": "canonical section paths and structural artifact state",
        "generated_on": str(registry.get("generated_on", "unknown")),
        "registry": "../../research/prompts/section_index.yaml",
        "schema": "manifest.schema.json",
        "generator": "../../research/prompts/tools/generate_wiki_manifest.py",
        "input_digest_sha256": input_hasher.hexdigest(),
        "planned_category_count": len(category_list),
        "planned_section_count": len(records),
        "categories": category_list,
    }


def render_manifest(manifest: dict) -> str:
    return yaml.safe_dump(manifest, sort_keys=False, allow_unicode=False, width=120)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("wiki_root", type=Path)
    parser.add_argument(
        "--registry",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "section_index.yaml",
    )
    parser.add_argument("--check", action="store_true", help="fail if manifest.yaml differs from generated output")
    args = parser.parse_args()

    try:
        manifest = build_manifest(args.wiki_root.resolve(), args.registry.resolve())
        rendered = render_manifest(manifest)
        destination = args.wiki_root / "manifest.yaml"
        if args.check:
            existing = destination.read_text(encoding="utf-8")
            if existing != rendered:
                print(f"STALE: {destination}", file=sys.stderr)
                return 1
            print(f"OK: {destination} matches registry and section manifests")
            return 0
        destination.write_text(rendered, encoding="utf-8", newline="\n")
        print(f"WROTE: {destination}")
        return 0
    except (OSError, KeyError, TypeError, ValueError, yaml.YAMLError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
