#!/usr/bin/env python3
"""Focused self-tests for generate_wiki_manifest.py."""

from __future__ import annotations

from pathlib import Path
import sys
import tempfile
import unittest

import yaml

sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_wiki_manifest import REQUIRED_FILES, build_manifest


class GenerateWikiManifestTests(unittest.TestCase):
    def make_fixture(self, line_ending: str) -> tuple[tempfile.TemporaryDirectory, Path, Path]:
        temporary = tempfile.TemporaryDirectory()
        root = Path(temporary.name)
        wiki_root = root / "wiki"
        section = wiki_root / "01_Category" / "01_Example"
        section.mkdir(parents=True)

        registry = {
            "generated_on": "2026-08-12",
            "sections": [
                {
                    "section_id": "01",
                    "title": "Example",
                    "category_id": "01",
                    "category_title": "Category",
                    "target_path": "01_Category/01_Example",
                }
            ],
        }
        registry_path = root / "section_index.yaml"
        registry_text = yaml.safe_dump(registry, sort_keys=False).replace("\n", line_ending)
        registry_path.write_bytes(registry_text.encode("utf-8"))

        for name in REQUIRED_FILES:
            (section / name).write_text("# Fixture\n", encoding="utf-8", newline=line_ending)
        section_manifest = {
            "section_id": "01",
            "category_id": "01",
            "title": "Example",
            "status": "draft",
            "last_verified": "2026-08-12",
            "source_count": 0,
            "open_question_count": 0,
        }
        section_text = yaml.safe_dump(section_manifest, sort_keys=False).replace("\n", line_ending)
        (section / "section.yaml").write_bytes(section_text.encode("utf-8"))
        return temporary, wiki_root, registry_path

    def test_input_digest_is_independent_of_line_endings(self) -> None:
        lf_temp, lf_wiki, lf_registry = self.make_fixture("\n")
        crlf_temp, crlf_wiki, crlf_registry = self.make_fixture("\r\n")
        self.addCleanup(lf_temp.cleanup)
        self.addCleanup(crlf_temp.cleanup)

        lf_manifest = build_manifest(lf_wiki, lf_registry)
        crlf_manifest = build_manifest(crlf_wiki, crlf_registry)
        self.assertEqual(lf_manifest, crlf_manifest)


if __name__ == "__main__":
    unittest.main()
