#!/usr/bin/env python3
"""Render the Markdown knowledge base as a self-contained offline wiki."""
from __future__ import annotations

import argparse
import json
import os
import re
import shutil
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

import mistune
from bs4 import BeautifulSoup
from jinja2 import Environment, BaseLoader, select_autoescape

ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs"
SITE = ROOT / "site"

NAV = [
    ("Start", [
        ("Overview", "index.html"),
        ("Decision tree", "decision-tree.html"),
        ("Evidence model", "evidence-model.html"),
    ]),
    ("Compatibility", [
        ("Versioned matrix", "compatibility-matrix.html"),
        ("Official support", "official-support.html"),
        ("Community validation", "community-validation.html"),
        ("Known regressions", "regressions.html"),
        ("Unsupported combinations", "unsupported-combinations.html"),
    ]),
    ("Build and operation", [
        ("Exact build flags", "build-flags.html"),
        ("Environment variables", "environment-variables.html"),
        ("Diagnostics", "diagnostics.html"),
        ("Containers", "containers.html"),
        ("Reproducibility", "reproducibility.html"),
    ]),
    ("Components", [
        ("ROCmFPX", "rocmfpx.html"),
        ("USB4 networking", "usb4-networking.html"),
        ("Glossary", "glossary.html"),
        ("Sources", "sources.html"),
    ]),
    ("Recipes", [
        ("ROCm 7.14 host", "recipes/host-official-rocm-714.html"),
        ("ROCm 7.14 tarball", "recipes/rocm714-tarball.html"),
        ("llama.cpp HIP / ROCm 7.2.1", "recipes/llama-hip-rocm721.html"),
        ("Fedora / ROCm 7.2.4", "recipes/community-fedora-rocm724.html"),
        ("llama.cpp RADV", "recipes/llama-vulkan-radv.html"),
        ("Mesa 26.1.5 source", "recipes/mesa-2615-source.html"),
        ("llama.cpp b10064 binary", "recipes/prebuilt-llama-b10064.html"),
        ("ROCmFPX pinned", "recipes/rocmfpx-pinned.html"),
        ("USB4 IP", "recipes/usb4-ip-network.html"),
        ("USB4 RDMA research", "recipes/usb4-rdma-experimental.html"),
    ]),
    ("Versions", [("2026.07.17", "versions/2026.07.17.html")]),
]


def slugify(text: str) -> str:
    text = BeautifulSoup(text, "html.parser").get_text(" ")
    text = text.strip().lower()
    text = re.sub(r"[`*_~]", "", text)
    text = re.sub(r"[^a-z0-9]+", "-", text).strip("-")
    return text or "section"


class AnchorRenderer(mistune.HTMLRenderer):
    def __init__(self) -> None:
        super().__init__(escape=False)
        self._anchors: dict[str, int] = {}

    def heading(self, text: str, level: int, **attrs) -> str:
        base = slugify(text)
        count = self._anchors.get(base, 0)
        self._anchors[base] = count + 1
        anchor = base if count == 0 else f"{base}-{count + 1}"
        return f'<h{level} id="{anchor}">{text}<a class="heading-anchor" href="#{anchor}" aria-label="Link to section">#</a></h{level}>\n'


