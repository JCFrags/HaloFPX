#!/usr/bin/env python3
"""Focused self-tests for validate_wiki.py."""

from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile
import unittest

import yaml

sys.path.insert(0, str(Path(__file__).resolve().parent))
from validate_wiki import REQUIRED_FILES, validate_wiki


class ValidateWikiTests(unittest.TestCase):
    def make_fixture(self, manifest_updates: dict | None = None) -> tuple[tempfile.TemporaryDirectory, Path, Path]:
        temporary = tempfile.TemporaryDirectory()
        root = Path(temporary.name)
        wiki_root = root / "wiki"
        section = wiki_root / "01_Category" / "01_Example"
        section.mkdir(parents=True)
        registry = [
            {
                "section_id": "01",
                "title": "Example",
                "category_id": "01",
                "category_title": "Category",
                "target_path": "01_Category/01_Example",
            }
        ]
        registry_path = root / "section_index.json"
        registry_path.write_text(json.dumps(registry), encoding="utf-8")
        for name in REQUIRED_FILES:
            if name.endswith(".md"):
                (section / name).write_text("---\nstatus: draft\n---\n\n# Fixture\n", encoding="utf-8")
        (section / "sources.md").write_text(
            "# Sources\n\n| ID | Source | Claims | Limitations |\n|---|---|---|---|\n"
            "| S01-01 | fixture | validation | none |\n",
            encoding="utf-8",
        )
        (section / "open_questions.md").write_text(
            "# Questions\n\n1. **[OPEN]** Is the invalid fixture rejected?\n",
            encoding="utf-8",
        )
        manifest = {
            "section_id": "01",
            "title": "Example",
            "category": "01_Category",
            "status": "draft",
            "last_verified": "2026-07-17",
            "source_count": 1,
            "open_question_count": 1,
            "required_machine_experiments": [{"id": "EXP-01", "title": "Fixture experiment"}],
            "related_sections": [],
            "applicability": {"repositories": []},
            "deliberate_extension": {"nested": ["accepted"]},
        }
        if manifest_updates:
            manifest.update(manifest_updates)
        (section / "section.yaml").write_text(
            yaml.safe_dump(manifest, sort_keys=False), encoding="utf-8"
        )
        return temporary, wiki_root, registry_path

    def test_valid_core_allows_extension_fields(self) -> None:
        temporary, wiki_root, registry = self.make_fixture()
        self.addCleanup(temporary.cleanup)
        results = validate_wiki(wiki_root, registry)
        self.assertEqual(results[0].metadata_errors, [])

    def test_invalid_enum_type_and_declared_counts_are_rejected(self) -> None:
        temporary, wiki_root, registry = self.make_fixture(
            {
                "status": "open",
                "last_verified": "2026-02-30",
                "source_count": "1",
                "open_question_count": 2,
                "required_machine_experiments": [{"id": "EXP-01"}],
                "related_sections": "02",
            }
        )
        self.addCleanup(temporary.cleanup)
        errors = validate_wiki(wiki_root, registry)[0].metadata_errors
        joined = "\n".join(errors)
        self.assertIn("status must be one of", joined)
        self.assertIn("last_verified must be a real", joined)
        self.assertIn("source_count must be a non-negative integer", joined)
        self.assertIn("open_question_count is 2", joined)
        self.assertIn("required_machine_experiments[0].title", joined)
        self.assertIn("related_sections must be a list", joined)

    def test_registry_identity_and_category_are_enforced(self) -> None:
        temporary, wiki_root, registry = self.make_fixture(
            {"section_id": 1, "title": "Wrong", "category": "02_Wrong"}
        )
        self.addCleanup(temporary.cleanup)
        joined = "\n".join(validate_wiki(wiki_root, registry)[0].metadata_errors)
        self.assertIn("section_id must be string '01'", joined)
        self.assertIn("title must equal", joined)
        self.assertIn("category must equal '01_Category'", joined)


if __name__ == "__main__":
    unittest.main()
