#!/usr/bin/env python3
"""Build the browser-ready offline-first wiki from Markdown sources.

The builder intentionally has a small dependency surface and does not require MkDocs.
A compatible mkdocs.yml remains available for teams that prefer MkDocs Material.
"""
from __future__ import annotations

import html
import json
import os
import re
import shutil
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable
from urllib.parse import urlsplit, urlunsplit

import mistune
import yaml
from bs4 import BeautifulSoup

ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs"
SITE = ROOT / "site"
CONFIG = ROOT / "mkdocs.yml"


@dataclass(frozen=True)
class Page:
    source: Path
    rel_source: Path
    output: Path
    rel_output: Path
    title: str
    status: str
    section_path: tuple[str, ...]


def slugify(text: str, used: set[str]) -> str:
    base = re.sub(r"[^a-z0-9]+", "-", text.lower()).strip("-") or "section"
    slug = base
    i = 2
    while slug in used:
        slug = f"{base}-{i}"
        i += 1
    used.add(slug)
    return slug


def parse_front_matter(text: str) -> tuple[dict[str, Any], str]:
    if not text.startswith("---\n"):
        return {}, text
    end = text.find("\n---\n", 4)
    if end < 0:
        return {}, text
    metadata = yaml.safe_load(text[4:end]) or {}
    return metadata, text[end + 5 :]


def iter_nav(nav: list[Any], sections: tuple[str, ...] = ()) -> Iterable[tuple[str, str, tuple[str, ...]]]:
    for item in nav:
        if isinstance(item, str):
            path = item
            yield Path(path).stem.replace("-", " ").title(), path, sections
            continue
        if not isinstance(item, dict) or len(item) != 1:
            raise ValueError(f"unsupported nav item: {item!r}")
        label, target = next(iter(item.items()))
        if isinstance(target, str):
            yield str(label), target, sections
        elif isinstance(target, list):
            yield from iter_nav(target, sections + (str(label),))
        else:
            raise ValueError(f"unsupported nav target: {target!r}")


def build_page_records(config: dict[str, Any]) -> list[Page]:
    records: list[Page] = []
    seen: set[Path] = set()
    for nav_title, target, sections in iter_nav(config["nav"]):
        rel = Path(target)
        source = (DOCS / rel).resolve()
        if not source.is_file():
            raise FileNotFoundError(f"navigation source missing: {source}")
        metadata, body = parse_front_matter(source.read_text(encoding="utf-8"))
        first_heading = re.search(r"^#\s+(.+)$", body, flags=re.MULTILINE)
        title = str(metadata.get("title") or (first_heading.group(1) if first_heading else nav_title))
        status = str(metadata.get("status") or "reference")
        rel_output = rel.with_suffix(".html")
        records.append(
            Page(
                source=source,
                rel_source=rel,
                output=SITE / rel_output,
                rel_output=rel_output,
                title=title,
                status=status,
                section_path=sections,
            )
        )
        seen.add(source)

    # Include orphan Markdown pages so the site remains complete even if nav lags.
    for source in sorted(DOCS.rglob("*.md")):
        source = source.resolve()
        if source in seen:
            continue
        rel = source.relative_to(DOCS)
        metadata, body = parse_front_matter(source.read_text(encoding="utf-8"))
        first_heading = re.search(r"^#\s+(.+)$", body, flags=re.MULTILINE)
        title = str(metadata.get("title") or (first_heading.group(1) if first_heading else rel.stem))
        records.append(
            Page(
                source=source,
                rel_source=rel,
                output=SITE / rel.with_suffix(".html"),
                rel_output=rel.with_suffix(".html"),
                title=title,
                status=str(metadata.get("status") or "reference"),
                section_path=("Unlisted",),
            )
        )
    return records


def output_for_source(path: Path) -> Path | None:
    try:
        rel = path.resolve().relative_to(DOCS.resolve())
    except ValueError:
        return None
    if rel.suffix.lower() != ".md":
        return None
    return SITE / rel.with_suffix(".html")


def site_copy_target(path: Path) -> Path | None:
    try:
        rel = path.resolve().relative_to(ROOT.resolve())
    except ValueError:
        return None
    # The copied project artifacts preserve repository-relative paths under site/.
    return SITE / rel


