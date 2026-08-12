#!/usr/bin/env python3
"""Poll configured upstream sources and append normalized events to a JSONL ledger.

No source change is adopted automatically. The output is discovery evidence; P0/P1
items require human triage, canaries, and a signed disposition.
"""
from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import os
import sys
import urllib.error
import urllib.parse
import urllib.request
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Any

import yaml

UTC=dt.timezone.utc

def now(): return dt.datetime.now(UTC)
def iso(t: dt.datetime|None=None): return (t or now()).astimezone(UTC).isoformat().replace('+00:00','Z')
def parse_time(value: str|None) -> dt.datetime|None:
    if not value: return None
    try: return dt.datetime.fromisoformat(value.replace('Z','+00:00')).astimezone(UTC)
    except ValueError: return None

def sha_bytes(data: bytes): return hashlib.sha256(data).hexdigest()
def event_id(source_id: str, immutable_id: str): return 'evt-'+hashlib.sha256(f'{source_id}\0{immutable_id}'.encode()).hexdigest()[:24]

def load_state(path: Path) -> dict:
    if not path.exists(): return {'schema_version':1,'sources':{}}
    return json.loads(path.read_text(encoding='utf-8'))
def save_state(path: Path,state: dict):
    path.parent.mkdir(parents=True,exist_ok=True); tmp=path.with_suffix(path.suffix+'.tmp'); tmp.write_text(json.dumps(state,indent=2)+'\n'); tmp.replace(path)
def load_seen_ledger(path: Path) -> set[str]:
    ids=set()
    if path.exists():
        for line in path.read_text(encoding='utf-8').splitlines():
            if line.strip():
                try: ids.add(json.loads(line)['event_id'])
                except Exception: pass
    return ids

def request(url: str, headers: dict[str,str], etag: str|None=None) -> tuple[int,bytes,dict[str,str]]:
    hdr=dict(headers)
    if etag: hdr['If-None-Match']=etag
    req=urllib.request.Request(url,headers=hdr)
    try:
        with urllib.request.urlopen(req,timeout=45) as resp:
            return int(getattr(resp,'status',200)),resp.read(),{k.lower():v for k,v in resp.headers.items()}
    except urllib.error.HTTPError as exc:
        if exc.code==304: return 304,b'',{k.lower():v for k,v in exc.headers.items()}
        body=exc.read(2048).decode('utf-8','replace')
        raise RuntimeError(f'HTTP {exc.code} {url}: {body}') from exc

def classify(source: dict,title: str,body: str,classifiers: dict) -> str:
    text=(title+' '+body).lower()
    for sev in ['P0','P1','P2']:
        for term in classifiers.get(sev,[]):
            if term.lower() in text: return sev
    if source.get('freshness_class')=='security_critical': return 'P1'
    if source.get('freshness_class')=='community_signal': return 'P3'
    return 'P2'

def normalize(source: dict, immutable_id: str, title: str, url: str|None, published: str|None, body: str, source_type: str, classifiers: dict) -> dict:
    published_dt=parse_time(published) or now()
    severity=classify(source,title,body,classifiers)
    keywords=[t for t in classifiers.get(severity,[]) if t.lower() in (title+' '+body).lower()]
    return {'schema_version':1,'event_id':event_id(source['id'],immutable_id),'source_id':source['id'],'source_type':source_type,
            'immutable_id':str(immutable_id),'title':title[:1000],'url':url,'published_at':iso(published_dt),'discovered_at':iso(),
            'content_sha256':hashlib.sha256((title+'\n'+body).encode()).hexdigest(),'severity':severity,
            'affected_layers':source.get('labels',[]),'keywords':keywords,'status':'new','owner':None,'decision':None,
            'canaries':source.get('canaries',[]),'evidence_paths':[]}

def github_headers(defaults: dict) -> dict[str,str]:
    h={'Accept':'application/vnd.github+json','User-Agent':defaults.get('user_agent','validation-watch/1.0'),'X-GitHub-Api-Version':'2022-11-28'}
    token=os.environ.get('GITHUB_TOKEN')
    if token: h['Authorization']=f'Bearer {token}'
    return h