PAGE_TEMPLATE = r"""
<!doctype html>
<html lang="en" data-theme="dark">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <meta name="color-scheme" content="dark light">
  <meta name="description" content="Strix Halo gfx1151 Linux, ROCm, Vulkan, llama.cpp and USB4 compatibility wiki">
  <title>{{ title }} · Strix Halo Wiki</title>
  <link rel="stylesheet" href="{{ root_prefix }}assets/wiki.css">
</head>
<body data-root="{{ root_prefix }}" data-page="{{ page_path }}">
  <a class="skip-link" href="#content">Skip to content</a>
  <header class="topbar">
    <button class="icon-button mobile-menu" id="menuButton" aria-label="Open navigation">☰</button>
    <a class="brand" href="{{ root_prefix }}index.html"><span class="brand-mark">SH</span><span>Strix Halo Wiki</span></a>
    <span class="top-status">gfx1151 · 2026.07.17</span>
    <button class="search-button" id="searchButton" aria-label="Search"><span>Search</span><kbd>/</kbd></button>
    <button class="icon-button" id="themeButton" aria-label="Toggle theme">◐</button>
  </header>
  <div class="layout">
    <aside class="sidebar" id="sidebar">
      <div class="sidebar-meta">
        <div class="target-chip">AMD RDNA 3.5</div>
        <div class="snapshot">Snapshot 2026.07.17</div>
      </div>
      <nav aria-label="Wiki navigation">
      {% for group, items in nav %}
        <section class="nav-group">
          <h2>{{ group }}</h2>
          {% for label, href in items %}
            <a href="{{ root_prefix }}{{ href }}"{% if href == page_path %} class="active" aria-current="page"{% endif %}>{{ label }}</a>
          {% endfor %}
        </section>
      {% endfor %}
      </nav>
      <div class="sidebar-footer">
        <a href="{{ root_prefix }}../data/compatibility-matrix-2026.07.17.csv">CSV</a>
        <a href="{{ root_prefix }}../data/compatibility-matrix-2026.07.17.json">JSON</a>
        <a href="{{ root_prefix }}../README.md">README</a>
      </div>
    </aside>
    <main id="content" class="content">
      <div class="page-kicker">{{ section }}</div>
      <article class="article">{{ body | safe }}</article>
      <footer class="page-footer">
        <span>Static research snapshot. Verify live support matrices before changing a production host.</span>
        <a href="{{ root_prefix }}sources.html">Source registry</a>
      </footer>
    </main>
    <aside class="toc" aria-label="On this page">
      <h2>On this page</h2>
      {{ toc | safe }}
    </aside>
  </div>
  <dialog class="search-dialog" id="searchDialog">
    <form method="dialog" class="search-head">
      <input id="searchInput" type="search" placeholder="Search the wiki" autocomplete="off" aria-label="Search query">
      <button class="icon-button" value="cancel" aria-label="Close search">×</button>
    </form>
    <div id="searchResults" class="search-results"><p>Type a component, version, flag, or regression.</p></div>
  </dialog>
  <script src="{{ root_prefix }}assets/wiki.js"></script>
</body>
</html>
"""

