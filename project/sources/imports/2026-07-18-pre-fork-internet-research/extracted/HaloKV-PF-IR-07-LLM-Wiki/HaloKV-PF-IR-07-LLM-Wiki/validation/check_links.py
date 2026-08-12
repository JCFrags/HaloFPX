#!/usr/bin/env python3
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urlsplit
import sys

ROOT = Path(__file__).resolve().parents[1]

class LinkParser(HTMLParser):
    def __init__(self):
        super().__init__()
        self.links = []
        self.ids = set()
    def handle_starttag(self, tag, attrs):
        d = dict(attrs)
        if 'id' in d:
            self.ids.add(d['id'])
        if tag in {'a','link','script'}:
            value = d.get('href') if tag != 'script' else d.get('src')
            if value:
                self.links.append(value)

def parse_html(path):
    p = LinkParser()
    p.feed(path.read_text(encoding='utf-8'))
    return p

html_files = sorted(ROOT.rglob('*.html'))
parsed = {p: parse_html(p) for p in html_files}
errors = []
checked = 0
for src, parser in parsed.items():
    for raw in parser.links:
        if raw.startswith(('http://','https://','mailto:','data:','javascript:')):
            continue
        split = urlsplit(raw)
        path_part = unquote(split.path)
        if not path_part:
            target = src
        elif path_part.startswith('/'):
            target = ROOT / path_part.lstrip('/')
        else:
            target = (src.parent / path_part).resolve()
        checked += 1
        try:
            target.relative_to(ROOT.resolve())
        except ValueError:
            errors.append(f'{src.relative_to(ROOT)} -> escapes bundle: {raw}')
            continue
        if not target.exists():
            errors.append(f'{src.relative_to(ROOT)} -> missing: {raw}')
            continue
        if split.fragment and target.suffix.lower() == '.html':
            tparser = parsed.get(target)
            if tparser is None:
                tparser = parse_html(target)
                parsed[target] = tparser
            if split.fragment not in tparser.ids:
                errors.append(f'{src.relative_to(ROOT)} -> missing anchor: {raw}')

if errors:
    print(f'FAIL: {len(errors)} broken local references across {len(html_files)} HTML files')
    for e in errors[:100]: print(e)
    sys.exit(1)
print(f'PASS: {checked} local references across {len(html_files)} HTML files')
