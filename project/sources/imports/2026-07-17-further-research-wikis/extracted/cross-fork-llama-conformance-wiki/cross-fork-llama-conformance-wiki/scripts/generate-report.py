#!/usr/bin/env python3
from __future__ import annotations
import argparse, json
from collections import Counter, defaultdict
from pathlib import Path

def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument("observations", type=Path, nargs="+")
    ap.add_argument("--output", type=Path, required=True)
    ns=ap.parse_args()
    observations=[json.loads(p.read_text(encoding="utf-8")) for p in ns.observations]
    statuses=Counter(o["result"]["status"] for o in observations)
    by_fork=defaultdict(Counter)
    for o in observations:
        by_fork[o["fork"]][o["result"]["status"]]+=1
    report={
        "schema_version":"1.0",
        "observation_count":len(observations),
        "statuses":dict(statuses),
        "by_fork":{k:dict(v) for k,v in sorted(by_fork.items())},
        "failures":[
            {"case_id":o["case_id"],"fork":o["fork"],"status":o["result"]["status"]}
            for o in observations if o["result"]["status"] not in {"pass","skip"}
        ],
    }
    ns.output.parent.mkdir(parents=True,exist_ok=True)
    ns.output.write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
    return 0
if __name__=="__main__":
    raise SystemExit(main())
