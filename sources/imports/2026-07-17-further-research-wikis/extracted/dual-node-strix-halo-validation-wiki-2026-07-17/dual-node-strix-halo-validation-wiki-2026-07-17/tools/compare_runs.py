#!/usr/bin/env python3
"""Paired candidate/baseline ratio with bootstrap CI and warn/fail classification.

Input CSV columns: pair_id,baseline,candidate. Values must be positive and matched.
"""
from __future__ import annotations
import argparse,csv,json,math,random,statistics,sys
from pathlib import Path

def pct(xs,q):
    xs=sorted(xs); p=(len(xs)-1)*q; lo=math.floor(p); hi=math.ceil(p)
    return xs[lo] if lo==hi else xs[lo]+(xs[hi]-xs[lo])*(p-lo)
def main():
    p=argparse.ArgumentParser(description=__doc__); p.add_argument('csv',type=Path); p.add_argument('--metric',required=True)
    p.add_argument('--direction',choices=['higher_better','lower_better'],required=True); p.add_argument('--warn',type=float,required=True,help='relative degradation, e.g. 0.03')
    p.add_argument('--fail',type=float,required=True); p.add_argument('--bootstrap',type=int,default=10000); p.add_argument('--output',type=Path)
    a=p.parse_args(); rows=list(csv.DictReader(a.csv.open())); ratios=[]; seen=set()
    for n,r in enumerate(rows,2):
        pid=r['pair_id']; b=float(r['baseline']); c=float(r['candidate'])
        if pid in seen or b<=0 or c<0: raise SystemExit(f'invalid pair at line {n}')
        seen.add(pid); ratios.append(c/b)
    if len(ratios)<2: raise SystemExit('need at least two pairs')
    point=statistics.median(ratios); rng=random.Random(20260717); boots=[]
    for _ in range(a.bootstrap): boots.append(statistics.median(rng.choices(ratios,k=len(ratios))))
    ci=[pct(boots,.025),pct(boots,.975)]; warn_boundary=1-a.warn if a.direction=='higher_better' else 1+a.warn
    fail_boundary=1-a.fail if a.direction=='higher_better' else 1+a.fail
    confirmed_fail=ci[1] < fail_boundary if a.direction=='higher_better' else ci[0] > fail_boundary
    point_warn=point < warn_boundary if a.direction=='higher_better' else point > warn_boundary
    point_fail=point < fail_boundary if a.direction=='higher_better' else point > fail_boundary
    status='FAIL' if confirmed_fail else ('WARN_RETEST' if point_fail or point_warn else 'PASS')
    out={'metric':a.metric,'pairs':len(ratios),'direction':a.direction,'median_candidate_over_baseline':point,'bootstrap_ci95':ci,
         'warn_boundary_ratio':warn_boundary,'fail_boundary_ratio':fail_boundary,'status':status,
         'rule':'FAIL only when the paired bootstrap CI lies beyond the fail boundary; point breach otherwise WARN_RETEST.'}
    text=json.dumps(out,indent=2); print(text); a.output and a.output.write_text(text+'\n'); return 1 if status!='PASS' else 0
if __name__=='__main__': sys.exit(main())