CSS = r"""
:root {
  --bg: #0b0f14; --surface: #111821; --surface-2: #17212d; --surface-3: #1d2a38;
  --text: #d9e2ec; --muted: #8ea0b5; --border: #263548; --accent: #75b7ff;
  --accent-2: #8be9c7; --warn: #ffc857; --danger: #ff7b86; --ok: #72d572;
  --code: #0a0e13; --shadow: 0 16px 50px rgba(0,0,0,.35); --topbar: 56px;
  font-family: Inter, ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
}
html[data-theme="light"] {
  --bg:#f6f8fb; --surface:#ffffff; --surface-2:#eef3f8; --surface-3:#e2eaf3;
  --text:#17202b; --muted:#5f6f82; --border:#ced8e4; --accent:#1769aa; --accent-2:#087f5b;
  --warn:#9a6700; --danger:#b4232f; --ok:#2d7d32; --code:#f0f3f6; --shadow:0 16px 50px rgba(18,38,63,.12);
}
* { box-sizing: border-box; }
html { scroll-behavior: smooth; background: var(--bg); }
body { margin:0; color:var(--text); background:var(--bg); font-size:15px; line-height:1.65; }
a { color:var(--accent); text-decoration:none; }
a:hover { text-decoration:underline; }
.skip-link { position:fixed; left:12px; top:-80px; z-index:200; padding:8px 12px; background:var(--surface); border:1px solid var(--accent); }
.skip-link:focus { top:8px; }
.topbar { height:var(--topbar); position:fixed; inset:0 0 auto 0; z-index:100; display:flex; align-items:center; gap:18px; padding:0 18px; background:color-mix(in srgb,var(--surface) 93%,transparent); border-bottom:1px solid var(--border); backdrop-filter:blur(14px); }
.brand { display:flex; align-items:center; gap:10px; color:var(--text); font-weight:720; letter-spacing:-.01em; }
.brand:hover { text-decoration:none; }
.brand-mark { display:grid; place-items:center; width:30px; height:30px; border-radius:8px; background:linear-gradient(135deg,var(--accent),var(--accent-2)); color:#071018; font:800 12px/1 ui-monospace,monospace; }
.top-status { color:var(--muted); font:12px/1.2 ui-monospace,SFMono-Regular,Consolas,monospace; }
.search-button { margin-left:auto; min-width:230px; height:34px; display:flex; justify-content:space-between; align-items:center; border:1px solid var(--border); border-radius:8px; background:var(--surface-2); color:var(--muted); padding:0 10px; cursor:pointer; }
kbd { border:1px solid var(--border); border-bottom-width:2px; border-radius:5px; padding:1px 7px; background:var(--surface); color:var(--muted); font:11px ui-monospace,monospace; }
.icon-button { border:1px solid var(--border); border-radius:8px; background:var(--surface-2); color:var(--text); min-width:34px; height:34px; cursor:pointer; }
.mobile-menu { display:none; }
.layout { display:grid; grid-template-columns:260px minmax(0,1fr) 220px; max-width:1600px; margin:0 auto; padding-top:var(--topbar); min-height:100vh; }
.sidebar { position:sticky; top:var(--topbar); height:calc(100vh - var(--topbar)); overflow:auto; padding:20px 16px 28px; border-right:1px solid var(--border); background:var(--surface); }
.sidebar-meta { padding:8px 8px 18px; border-bottom:1px solid var(--border); margin-bottom:14px; }
.target-chip { display:inline-block; color:var(--accent-2); font:700 11px ui-monospace,monospace; text-transform:uppercase; letter-spacing:.08em; }
.snapshot { color:var(--muted); font:12px ui-monospace,monospace; margin-top:4px; }
.nav-group { margin:16px 0; }
.nav-group h2 { margin:0 8px 5px; font-size:11px; color:var(--muted); text-transform:uppercase; letter-spacing:.1em; }
.nav-group a { display:block; padding:6px 9px; margin:2px 0; border-radius:7px; color:var(--muted); font-size:13px; }
.nav-group a:hover { color:var(--text); background:var(--surface-2); text-decoration:none; }
.nav-group a.active { color:var(--text); background:var(--surface-3); border-left:2px solid var(--accent); padding-left:7px; }
.sidebar-footer { display:flex; gap:12px; padding:16px 8px; border-top:1px solid var(--border); font:11px ui-monospace,monospace; }
.content { min-width:0; padding:48px clamp(28px,5vw,78px) 80px; }
.page-kicker { color:var(--accent-2); text-transform:uppercase; letter-spacing:.11em; font:700 11px ui-monospace,monospace; margin-bottom:12px; }
.article { max-width:930px; }
.article h1 { font-size:clamp(31px,4vw,48px); line-height:1.08; letter-spacing:-.035em; margin:0 0 26px; color:var(--text); }
.article h2 { font-size:25px; letter-spacing:-.02em; margin:46px 0 16px; padding-top:4px; border-top:1px solid var(--border); }
.article h3 { font-size:19px; margin:30px 0 10px; }
.article h4 { font-size:15px; margin:24px 0 8px; color:var(--accent-2); }
.heading-anchor { opacity:0; margin-left:8px; font:400 .72em ui-monospace,monospace; }
h1:hover .heading-anchor,h2:hover .heading-anchor,h3:hover .heading-anchor,h4:hover .heading-anchor { opacity:.65; text-decoration:none; }
p,ul,ol { max-width:82ch; }
li { margin:.22em 0; }
strong { color:color-mix(in srgb,var(--text) 94%,white); }
code { padding:.14em .38em; border:1px solid var(--border); border-radius:5px; background:var(--code); color:var(--accent-2); font: .88em ui-monospace,SFMono-Regular,Consolas,monospace; word-break:break-word; }
pre { position:relative; overflow:auto; margin:18px 0 24px; padding:18px 20px; border:1px solid var(--border); border-radius:10px; background:var(--code); box-shadow:inset 0 1px rgba(255,255,255,.025); }
pre code { padding:0; border:0; background:transparent; color:var(--text); word-break:normal; }
.copy-code { position:absolute; top:8px; right:8px; opacity:.4; border:1px solid var(--border); border-radius:6px; background:var(--surface); color:var(--muted); padding:4px 8px; cursor:pointer; font:11px ui-monospace,monospace; }
pre:hover .copy-code { opacity:1; }
blockquote { margin:20px 0; padding:12px 18px; border-left:3px solid var(--accent); background:var(--surface-2); border-radius:0 8px 8px 0; }
blockquote p { margin:0; }
.callout { margin:20px 0; padding:14px 18px; border:1px solid var(--border); border-left-width:4px; border-radius:9px; background:var(--surface-2); }
.callout-title { display:block; margin-bottom:4px; font:750 12px ui-monospace,monospace; text-transform:uppercase; letter-spacing:.08em; }
.callout-important,.callout-note { border-left-color:var(--accent); }
.callout-caution,.callout-warning { border-left-color:var(--warn); }
.callout-danger { border-left-color:var(--danger); }
.callout-tip { border-left-color:var(--ok); }
.table-wrap { overflow:auto; margin:20px 0 28px; border:1px solid var(--border); border-radius:10px; }
table { border-collapse:collapse; min-width:100%; font-size:12.5px; line-height:1.45; background:var(--surface); }
th { position:sticky; top:0; z-index:1; text-align:left; color:var(--text); background:var(--surface-3); font-weight:700; }
th,td { padding:10px 11px; border-bottom:1px solid var(--border); border-right:1px solid var(--border); vertical-align:top; }
tr:last-child td { border-bottom:0; }
td:last-child,th:last-child { border-right:0; }
tr:hover td { background:color-mix(in srgb,var(--surface-2) 65%,transparent); }
.badge { display:inline-block; padding:4px 8px; border:1px solid var(--border); border-radius:999px; margin:0 5px 8px 0; color:var(--muted); background:var(--surface-2); font:700 11px ui-monospace,monospace; }
.badge-version { color:var(--accent); }.badge-target { color:var(--accent-2); }.badge-static { color:var(--warn); }
.toc { position:sticky; top:var(--topbar); height:calc(100vh - var(--topbar)); overflow:auto; padding:44px 20px; border-left:1px solid var(--border); }
.toc h2 { margin:0 0 10px; font-size:11px; text-transform:uppercase; letter-spacing:.1em; color:var(--muted); }
.toc a { display:block; padding:4px 0; color:var(--muted); font-size:12px; line-height:1.35; }
.toc a.level-3 { padding-left:12px; }
.toc a:hover { color:var(--text); text-decoration:none; }
.page-footer { display:flex; justify-content:space-between; gap:18px; max-width:930px; margin-top:70px; padding-top:18px; border-top:1px solid var(--border); color:var(--muted); font-size:12px; }
.search-dialog { width:min(760px,calc(100vw - 28px)); max-height:80vh; border:1px solid var(--border); border-radius:12px; background:var(--surface); color:var(--text); box-shadow:var(--shadow); padding:0; }
.search-dialog::backdrop { background:rgba(0,0,0,.58); backdrop-filter:blur(3px); }
.search-head { display:flex; gap:10px; padding:14px; border-bottom:1px solid var(--border); }
.search-head input { flex:1; height:42px; border:1px solid var(--border); border-radius:8px; padding:0 12px; background:var(--bg); color:var(--text); font:15px inherit; }
.search-results { max-height:62vh; overflow:auto; padding:10px; }
.search-result { display:block; padding:12px; border-radius:8px; color:var(--text); }
.search-result:hover { background:var(--surface-2); text-decoration:none; }
.search-result strong { display:block; }
.search-result span { display:block; color:var(--muted); font-size:12px; margin-top:3px; }
mark { background:color-mix(in srgb,var(--warn) 35%,transparent); color:inherit; }
@media (max-width:1180px) { .layout{grid-template-columns:240px minmax(0,1fr)} .toc{display:none} }
@media (max-width:780px) {
  .top-status{display:none}.search-button{min-width:0;width:38px;padding:0;justify-content:center}.search-button span{display:none}.search-button kbd{border:0;background:transparent;padding:0}.mobile-menu{display:block}
  .layout{display:block}.sidebar{position:fixed;z-index:90;top:var(--topbar);left:0;width:280px;transform:translateX(-105%);transition:transform .2s ease;box-shadow:var(--shadow)}.sidebar.open{transform:translateX(0)}
  .content{padding:34px 20px 65px}.article h1{font-size:34px}.article h2{font-size:22px}.page-footer{display:block}.page-footer a{display:block;margin-top:8px}
}
"""

