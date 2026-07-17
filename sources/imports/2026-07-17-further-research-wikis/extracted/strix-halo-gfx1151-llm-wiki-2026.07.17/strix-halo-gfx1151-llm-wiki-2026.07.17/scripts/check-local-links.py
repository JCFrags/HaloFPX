#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path
from urllib.parse import unquote
from bs4 import BeautifulSoup

ROOT = Path(__file__).resolve().parents[1]
errors: list[str] = []
MD_LINK = re.compile(r"!?\[[^\]]*\]\(([^)]+)\)")


def local_target(base: Path, href: str):
    href = href.strip().strip('<>')
    if not href or href.startswith(('#','http://','https://','mailto:','data:','javascript:')): return None
    href = unquote(href.split('#',1)[0].split('?',1)[0])
    if not href: return None
    return (base / href).resolve()

for md in list(ROOT.glob('*.md')) + list((ROOT/'docs').rglob('*.md')) + list((ROOT/'containers').glob('*.md')) + list((ROOT/'sources').glob('*.md')):
    text = md.read_text(encoding='utf-8')
    for href in MD_LINK.findall(text):
        target = local_target(md.parent, href)
        if target is not None and not target.exists(): errors.append(f"{md.relative_to(ROOT)} -> {href}")

site = ROOT / 'site'
for html in site.rglob('*.html'):
    soup = BeautifulSoup(html.read_text(encoding='utf-8'), 'html.parser')
    for tag in soup.find_all(href=True):
        href = tag['href']
        target = local_target(html.parent, href)
        if target is not None and not target.exists(): errors.append(f"{html.relative_to(ROOT)} -> {href}")
    for tag in soup.find_all(src=True):
        target = local_target(html.parent, tag['src'])
        if target is not None and not target.exists(): errors.append(f"{html.relative_to(ROOT)} -> {tag['src']}")

if errors:
    print('Broken local links:', file=sys.stderr)
    for e in sorted(set(errors)): print('- ' + e, file=sys.stderr)
    sys.exit(1)
print('Local Markdown and HTML links are valid')