def poll_github(source: dict,state: dict,defaults: dict,classifiers: dict,lookback_h: float) -> tuple[list[dict],dict]:
    api=defaults.get('github_api','https://api.github.com'); typ=source['type']; params={}
    if typ=='github_releases':
        url=f"{api}/repos/{source['repo']}/releases?per_page=30"
    elif typ=='github_commits':
        since=parse_time(state.get('last_success_at')) or (now()-dt.timedelta(hours=lookback_h))
        # overlap by two hours so delayed indexing does not create a blind spot.
        params={'per_page':100,'since':iso(since-dt.timedelta(hours=2))}
        if source.get('path'): params['path']=source['path']
        url=f"{api}/repos/{source['repo']}/commits?"+urllib.parse.urlencode(params)
    elif typ=='github_issue_search':
        params={'q':source['query'],'sort':'updated','order':'desc','per_page':100}
        url=f"{api}/search/issues?"+urllib.parse.urlencode(params)
    else: raise ValueError(typ)
    status,data,hdr=request(url,github_headers(defaults),state.get('etag'))
    new_state=dict(state); new_state['last_success_at']=iso(); new_state['last_url']=url
    if hdr.get('etag'): new_state['etag']=hdr['etag']
    if hdr.get('x-ratelimit-remaining'): new_state['github_rate_remaining']=hdr['x-ratelimit-remaining']
    if status==304: return [],new_state
    payload=json.loads(data)
    items=payload.get('items',[]) if isinstance(payload,dict) and typ=='github_issue_search' else payload
    events=[]
    for item in items:
        if typ=='github_releases':
            immutable=f"release:{item.get('id')}:{item.get('tag_name')}"; title=item.get('name') or item.get('tag_name') or immutable
            pub=item.get('published_at') or item.get('created_at'); url=item.get('html_url'); body=item.get('body') or ''
        elif typ=='github_commits':
            immutable=item.get('sha'); commit=item.get('commit') or {}; title=(commit.get('message') or immutable).splitlines()[0]
            pub=(commit.get('committer') or {}).get('date'); url=item.get('html_url'); body=commit.get('message') or ''
        else:
            immutable=f"issue:{item.get('id')}:{item.get('updated_at')}"; title=item.get('title') or immutable
            pub=item.get('updated_at') or item.get('created_at'); url=item.get('html_url'); body=item.get('body') or ''
        combined=(str(title)+' '+str(body)).lower()
        terms=[str(x).lower() for x in source.get('keywords',[])]
        if terms and not any(term in combined for term in terms):
            continue
        events.append(normalize(source,str(immutable),str(title),url,pub,str(body),typ,classifiers))
    return events,new_state

def poll_web_hash(source: dict,state: dict,defaults: dict,classifiers: dict) -> tuple[list[dict],dict]:
    url=source['url']; headers={'User-Agent':defaults.get('user_agent','validation-watch/1.0'),'Accept':'text/html,application/xml;q=0.9,*/*;q=0.8'}
    status,data,hdr=request(url,headers,state.get('etag')); new=dict(state); new['last_success_at']=iso(); new['last_url']=url
    if hdr.get('etag'): new['etag']=hdr['etag']
    if hdr.get('last-modified'): new['last_modified']=hdr['last-modified']
    if status==304: return [],new
    digest=sha_bytes(data); old=state.get('content_sha256'); new['content_sha256']=digest
    if old==digest: return [],new
    title=('Initial snapshot: ' if old is None else 'Content changed: ')+source['id']
    ev=normalize(source,f'webhash:{digest}',title,url,hdr.get('last-modified'),f'previous={old} current={digest}','web_hash',classifiers)
    return [ev],new

def text_of(node: ET.Element, local: str) -> str|None:
    for child in node.iter():
        if child.tag.rsplit('}',1)[-1]==local and child.text: return child.text.strip()
    return None

def poll_atom(source: dict,state: dict,defaults: dict,classifiers: dict,lookback_h: float) -> tuple[list[dict],dict]:
    status,data,hdr=request(source['url'],{'User-Agent':defaults.get('user_agent','validation-watch/1.0'),'Accept':'application/atom+xml'},state.get('etag'))
    new=dict(state); new['last_success_at']=iso(); new['last_url']=source['url'];
    if hdr.get('etag'): new['etag']=hdr['etag']
    if status==304: return [],new
    root=ET.fromstring(data); cutoff=(parse_time(state.get('last_success_at')) or now()-dt.timedelta(hours=lookback_h))-dt.timedelta(hours=2)
    events=[]; terms=[x.lower() for x in source.get('keywords',[])]
    for entry in root.iter():
        if entry.tag.rsplit('}',1)[-1]!='entry': continue
        title=text_of(entry,'title') or 'Untitled atom entry'; ident=text_of(entry,'id') or hashlib.sha256(ET.tostring(entry)).hexdigest()
        updated=text_of(entry,'updated') or text_of(entry,'published'); t=parse_time(updated)
        if t and t<cutoff: continue
        body=text_of(entry,'summary') or text_of(entry,'content') or ''
        if terms and not any(x in (title+' '+body).lower() for x in terms): continue
        link=None
        for child in entry.iter():
            if child.tag.rsplit('}',1)[-1]=='link' and child.attrib.get('href'):
                link=child.attrib['href']; break
        events.append(normalize(source,ident,title,link,updated,body,'git_atom',classifiers))
    return events,new

