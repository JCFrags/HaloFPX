#!/usr/bin/env python3
"""Check relative Markdown links in the wiki tree."""
from __future__ import annotations
import re
import sys
from pathlib import Path
from urllib.parse import unquote

LINK = re.compile(r"(?<!!)\[[^\]]*\]\(([^)]+)\)")
IMAGE = re.compile(r"!\[[^\]]*\]\(([^)]+)\)")

def check(root: Path) -> list[str]:
    errors: list[str] = []
    for md in root.rglob("*.md"):
        text = md.read_text(encoding="utf-8")
        for raw in LINK.findall(text) + IMAGE.findall(text):
            target = raw.strip().split()[0].strip("<>")
            if target.startswith(("http://", "https://", "mailto:", "#")):
                continue
            target = unquote(target.split("#", 1)[0])
            if not target:
                continue
            p = (md.parent / target).resolve()
            try:
                p.relative_to(root.resolve())
            except ValueError:
                errors.append(f"{md.relative_to(root)}: link escapes root: {raw}")
                continue
            if not p.exists():
                errors.append(f"{md.relative_to(root)}: missing target: {raw}")
    return errors


def main() -> int:
    root = Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
    errors = check(root)
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print(f"checked relative links under {root}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
