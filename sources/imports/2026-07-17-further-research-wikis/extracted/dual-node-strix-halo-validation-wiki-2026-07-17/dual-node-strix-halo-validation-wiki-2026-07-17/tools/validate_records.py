#!/usr/bin/env python3
"""Validate a JSON document or JSONL stream against a JSON Schema 2020-12 file."""
from __future__ import annotations
import argparse, json, sys
from pathlib import Path
from jsonschema import Draft202012Validator


def main() -> int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('schema',type=Path); p.add_argument('data',type=Path); p.add_argument('--jsonl',action='store_true')
    a=p.parse_args(); schema=json.loads(a.schema.read_text()); Draft202012Validator.check_schema(schema); v=Draft202012Validator(schema)
    records=[]
    if a.jsonl:
        for n,line in enumerate(a.data.read_text().splitlines(),1):
            if line.strip(): records.append((n,json.loads(line)))
    else: records=[(1,json.loads(a.data.read_text()))]
    count=0
    for n,item in records:
        for err in sorted(v.iter_errors(item),key=lambda e:list(e.path)):
            count+=1; print(f'{a.data}:{n} {err.json_path}: {err.message}',file=sys.stderr)
    print(json.dumps({'records':len(records),'errors':count,'status':'PASS' if count==0 else 'FAIL'}))
    return 1 if count else 0
if __name__=='__main__': sys.exit(main())