def rewrite_local_url(raw: str, page: Page) -> str:
    if not raw or raw.startswith("#"):
        return raw
    parts = urlsplit(raw)
    if parts.scheme or parts.netloc or raw.startswith("//"):
        return raw
    path_part = parts.path
    if not path_part:
        return raw
    source_target = (page.source.parent / path_part).resolve()
    target_output = output_for_source(source_target)
    if target_output is None:
        target_output = site_copy_target(source_target)
    if target_output is None:
        return raw
    rel = os.path.relpath(target_output, page.output.parent).replace(os.sep, "/")
    return urlunsplit(("", "", rel, parts.query, parts.fragment))


def enhance_html(rendered: str, page: Page) -> tuple[str, list[dict[str, str]], str]:
    soup = BeautifulSoup(rendered, "html.parser")
    used: set[str] = set()
    toc: list[dict[str, str]] = []
    for heading in soup.find_all(["h1", "h2", "h3"]):
        text = heading.get_text(" ", strip=True)
        heading_id = slugify(text, used)
        heading["id"] = heading_id
        if heading.name != "h1":
            toc.append({"level": heading.name, "text": text, "id": heading_id})

    for tag in soup.find_all(["a", "img"]):
        attr = "href" if tag.name == "a" else "src"
        if tag.has_attr(attr):
            tag[attr] = rewrite_local_url(str(tag[attr]), page)
        if tag.name == "a" and str(tag.get("href", "")).startswith(("http://", "https://")):
            tag["rel"] = "noreferrer"
            tag["target"] = "_blank"

    # Label evidence terms visibly without changing source Markdown.
    label_classes = {
        "SOURCED FACT": "sourced",
        "CALCULATED LOWER BOUND": "calculated",
        "CALCULATED": "calculated",
        "MEASURED INPUT REQUIRED": "measured",
        "SCENARIO ASSUMPTION": "assumption",
        "DECISION RULE": "rule",
    }
    for text_node in list(soup.find_all(string=True)):
        parent = text_node.parent
        if parent and parent.name in {"code", "pre", "script", "style", "a"}:
            continue
        value = str(text_node)
        matches = []
        for label, cls in label_classes.items():
            for match in re.finditer(rf"\b{re.escape(label)}\b", value):
                matches.append((match.start(), match.end(), label, cls))
        if not matches:
            continue
        # Prefer longest label at the same start and avoid overlaps.
        matches.sort(key=lambda x: (x[0], -(x[1] - x[0])))
        selected: list[tuple[int, int, str, str]] = []
        cursor = -1
        for match in matches:
            if match[0] >= cursor:
                selected.append(match)
                cursor = match[1]
        if not selected:
            continue
        fragments: list[Any] = []
        pos = 0
        for start, end, label, cls in selected:
            if start > pos:
                fragments.append(value[pos:start])
            span = soup.new_tag("span")
            span["class"] = ["evidence", f"evidence-{cls}"]
            span.string = label
            fragments.append(span)
            pos = end
        if pos < len(value):
            fragments.append(value[pos:])
        for fragment in reversed(fragments):
            text_node.insert_after(fragment)
        text_node.extract()

    plain = soup.get_text(" ", strip=True)
    return str(soup), toc, plain


def rel_href(current: Page, target: Page) -> str:
    return os.path.relpath(target.output, current.output.parent).replace(os.sep, "/")


def render_nav(config_nav: list[Any], page: Page, pages_by_rel: dict[str, Page], depth: int = 0) -> str:
    out = [f'<ul class="nav-level nav-level-{depth}">']
    for item in config_nav:
        if isinstance(item, str):
            target = item
            label = Path(target).stem.replace("-", " ").title()
            child = None
        else:
            label, child = next(iter(item.items()))
            target = child if isinstance(child, str) else None
        if target is not None:
            target_page = pages_by_rel[str(Path(target))]
            active = " active" if target_page.rel_source == page.rel_source else ""
            href = rel_href(page, target_page)
            out.append(
                f'<li class="nav-page{active}"><a href="{html.escape(href)}">'
                f'{html.escape(str(label))}</a></li>'
            )
        else:
            out.append(f'<li class="nav-section"><span>{html.escape(str(label))}</span>')
            out.append(render_nav(child, page, pages_by_rel, depth + 1))
            out.append("</li>")
    out.append("</ul>")
    return "".join(out)


