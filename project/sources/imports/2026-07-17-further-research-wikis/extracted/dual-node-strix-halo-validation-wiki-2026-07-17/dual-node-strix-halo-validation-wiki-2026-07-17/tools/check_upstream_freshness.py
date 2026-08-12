#!/usr/bin/env python3
"""Evaluate upstream poll-state freshness and unresolved P0/P1 ledger blockers."""
from __future__ import annotations
import argparse,datetime as dt,json,re,sys
from pathlib import Path
import yaml
UTC=dt.timezone.utc

def parse_time(s):
    if not s: return None
    try: return dt.datetime.fromisoformat(s.replace('Z','+00:00')).astimezone(UTC)
    except ValueError: return None
def duration(s):
    m=re.fullmatch(r'\s*(\d+(?:\.\d+)?)\s*([mhd])\s*',str(s))
    if not m: raise ValueError(f'invalid duration {s}')
    return float(m.group(1))*{'m':60,'h':3600,'d':86400}[m.group(2)]
def main():
    root=Path(__file__).resolve().parents[1]; p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--watch-config',type=Path,default=root/'config/upstream-watch.yaml'); p.add_argument('--policy',type=Path,default=root/'config/freshness-policy.yaml')
    p.add_argument('--state',type=Path,default=root/'raw-data/upstream/watch-state.json'); p.add_argument('--ledger',type=Path,default=root/'raw-data/upstream/events.jsonl')
    p.add_argument('--include-conditional',action='store_true'); p.add_argument('--at',help='ISO UTC evaluation time'); p.add_argument('--output',type=Path)
    a=p.parse_args(); watch=yaml.safe_load(a.watch_config.read_text()); policy=yaml.safe_load(a.policy.read_text())
    budgets={x['class']:x for x in policy['budgets']}; state=json.loads(a.state.read_text()) if a.state.exists() else {'sources':{}}
    at=parse_time(a.at) if a.at else dt.datetime.now(UTC); assert at
    details=[]
    for src in watch['sources']:
        if src.get('enabled_when') and not a.include_conditional: continue
        cls=src['freshness_class']; budget=budgets[cls]; st=state.get('sources',{}).get(src['id'],{}); last=parse_time(st.get('last_success_at'))
        reasons=[]
        if src['type']=='manual_or_web_hash' and not src.get('url'): reasons.append('deployment URL not configured')
        if not last: reasons.append('no successful poll timestamp')
        elif (at-last).total_seconds()>duration(budget['stale_after']): reasons.append(f"last success age {(at-last).total_seconds()/3600:.2f}h exceeds {budget['stale_after']}")
        if st.get('last_error'): reasons.append('last poll error: '+str(st['last_error']))
        details.append({'source_id':src['id'],'freshness_class':cls,'release_blocking':bool(budget['release_blocking']),
                        'last_success_at':st.get('last_success_at'),'fresh':not reasons,'reasons':reasons})
    blockers=[]; event_count=0
    if a.ledger.exists():
        for n,line in enumerate(a.ledger.read_text().splitlines(),1):
            if not line.strip(): continue
            try: ev=json.loads(line); event_count+=1
            except Exception as exc: blockers.append({'event_id':f'parse-error-line-{n}','severity':'P0','status':'new','reason':str(exc)}); continue
            if ev.get('severity') in {'P0','P1'} and ev.get('status') not in {'adopted','not_applicable','duplicate','closed'}:
                blockers.append({'event_id':ev.get('event_id'),'source_id':ev.get('source_id'),'severity':ev.get('severity'),'status':ev.get('status'),'title':ev.get('title')})
    blocking_stale=[d for d in details if d['release_blocking'] and not d['fresh']]
    result={'schema_version':1,'evaluated_at':at.isoformat().replace('+00:00','Z'),'sources_fresh':not blocking_stale,
            'blocking_stale_sources':blocking_stale,'all_sources':details,'ledger_events':event_count,
            'untriaged_p0_p1_blockers':len(blockers),'blockers':blockers,
            'release_ready':not blocking_stale and not blockers,
            'note':'Fresh polling and zero blockers are release prerequisites; they do not prove candidate compatibility.'}
    text=json.dumps(result,indent=2); print(text)
    if a.output: a.output.parent.mkdir(parents=True,exist_ok=True); a.output.write_text(text+'\n')
    return 0 if result['release_ready'] else 1
if __name__=='__main__': sys.exit(main())