JS = r"""
(() => {
  const doc = document.documentElement;
  const root = document.body.dataset.root || '';
  const saved = localStorage.getItem('strix-wiki-theme');
  if (saved) doc.dataset.theme = saved;
  document.getElementById('themeButton')?.addEventListener('click', () => {
    const next = doc.dataset.theme === 'light' ? 'dark' : 'light';
    doc.dataset.theme = next; localStorage.setItem('strix-wiki-theme', next);
  });
  const sidebar = document.getElementById('sidebar');
  document.getElementById('menuButton')?.addEventListener('click', () => sidebar?.classList.toggle('open'));
  document.querySelectorAll('.sidebar a').forEach(a => a.addEventListener('click', () => sidebar?.classList.remove('open')));

  document.querySelectorAll('pre').forEach(pre => {
    const b = document.createElement('button'); b.className='copy-code'; b.textContent='copy';
    b.addEventListener('click', async () => {
      const code = pre.querySelector('code')?.innerText || pre.innerText;
      await navigator.clipboard.writeText(code.replace(/copy$/, ''));
      b.textContent='copied'; setTimeout(() => b.textContent='copy', 1200);
    }); pre.appendChild(b);
  });

  const dialog = document.getElementById('searchDialog');
  const input = document.getElementById('searchInput');
  const results = document.getElementById('searchResults');
  let index = null;
  async function openSearch() {
    dialog?.showModal(); input?.focus();
    if (!index) index = await fetch(root + 'search-index.json').then(r => r.json());
  }
  document.getElementById('searchButton')?.addEventListener('click', openSearch);
  window.addEventListener('keydown', e => {
    if (e.key === '/' && !['INPUT','TEXTAREA'].includes(document.activeElement?.tagName)) { e.preventDefault(); openSearch(); }
    if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'k') { e.preventDefault(); openSearch(); }
  });
  input?.addEventListener('input', () => {
    const q = input.value.trim().toLowerCase();
    if (!q) { results.innerHTML='<p>Type a component, version, flag, or regression.</p>'; return; }
    const terms = q.split(/\s+/).filter(Boolean);
    const hits = (index || []).map(item => {
      const hay = (item.title+' '+item.headings.join(' ')+' '+item.text).toLowerCase();
      let score = 0; terms.forEach(t => { if (item.title.toLowerCase().includes(t)) score+=8; if (item.headings.join(' ').toLowerCase().includes(t)) score+=4; score += Math.min(3,(hay.split(t).length-1)); });
      return {item,score};
    }).filter(x => x.score>0).sort((a,b)=>b.score-a.score).slice(0,20);
    if (!hits.length) { results.innerHTML='<p>No matches.</p>'; return; }
    results.innerHTML = hits.map(({item}) => `<a class="search-result" href="${root}${item.path}"><strong>${escapeHtml(item.title)}</strong><span>${escapeHtml(item.section)} · ${escapeHtml(item.snippet)}</span></a>`).join('');
  });
  function escapeHtml(s){ return s.replace(/[&<>'"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;',"'":'&#39;','"':'&quot;'}[c])); }
})();
"""


