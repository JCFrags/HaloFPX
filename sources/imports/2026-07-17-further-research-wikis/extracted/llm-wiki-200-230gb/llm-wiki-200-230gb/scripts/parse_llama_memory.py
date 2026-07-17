#!/usr/bin/env python3
"""Best-effort extraction of llama.cpp memory lines; retain the raw log as authority."""
from __future__ import annotations
import argparse,json,re
from pathlib import Path
PATTERNS={
 'model_buffer':re.compile(r'(?i)(?:model buffer|model size|model.*buffer).*?([0-9.]+)\s*(MiB|GiB)'),
 'kv_cache':re.compile(r'(?i)(?:KV|K[Vv]).*?(?:buffer|cache).*?([0-9.]+)\s*(MiB|GiB)'),
 'compute_buffer':re.compile(r'(?i)(?:compute|graph).*?buffer.*?([0-9.]+)\s*(MiB|GiB)'),
}
def gib(v,u):return float(v)/(1024 if u.lower()=='mib' else 1)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('log');ap.add_argument('--output',default='measured-memory.json');a=ap.parse_args();text=Path(a.log).read_text(errors='replace')
 out={'source_log':str(Path(a.log).resolve()),'matches':[],'totals_gib':{}}
 totals={k:0.0 for k in PATTERNS}
 for line in text.splitlines():
  for k,p in PATTERNS.items():
   m=p.search(line)
   if m:
    v=gib(m.group(1),m.group(2));totals[k]+=v;out['matches'].append({'kind':k,'gib':v,'line':line})
 out['totals_gib']=totals;Path(a.output).write_text(json.dumps(out,indent=2));print(json.dumps(totals,indent=2))
if __name__=='__main__':main()
