#!/usr/bin/env python3
"""Build the dependency-light offline HTML wiki from the Markdown sources."""
from __future__ import annotations
import json, os, re, shutil
from pathlib import Path
import jinja2, mistune

ROOT=Path(__file__).resolve().parents[1]
SITE=ROOT/'site'
AS_OF=json.loads((ROOT/'data/candidates.json').read_text())['as_of']
CANDS=sorted(json.loads((ROOT/'data/candidates.json').read_text())['candidates'],key=lambda x:x['rank'])
if SITE.exists(): shutil.rmtree(SITE)
(SITE/'assets').mkdir(parents=True)
shutil.copy(ROOT/'assets/wiki.css',SITE/'assets/wiki.css')
shutil.copy(ROOT/'assets/wiki.js',SITE/'assets/wiki.js')
shutil.copytree(ROOT/'data', SITE/'data')
shutil.copytree(ROOT/'manifests', SITE/'manifests')
markdown=mistune.create_markdown(plugins=['table','strikethrough','task_lists','url'])
all_pages=[ROOT/'Home.md']+sorted((ROOT/'pages').glob('*.md'))+sorted((ROOT/'candidates').glob('*.md'))+[ROOT/'README.md',ROOT/'LICENSE-NOTES.md']
page_map={}
for p in all_pages:
    rel=p.relative_to(ROOT)
    out_rel=Path('index.html') if rel.name=='Home.md' else rel.with_suffix('.html')
    page_map=page_map | {rel.as_posix():out_rel.as_posix()}
nav_main=[
 ('Home','index.html'),('Ranked Shortlist','pages/Ranked-Shortlist.html'),('Candidate Matrix','pages/Candidate-Matrix.html'),
 ('Capacity Planning','pages/Capacity-Planning.html'),('Exact Per-Node Budgets','pages/Per-Node-Budgets.html'),('KV Cache','pages/KV-Cache-and-Buffers.html'),
 ('Runtime Support','pages/Runtime-Support.html'),('Quantization','pages/Quantization-Comparison.html'),('Provenance','pages/Download-Provenance.html'),
 ('Quality Evidence','pages/Quality-Evidence.html'),('Strix Halo','pages/Strix-Halo-Open-Questions.html'),('Methodology','pages/Methodology.html'),
 ('Reproduction','pages/Reproduction.html'),('Source Index','pages/Source-Index.html')]
nav_cands=[(c['name'],f"candidates/{c['id']}.html") for c in CANDS]
template=jinja2.Template(r'''<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>{{ title }} · 200–230 GB LLM Wiki</title><link rel="stylesheet" href="{{ root }}assets/wiki.css"></head><body><div class="layout"><aside class="sidebar"><div class="brand">LLM Capacity Wiki</div><div class="snapshot">200–230 GB · snapshot {{ as_of }}</div><input id="search" class="search" type="search" placeholder="Filter navigation"><nav class="nav"><div class="section">Research</div>{% for label,href in nav_main %}<a data-search="{{ label|lower }}" href="{{ root }}{{ href }}">{{ label }}</a>{% endfor %}<div class="section">Candidates</div>{% for label,href in nav_cands %}<a data-search="{{ label|lower }}" href="{{ root }}{{ href }}">{{ label }}</a>{% endfor %}<div class="section">Raw files</div><a data-search="data csv json" href="{{ root }}data/candidates.csv">Candidate CSV</a><a data-search="capacity csv" href="{{ root }}data/capacity_budgets.csv">Capacity CSV</a><a data-search="manifest provenance" href="{{ root }}manifests/README.md">Manifests</a></nav></aside><main class="content">{{ body|safe }}<div class="footer">Snapshot {{ as_of }} · No model weights included · Performance claims require direct measurement.</div></main></div><script src="{{ root }}assets/wiki.js"></script></body></html>''')
def rewrite(md:str,current_rel:Path)->str:
    def repl(m):
        label,target=m.group(1),m.group(2)
        if '://' in target or target.startswith(('#','mailto:')): return m.group(0)
        base=current_rel.parent; norm=(base/target).as_posix(); parts=[]
        for x in norm.split('/'):
            if x in ('','.'): continue
            if x=='..':
                if parts: parts.pop()
            else: parts.append(x)
        norm='/'.join(parts); anchor=''
        if '#' in norm: norm,anchor=norm.split('#',1); anchor='#'+anchor
        dest=page_map.get(norm)
        if not dest: return m.group(0)
        cur_out=Path(page_map[current_rel.as_posix()]).parent
        return f"[{label}]({os.path.relpath(dest,cur_out).replace(os.sep,'/')}{anchor})"
    return re.sub(r'\[([^\]]+)\]\(([^)]+)\)',repl,md)
for p in all_pages:
    rel=p.relative_to(ROOT); out_rel=Path(page_map[rel.as_posix()]); out=SITE/out_rel; out.parent.mkdir(parents=True,exist_ok=True)
    raw=p.read_text(encoding='utf-8'); body=markdown(rewrite(raw,rel)); title=re.sub(r'^#\s+','',raw.splitlines()[0]).strip(); root='../'*len(out_rel.parent.parts)
    out.write_text(template.render(title=title,body=body,root=root,as_of=AS_OF,nav_main=nav_main,nav_cands=nav_cands),encoding='utf-8')
(ROOT/'index.html').write_text('<!doctype html><meta http-equiv="refresh" content="0; url=site/index.html"><title>LLM Wiki</title><a href="site/index.html">Open wiki</a>',encoding='utf-8')
print('built',len(all_pages),'pages')