def rel_root(output_path: Path) -> str:
    depth = len(output_path.relative_to(SITE).parents) - 1
    return "../" * depth


def section_for(path: Path) -> str:
    rel = path.relative_to(DOCS)
    if rel.parts[0] == "recipes": return "Reproducible recipe"
    if rel.parts[0] == "versions": return "Version history"
    groups = {
        "index.md": "Knowledge base", "compatibility-matrix.md": "Compatibility",
        "official-support.md": "Support classification", "community-validation.md": "Support classification",
        "regressions.md": "Failure analysis", "unsupported-combinations.md": "Failure analysis",
        "build-flags.md": "Build system", "environment-variables.md": "Runtime control",
        "diagnostics.md": "Operations", "containers.md": "Reproducibility",
        "reproducibility.md": "Reproducibility", "rocmfpx.md": "Experimental component",
        "usb4-networking.md": "Networking", "sources.md": "Evidence",
        "evidence-model.md": "Evidence", "decision-tree.md": "Selection guide", "glossary.md": "Reference",
    }
    return groups.get(rel.as_posix(), "Wiki")


def rewrite_links(soup: BeautifulSoup, src_path: Path, out_path: Path) -> None:
    for a in soup.find_all("a", href=True):
        href = a["href"]
        if re.match(r"^(?:[a-z]+:|//|#)", href, re.I):
            if href.startswith("http"):
                a["target"] = "_blank"; a["rel"] = "noopener noreferrer"
            continue
        target, hashmark, fragment = href.partition("#")
        if target.endswith(".md"):
            resolved = (src_path.parent / target).resolve()
            try:
                rel_doc = resolved.relative_to(DOCS.resolve())
            except ValueError:
                continue
            target_html = rel_doc.with_suffix(".html")
            rel = os.path.relpath(SITE / target_html, out_path.parent).replace(os.sep, "/")
            a["href"] = rel + (("#" + fragment) if hashmark else "")