def render_toc(toc: list[dict[str, str]]) -> str:
    if not toc:
        return '<p class="toc-empty">No subsections</p>'
    rows = ['<ol class="page-toc-list">']
    for item in toc:
        rows.append(
            f'<li class="toc-{item["level"]}"><a href="#{html.escape(item["id"])}">'
            f'{html.escape(item["text"])}</a></li>'
        )
    rows.append("</ol>")
    return "".join(rows)


def breadcrumbs(page: Page) -> str:
    parts = ['<a href="{}">Home</a>'.format(html.escape(os.path.relpath(SITE / "index.html", page.output.parent).replace(os.sep, "/")))]
    parts.extend(html.escape(x) for x in page.section_path)
    parts.append(html.escape(page.title))
    return '<span class="crumb-sep">/</span>'.join(parts)


PAGE_TEMPLATE = """<!doctype html>
<html lang="en" data-theme="dark">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta name="description" content="{description}">
  <title>{title} · Dual-Strix-Halo USB4 LLM Wiki</title>
  <link rel="stylesheet" href="{css_href}">
  <script>window.WIKI_SEARCH_INDEX={search_json};window.WIKI_PAGE_ROOT={page_root_json};</script>
  <script defer src="{js_href}"></script>
  <script>
    window.MathJax={{tex:{{inlineMath:[["\\\\(","\\\\)"],["$","$"]],displayMath:[["\\\\[","\\\\]"],["$$","$$"]],processEscapes:true}},svg:{{fontCache:"global"}}}};
  </script>
  <script defer src="{mathjax_href}"></script>
</head>
<body>
  <header class="topbar">
    <button class="icon-button mobile-only" id="nav-toggle" aria-label="Toggle navigation">☰</button>
    <a class="brand" href="{home_href}"><span class="brand-mark">∥</span><span><b>LLM WIKI</b><small>dual Strix Halo · dual USB4</small></span></a>
    <div class="top-actions">
      <button class="icon-button" id="search-toggle" aria-label="Search">⌕</button>
      <button class="icon-button" id="theme-toggle" aria-label="Toggle theme">◐</button>
    </div>
  </header>
  <div class="search-panel" id="search-panel" hidden>
    <div class="search-box-wrap">
      <label for="wiki-search">Search the wiki</label>
      <input id="wiki-search" type="search" autocomplete="off" placeholder="Mode, formula, ownership, model…">
      <div id="search-results" class="search-results"></div>
    </div>
  </div>
  <div class="layout">
    <aside class="sidebar" id="sidebar">
      <div class="sidebar-intro"><span class="signal"></span>Research edition <b>v{version}</b></div>
      <nav aria-label="Wiki navigation">{nav_html}</nav>
      <div class="sidebar-foot">No benchmark performance embedded.<br>Measured inputs fail closed.</div>
    </aside>
    <main class="main">
      <div class="breadcrumbs">{breadcrumbs}</div>
      <article class="article">
        <div class="article-meta"><span class="status-chip">{status}</span><a href="{source_href}">Markdown source</a></div>
        {content}
      </article>
      <footer class="footer">
        <span>Dual-Strix-Halo USB4 Distributed LLM Wiki</span>
        <span>Evidence labels distinguish facts, calculations, assumptions, and required measurements.</span>
      </footer>
    </main>
    <aside class="page-toc"><div class="page-toc-inner"><b>ON THIS PAGE</b>{toc_html}</div></aside>
  </div>
</body>
</html>
"""


