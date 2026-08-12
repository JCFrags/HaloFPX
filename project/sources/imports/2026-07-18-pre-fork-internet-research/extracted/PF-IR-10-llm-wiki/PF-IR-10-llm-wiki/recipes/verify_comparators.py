#!/usr/bin/env python3
"""Exercise PF-IR-10 comparator command-line contracts on self-authored data.

SPDX-License-Identifier: CC0-1.0
"""
from __future__ import annotations
import json, subprocess, sys, tempfile
from pathlib import Path


def run(cmd: list[str]) -> dict:
    p=subprocess.run(cmd,text=True,capture_output=True)
    return {"command":cmd,"returncode":p.returncode,"stdout":p.stdout,"stderr":p.stderr,"pass":p.returncode==0}


def main() -> int:
    root=Path(__file__).resolve().parents[1]; py=sys.executable; results=[]
    with tempfile.TemporaryDirectory(prefix='pfir10-comparator-') as td:
        t=Path(td)
        (t/'e.json').write_text('{"id":"E","value":{"a":1,"b":[2,3]}}\n')
        (t/'a.json').write_text('{"id":"A","value":{"b":[2,3],"a":1}}\n')
        results.append(run([py,str(root/'comparators/exact_json.py'),str(t/'e.json'),str(t/'a.json'),'--ignore','/id']))
        (t/'n1.json').write_text('[1.0,2.0,3.0]\n'); (t/'n2.json').write_text('[1.000001,2.0,2.999999]\n')
        results.append(run([py,str(root/'comparators/numeric.py'),str(t/'n1.json'),str(t/'n2.json'),'--atol','1e-5']))
        (t/'reject.json').write_text(json.dumps({'exit_code':1,'stderr':'failed to load GGUF: bad magic'})+'\n')
        results.append(run([py,str(root/'comparators/rejection.py'),str(t/'reject.json'),'--class','reject-load']))
        rows=[json.loads(x) for x in (root/'fixtures/api/streaming/expected.jsonl').read_text().splitlines() if x]
        row=next(x for x in rows if x['id']=='canonical'); (t/'sse-expected.json').write_text(json.dumps(row,ensure_ascii=False)+'\n')
        results.append(run([py,str(root/'comparators/sse.py'),str(root/'fixtures/api/streaming/canonical.sse'),'--chunks',str(root/'fixtures/api/streaming/canonical.chunks.json'),'--expected-row',str(t/'sse-expected.json')]))
        for record in results:
            record["command"] = [arg.replace(str(t), "<TMP>").replace(str(root), "<ROOT>").replace(py, "python3") for arg in record["command"]]
    result={"schema_version":1,"candidate_binaries_executed":False,"match":all(r['pass'] for r in results),"results":results}
    out=root/'qualification/COMPARATOR-SELF-CHECK.json'; out.write_text(json.dumps(result,ensure_ascii=False,sort_keys=True,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'match':result['match'],'output':str(out)},sort_keys=True))
    return 0 if result['match'] else 1
if __name__=='__main__': raise SystemExit(main())