def main() -> int:
    p=argparse.ArgumentParser(description=__doc__)
    root=Path(__file__).resolve().parents[1]
    p.add_argument('--config',type=Path,default=root/'config/upstream-watch.yaml'); p.add_argument('--state',type=Path)
    p.add_argument('--ledger',type=Path); p.add_argument('--report',type=Path); p.add_argument('--lookback-hours',type=float,default=168)
    p.add_argument('--dry-run',action='store_true'); p.add_argument('--include-conditional',action='store_true'); p.add_argument('--strict',action='store_true')
    a=p.parse_args(); cfg=yaml.safe_load(a.config.read_text()); defaults=cfg.get('defaults',{})
    state_path=a.state or root/defaults.get('state_file','raw-data/upstream/watch-state.json')
    ledger_path=a.ledger or root/defaults.get('ledger_file','raw-data/upstream/events.jsonl')
    if a.dry_run:
        plans=[]
        for s in cfg['sources']:
            status='conditional-skip' if s.get('enabled_when') and not a.include_conditional else ('configuration-required' if not s.get('url') and s['type'] in {'manual_or_web_hash','web_hash','git_atom'} else 'poll')
            plans.append({'id':s['id'],'type':s['type'],'status':status,'freshness_class':s['freshness_class'],'target':s.get('repo') or s.get('url') or s.get('query')})
        print(json.dumps({'dry_run':True,'sources':plans},indent=2)); return 0

    state=load_state(state_path); seen=load_seen_ledger(ledger_path); events=[]; errors=[]; skipped=[]
    for source in cfg['sources']:
        sid=source['id']; sstate=state.setdefault('sources',{}).get(sid,{})
        if source.get('enabled_when') and not a.include_conditional:
            skipped.append({'source_id':sid,'reason':'conditional source disabled'}); continue
        if source['type']=='manual_or_web_hash' and not source.get('url'):
            msg='deployment must configure exact OEM URL'; skipped.append({'source_id':sid,'reason':msg})
            if a.strict: errors.append({'source_id':sid,'error':msg})
            continue
        try:
            if source['type'].startswith('github_'):
                found,new_state=poll_github(source,sstate,defaults,cfg.get('classifiers',{}),a.lookback_hours)
            elif source['type'] in {'web_hash','manual_or_web_hash'}:
                found,new_state=poll_web_hash(source,sstate,defaults,cfg.get('classifiers',{}))
            elif source['type']=='git_atom':
                found,new_state=poll_atom(source,sstate,defaults,cfg.get('classifiers',{}),a.lookback_hours)
            else: raise ValueError(f"unsupported type {source['type']}")
            new_state.pop('last_error',None); state['sources'][sid]=new_state
            for ev in found:
                if ev['event_id'] not in seen:
                    events.append(ev); seen.add(ev['event_id'])
        except Exception as exc:
            state['sources'].setdefault(sid,sstate); state['sources'][sid]['last_error']=str(exc); state['sources'][sid]['last_attempt_at']=iso()
            errors.append({'source_id':sid,'error':str(exc)})
    state['last_run_at']=iso(); save_state(state_path,state)
    if events:
        ledger_path.parent.mkdir(parents=True,exist_ok=True)
        with ledger_path.open('a',encoding='utf-8') as fh:
            for ev in sorted(events,key=lambda x:(x['severity'],x['published_at'],x['event_id'])): fh.write(json.dumps(ev,separators=(',',':'))+'\n')
    report={'schema_version':1,'run_at':iso(),'config':str(a.config),'state':str(state_path),'ledger':str(ledger_path),
            'new_events':len(events),'by_severity':{s:sum(e['severity']==s for e in events) for s in ['P0','P1','P2','P3']},
            'errors':errors,'skipped':skipped,'release_freshness_note':'A successful polling run is discovery freshness, not automatic compatibility validation.'}
    text=json.dumps(report,indent=2); print(text)
    if a.report: a.report.parent.mkdir(parents=True,exist_ok=True); a.report.write_text(text+'\n')
    return 1 if errors else 0
if __name__=='__main__': sys.exit(main())