CSS = r"""
:root {
  --bg: #0c0d12;
  --panel: #12141b;
  --panel-2: #171a23;
  --text: #e8eaf0;
  --muted: #9ca3b4;
  --line: #2a2e3a;
  --accent: #8b7bff;
  --accent-2: #4bd5c5;
  --code: #0a0b0f;
  --shadow: rgba(0,0,0,.28);
  --max: 920px;
}
html[data-theme="light"] {
  --bg: #f6f7fa;
  --panel: #ffffff;
  --panel-2: #f0f2f7;
  --text: #181a21;
  --muted: #606777;
  --line: #d9dde7;
  --accent: #5a43d6;
  --accent-2: #087f75;
  --code: #eef0f5;
  --shadow: rgba(30,35,50,.10);
}
* { box-sizing: border-box; }
html { scroll-behavior: smooth; }
body { margin: 0; background: var(--bg); color: var(--text); font: 15px/1.65 Inter, ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif; }
a { color: var(--accent-2); text-decoration: none; }
a:hover { text-decoration: underline; }
.topbar { position: fixed; z-index: 50; inset: 0 0 auto 0; height: 64px; display: flex; align-items: center; justify-content: space-between; padding: 0 18px; background: color-mix(in srgb, var(--panel) 94%, transparent); border-bottom: 1px solid var(--line); backdrop-filter: blur(14px); }
.brand { display: flex; gap: 10px; align-items: center; color: var(--text); letter-spacing: .08em; }
.brand:hover { text-decoration: none; }
.brand-mark { display: grid; place-items: center; width: 32px; height: 32px; border: 1px solid var(--accent); color: var(--accent); border-radius: 8px; font-weight: 800; }
.brand b { display: block; font-size: 12px; }
.brand small { display: block; color: var(--muted); font-size: 10px; letter-spacing: .04em; }
.top-actions { display: flex; gap: 8px; }
.icon-button { border: 1px solid var(--line); background: var(--panel-2); color: var(--text); width: 36px; height: 36px; border-radius: 8px; cursor: pointer; font-size: 18px; }
.icon-button:hover { border-color: var(--accent); }
.layout { display: grid; grid-template-columns: 280px minmax(0, 1fr) 240px; min-height: 100vh; padding-top: 64px; }
.sidebar { position: fixed; top: 64px; bottom: 0; width: 280px; overflow: auto; padding: 22px 16px 40px; border-right: 1px solid var(--line); background: var(--panel); }
.sidebar-intro { color: var(--muted); font-size: 11px; text-transform: uppercase; letter-spacing: .08em; padding: 0 10px 18px; }
.signal { display: inline-block; width: 7px; height: 7px; border-radius: 50%; background: var(--accent-2); margin-right: 7px; box-shadow: 0 0 12px var(--accent-2); }
.nav-level { list-style: none; margin: 0; padding: 0; }
.nav-level-1 { margin: 5px 0 12px 8px; padding-left: 10px; border-left: 1px solid var(--line); }
.nav-section > span { display: block; margin: 14px 8px 5px; color: var(--muted); font-size: 10px; font-weight: 750; text-transform: uppercase; letter-spacing: .10em; }
.nav-page a { display: block; padding: 6px 9px; border-radius: 6px; color: var(--muted); font-size: 12px; }
.nav-page a:hover { color: var(--text); background: var(--panel-2); text-decoration: none; }
.nav-page.active a { color: var(--text); background: color-mix(in srgb, var(--accent) 18%, var(--panel-2)); border-left: 2px solid var(--accent); }
.sidebar-foot { margin: 24px 8px 0; padding-top: 16px; border-top: 1px solid var(--line); color: var(--muted); font-size: 10px; line-height: 1.5; }
.main { grid-column: 2; min-width: 0; }
.breadcrumbs { max-width: var(--max); margin: 0 auto; padding: 22px 34px 0; color: var(--muted); font-size: 11px; }
.crumb-sep { margin: 0 7px; color: var(--line); }
.article { max-width: var(--max); margin: 0 auto; padding: 14px 34px 80px; }
.article-meta { display: flex; align-items: center; gap: 12px; margin-bottom: 22px; color: var(--muted); font-size: 11px; }
.status-chip { display: inline-block; padding: 3px 8px; border: 1px solid var(--line); border-radius: 99px; text-transform: uppercase; letter-spacing: .06em; }
h1, h2, h3, h4 { line-height: 1.24; scroll-margin-top: 82px; }
h1 { margin: 0 0 24px; font-size: clamp(34px, 5vw, 56px); letter-spacing: -.045em; }
h2 { margin-top: 52px; padding-bottom: 9px; border-bottom: 1px solid var(--line); font-size: 25px; letter-spacing: -.02em; }
h3 { margin-top: 34px; font-size: 18px; }
p, li { max-width: 78ch; }
strong { color: color-mix(in srgb, var(--text) 92%, var(--accent)); }
blockquote { margin: 22px 0; padding: 10px 18px; border-left: 3px solid var(--accent); color: var(--muted); background: var(--panel); }
hr { border: 0; border-top: 1px solid var(--line); margin: 40px 0; }
pre { overflow: auto; padding: 17px; border: 1px solid var(--line); border-radius: 9px; background: var(--code); box-shadow: 0 8px 30px var(--shadow); }
code { font: .88em/1.5 ui-monospace, SFMono-Regular, Menlo, Consolas, monospace; background: var(--code); padding: .12em .34em; border: 1px solid var(--line); border-radius: 4px; }
pre code { padding: 0; border: 0; background: transparent; }
table { width: 100%; display: block; overflow-x: auto; border-collapse: collapse; margin: 22px 0 30px; font-size: 12px; }
th, td { padding: 9px 11px; border: 1px solid var(--line); text-align: left; vertical-align: top; }
th { background: var(--panel-2); color: var(--text); }
tr:nth-child(even) td { background: color-mix(in srgb, var(--panel) 65%, transparent); }
img { max-width: 100%; height: auto; border-radius: 8px; background: #fff; padding: 8px; border: 1px solid var(--line); }
input[type="checkbox"] { accent-color: var(--accent); }
.evidence { display: inline-block; padding: 0 .38em; margin: 0 .08em; border-radius: 999px; font-size: .72em; line-height: 1.65; font-weight: 780; letter-spacing: .035em; vertical-align: .08em; border: 1px solid var(--line); white-space: nowrap; }
.evidence-sourced { color: #66dfbd; background: rgba(44,185,145,.10); }
.evidence-calculated { color: #9ba8ff; background: rgba(99,115,255,.12); }
.evidence-measured { color: #ffcc70; background: rgba(220,155,20,.12); }
.evidence-assumption { color: #f39ac6; background: rgba(220,80,145,.12); }
.evidence-rule { color: #9cd3ff; background: rgba(30,130,220,.12); }
.page-toc { grid-column: 3; }
.page-toc-inner { position: fixed; top: 88px; width: 220px; max-height: calc(100vh - 115px); overflow: auto; padding: 0 20px; color: var(--muted); font-size: 11px; }
.page-toc-inner > b { display: block; margin-bottom: 10px; font-size: 9px; letter-spacing: .12em; }
.page-toc-list { list-style: none; margin: 0; padding: 0; border-left: 1px solid var(--line); }
.page-toc-list li { margin: 0; }
.page-toc-list a { display: block; padding: 4px 0 4px 10px; color: var(--muted); }
.page-toc-list a:hover { color: var(--text); text-decoration: none; }
.page-toc-list .toc-h3 a { padding-left: 22px; font-size: 10px; }
.toc-empty { color: var(--muted); }
.footer { max-width: var(--max); margin: 0 auto; padding: 24px 34px 40px; border-top: 1px solid var(--line); color: var(--muted); font-size: 10px; display: flex; justify-content: space-between; gap: 25px; }
.search-panel { position: fixed; z-index: 80; inset: 64px 0 0; background: rgba(0,0,0,.62); backdrop-filter: blur(7px); padding: 5vh 18px; }
.search-box-wrap { max-width: 760px; margin: 0 auto; padding: 20px; border: 1px solid var(--line); border-radius: 12px; background: var(--panel); box-shadow: 0 22px 80px rgba(0,0,0,.45); }
.search-box-wrap label { display: block; color: var(--muted); font-size: 11px; text-transform: uppercase; letter-spacing: .09em; margin-bottom: 8px; }
#wiki-search { width: 100%; padding: 14px 15px; border: 1px solid var(--line); border-radius: 8px; outline: 0; background: var(--bg); color: var(--text); font: inherit; }
#wiki-search:focus { border-color: var(--accent); }
.search-results { max-height: 58vh; overflow: auto; margin-top: 12px; }
.search-hit { display: block; padding: 12px; border-top: 1px solid var(--line); color: var(--text); }
.search-hit:hover { background: var(--panel-2); text-decoration: none; }
.search-hit b { display: block; }
.search-hit small { color: var(--muted); }
.mobile-only { display: none; }
@media (max-width: 1180px) {
  .layout { grid-template-columns: 260px minmax(0, 1fr); }
  .sidebar { width: 260px; }
  .page-toc { display: none; }
}
@media (max-width: 780px) {
  .mobile-only { display: inline-grid; place-items: center; }
  .layout { display: block; }
  .sidebar { transform: translateX(-105%); transition: transform .2s ease; z-index: 40; width: min(88vw, 320px); box-shadow: 12px 0 40px var(--shadow); }
  .sidebar.open { transform: translateX(0); }
  .article, .breadcrumbs, .footer { padding-left: 20px; padding-right: 20px; }
  .brand small { display: none; }
  .footer { display: block; }
}
@media print {
  .topbar, .sidebar, .page-toc, .breadcrumbs, .article-meta, .footer { display: none !important; }
  .layout, .main { display: block; padding: 0; }
  .article { max-width: none; padding: 0; }
  body { background: #fff; color: #000; }
  a { color: #000; text-decoration: underline; }
}
"""