def transform_html(html: str, src_path: Path, out_path: Path):
    soup = BeautifulSoup(html, "html.parser")
    rewrite_links(soup, src_path, out_path)
    for block in list(soup.find_all("blockquote")):
        first = block.find("p")
        if not first: continue
        text = first.get_text(" ", strip=True)
        m = re.match(r"^\[!(IMPORTANT|NOTE|TIP|WARNING|CAUTION|DANGER)\]\s*", text, re.I)
        if not m: continue
        kind = m.group(1).lower()
        for node in list(first.contents):
            if isinstance(node, str) and re.match(r"^\[![A-Z]+\]\s*", node, re.I):
                node.replace_with(re.sub(r"^\[![A-Z]+\]\s*", "", node, flags=re.I)); break
        block.name = "div"; block["class"] = ["callout", f"callout-{kind}"]
        title = soup.new_tag("span", attrs={"class": "callout-title"}); title.string = kind
        block.insert(0, title)
    for tbl in list(soup.find_all("table")):
        wrapper = soup.new_tag("div", attrs={"class": "table-wrap"})
        tbl.wrap(wrapper)
    for img in soup.find_all("img"):
        img["loading"] = "lazy"
    headings = []
    for h in soup.find_all(["h2", "h3"]):
        headings.append((int(h.name[1]), h.get("id", ""), h.get_text(" ", strip=True).rstrip("#").strip()))
    toc = "".join(f'<a class="level-{lvl}" href="#{anchor}">{text}</a>' for lvl, anchor, text in headings if anchor)
    title_tag = soup.find("h1")
    title = title_tag.get_text(" ", strip=True).rstrip("#").strip() if title_tag else src_path.stem.replace("-", " ").title()
    plain = soup.get_text(" ", strip=True)
    return str(soup), toc, title, headings, plain


def render_site(clean: bool = True) -> None:
    if clean and SITE.exists():
        for p in SITE.iterdir():
            if p.name == "assets": continue
            if p.is_dir(): shutil.rmtree(p)
            else: p.unlink()
    (SITE / "assets").mkdir(parents=True, exist_ok=True)
    (SITE / "assets" / "wiki.css").write_text(CSS.strip() + "\n", encoding="utf-8")
    (SITE / "assets" / "wiki.js").write_text(JS.strip() + "\n", encoding="utf-8")

    env = Environment(loader=BaseLoader(), autoescape=select_autoescape(["html"]))
    template = env.from_string(PAGE_TEMPLATE)
    search = []
    for src in sorted(DOCS.rglob("*.md")):
        rel = src.relative_to(DOCS)
        out_rel = rel.with_suffix(".html")
        out = SITE / out_rel
        out.parent.mkdir(parents=True, exist_ok=True)
        renderer = AnchorRenderer()
        md = mistune.create_markdown(renderer=renderer, plugins=["table", "strikethrough", "task_lists", "url"])
        raw = md(src.read_text(encoding="utf-8"))
        body, toc, title, headings, plain = transform_html(raw, src, out)
        prefix = rel_root(out)
        page = template.render(title=title, body=body, toc=toc, root_prefix=prefix, nav=NAV,
                               page_path=out_rel.as_posix(), section=section_for(src))
        out.write_text(page, encoding="utf-8")
        search.append({
            "title": title, "path": out_rel.as_posix(), "section": section_for(src),
            "headings": [h[2] for h in headings], "text": plain[:15000],
            "snippet": plain[:240] + ("…" if len(plain) > 240 else "")
        })
    (SITE / "search-index.json").write_text(json.dumps(search, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    (SITE / "404.html").write_text(template.render(
        title="Page not found", body="<h1>Page not found</h1><p>Return to the <a href=\"index.html\">wiki index</a>.</p>",
        toc="", root_prefix="", nav=NAV, page_path="404.html", section="Error"), encoding="utf-8")
    (SITE / "robots.txt").write_text("User-agent: *\nAllow: /\n", encoding="utf-8")
    print(f"Rendered {len(search)} pages to {SITE}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--no-clean", action="store_true")
    args = parser.parse_args()
    render_site(clean=not args.no_clean)

if __name__ == "__main__":
    main()
