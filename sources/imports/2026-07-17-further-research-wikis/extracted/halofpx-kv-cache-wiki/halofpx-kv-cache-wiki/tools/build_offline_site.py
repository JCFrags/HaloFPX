#!/usr/bin/env python3
"""Build the single-file offline Wiki handbook with stable chapter anchors."""
from __future__ import annotations

import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs"
SITE = ROOT / "site"

CHAPTERS: list[tuple[str, str]] = [
    ("index.md", "wiki-home"),
    ("executive-findings.md", "executive-findings"),
    ("scope-and-method.md", "scope-and-method"),
    ("observed-cachyllama.md", "observed-cachyllama"),
    ("observed-llama-cpp.md", "observed-llama-cpp"),
    ("comparative-designs.md", "comparative-designs"),
    ("halofpx-redesign.md", "halofpx-redesign"),
    ("integrity-invariants.md", "integrity-invariants"),
    ("storage-schemas.md", "storage-schemas"),
    ("state-machines.md", "state-machines"),
    ("failure-and-recovery.md", "failure-and-recovery"),
    ("security.md", "security"),
    ("ssd-endurance.md", "ssd-endurance"),
    ("validation.md", "validation"),
    ("source-matrix.md", "source-matrix"),
    ("claim-ledger.md", "claim-ledger"),
    ("glossary.md", "glossary"),
    ("changelog.md", "changelog"),
]
ANCHOR_BY_FILE = dict(CHAPTERS)
LOCAL_MD_LINK = re.compile(r"(?P<prefix>\[[^\]]*\]\()(?P<target>[^)\s]+\.md(?:#[^)\s]+)?)(?P<suffix>[^)]*\))")


def rewrite_local_links(text: str) -> str:
    def replace(match: re.Match[str]) -> str:
        target = match.group("target")
        if "://" in target:
            return match.group(0)
        path, marker, fragment = target.partition("#")
        name = Path(path).name
        if name not in ANCHOR_BY_FILE:
            return match.group(0)
        anchor = fragment if marker else ANCHOR_BY_FILE[name]
        return f'{match.group("prefix")}#{anchor}{match.group("suffix")}'

    return LOCAL_MD_LINK.sub(replace, text)


def add_chapter_anchor(text: str, anchor: str, source: Path) -> str:
    lines = text.splitlines()
    for index, line in enumerate(lines):
        if line.startswith("# "):
            if "{#" not in line:
                lines[index] = f"{line} {{#{anchor}}}"
            return "\n".join(lines) + "\n"
    raise ValueError(f"No level-1 heading in {source}")


def main() -> int:
    SITE.mkdir(parents=True, exist_ok=True)
    parts: list[str] = []
    for filename, anchor in CHAPTERS:
        source = DOCS / filename
        chapter = add_chapter_anchor(source.read_text(encoding="utf-8"), anchor, source)
        parts.append(rewrite_local_links(chapter))
    build_input = DOCS / "_handbook-build.md"
    build_input.write_text("\n\n".join(parts), encoding="utf-8")
    try:
        subprocess.run(
            [
                "pandoc",
                build_input.name,
                "--from=gfm+raw_html+pipe_tables+strikeout+task_lists+attributes",
                "--to=html5",
                "--standalone",
                "--toc",
                "--toc-depth=3",
                "--section-divs",
                "--metadata",
                "title=HaloFPX Persistent KV-Cache Wiki",
                "--metadata",
                "lang=en",
                "--css",
                "assets/extra.css",
                "--embed-resources",
                "--resource-path=.:..",
                "-o",
                str(SITE / "index.html"),
            ],
            cwd=DOCS,
            check=True,
        )
    finally:
        build_input.unlink(missing_ok=True)
    (SITE / "README.txt").write_text(
        "HaloFPX Persistent KV-Cache Wiki — Offline handbook\n"
        "Research cut: 2026-07-17\n\n"
        "Open index.html in a modern browser. Styling and diagrams are embedded. "
        "External primary-source links require network access.\n"
        "The editable Wiki source is in ../docs/ plus the standard root Wiki files.\n",
        encoding="utf-8",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