JS = r"""
(() => {
  const root = document.documentElement;
  const stored = localStorage.getItem('wiki-theme');
  if (stored) root.dataset.theme = stored;
  const toggle = document.getElementById('theme-toggle');
  toggle?.addEventListener('click', () => {
    root.dataset.theme = root.dataset.theme === 'light' ? 'dark' : 'light';
    localStorage.setItem('wiki-theme', root.dataset.theme);
  });

  const sidebar = document.getElementById('sidebar');
  document.getElementById('nav-toggle')?.addEventListener('click', () => sidebar?.classList.toggle('open'));

  const panel = document.getElementById('search-panel');
  const input = document.getElementById('wiki-search');
  const results = document.getElementById('search-results');
  const closeSearch = () => { if (panel) panel.hidden = true; };
  const openSearch = () => { if (panel) { panel.hidden = false; setTimeout(() => input?.focus(), 0); } };
  document.getElementById('search-toggle')?.addEventListener('click', openSearch);
  panel?.addEventListener('click', e => { if (e.target === panel) closeSearch(); });
  document.addEventListener('keydown', e => {
    if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'k') { e.preventDefault(); openSearch(); }
    if (e.key === 'Escape') closeSearch();
  });

  const escapeHtml = value => value.replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  input?.addEventListener('input', () => {
    const q = input.value.trim().toLowerCase();
    if (!results) return;
    if (!q) { results.innerHTML = '<small>Type at least two characters. Ctrl/⌘+K opens search.</small>'; return; }
    const terms = q.split(/\s+/).filter(Boolean);
    const hits = (window.WIKI_SEARCH_INDEX || []).map(item => {
      const hay = (item.title + ' ' + item.text).toLowerCase();
      const score = terms.reduce((s, t) => s + (item.title.toLowerCase().includes(t) ? 8 : 0) + (hay.includes(t) ? 1 : -20), 0);
      return {...item, score};
    }).filter(x => x.score >= terms.length).sort((a,b) => b.score-a.score).slice(0, 20);
    results.innerHTML = hits.length ? hits.map(hit => {
      const href = window.WIKI_PAGE_ROOT + hit.path;
      return `<a class="search-hit" href="${escapeHtml(href)}"><b>${escapeHtml(hit.title)}</b><small>${escapeHtml(hit.section)} · ${escapeHtml(hit.snippet)}</small></a>`;
    }).join('') : '<small>No matching pages.</small>';
  });
})();
"""


def copy_artifacts() -> None:
    for name in ("diagrams", "placements", "schemas", "data", "tools", "vendor"):
        src = ROOT / name
        dst = SITE / name
        if src.exists():
            shutil.copytree(src, dst, dirs_exist_ok=True, ignore=shutil.ignore_patterns("__pycache__", "*.pyc"))
    for name in (
        "README.md", "SUMMARY.md", "MANIFEST.md", "AUDIT.md", "CITATIONS.md",
        "LICENSE.md", "CONTRIBUTING.md", "CODE_OF_CONDUCT.md", "SECURITY.md",
        "CHANGELOG.md", "CITATION.cff", "THIRD_PARTY_NOTICES.md", "VERSION", "mkdocs.yml",
    ):
        src = ROOT / name
        if src.exists():
            shutil.copy2(src, SITE / name)


def main() -> int:
    config = yaml.safe_load(CONFIG.read_text(encoding="utf-8"))
    version = (ROOT / "VERSION").read_text(encoding="utf-8").strip()
    if SITE.exists():
        shutil.rmtree(SITE)
    (SITE / "assets").mkdir(parents=True)
    (SITE / "assets" / "wiki.css").write_text(CSS.strip() + "\n", encoding="utf-8")
    (SITE / "assets" / "wiki.js").write_text(JS.strip() + "\n", encoding="utf-8")
    copy_artifacts()

    pages = build_page_records(config)
    pages_by_rel = {str(p.rel_source): p for p in pages}
    markdown = mistune.create_markdown(
        escape=False,
        plugins=["strikethrough", "footnotes", "table", "task_lists", "url"],
    )

    converted: dict[Path, tuple[str, list[dict[str, str]], str]] = {}
    search_rows: list[dict[str, str]] = []
    for page in pages:
        _, body = parse_front_matter(page.source.read_text(encoding="utf-8"))
        rendered = markdown(body)
        content, toc, plain = enhance_html(rendered, page)
        converted[page.rel_source] = (content, toc, plain)
        search_rows.append(
            {
                "title": page.title,
                "path": page.rel_output.as_posix(),
                "section": " / ".join(page.section_path) or "Home",
                "text": plain[:12000],
                "snippet": plain[:220],
            }
        )

    search_json = json.dumps(search_rows, ensure_ascii=False).replace("</", "<\\/")
    for page in pages:
        content, toc, plain = converted[page.rel_source]
        page.output.parent.mkdir(parents=True, exist_ok=True)
        css_href = os.path.relpath(SITE / "assets" / "wiki.css", page.output.parent).replace(os.sep, "/")
        js_href = os.path.relpath(SITE / "assets" / "wiki.js", page.output.parent).replace(os.sep, "/")
        mathjax_href = os.path.relpath(SITE / "vendor" / "mathjax" / "tex-svg.js", page.output.parent).replace(os.sep, "/")
        home_href = os.path.relpath(SITE / "index.html", page.output.parent).replace(os.sep, "/")
        source_target = SITE / "docs" / page.rel_source
        source_target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(page.source, source_target)
        source_href = os.path.relpath(source_target, page.output.parent).replace(os.sep, "/")
        page_root = os.path.relpath(SITE, page.output.parent).replace(os.sep, "/")
        if page_root == ".":
            page_root = ""
        elif not page_root.endswith("/"):
            page_root += "/"
        description = plain[:180]
        html_text = PAGE_TEMPLATE.format(
            description=html.escape(description, quote=True),
            title=html.escape(page.title),
            css_href=html.escape(css_href),
            js_href=html.escape(js_href),
            mathjax_href=html.escape(mathjax_href),
            home_href=html.escape(home_href),
            search_json=search_json,
            page_root_json=json.dumps(page_root),
            version=html.escape(version),
            nav_html=render_nav(config["nav"], page, pages_by_rel),
            breadcrumbs=breadcrumbs(page),
            status=html.escape(page.status),
            source_href=html.escape(source_href),
            content=content,
            toc_html=render_toc(toc),
        )
        page.output.write_text(html_text, encoding="utf-8")

    (SITE / "search-index.json").write_text(
        json.dumps(search_rows, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
    )
    print(f"Built {len(pages)} pages in {SITE}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
