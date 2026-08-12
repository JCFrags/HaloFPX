#!/usr/bin/env python3
"""Write the explicit PF-IR-10 non-execution record.

SPDX-License-Identifier: CC0-1.0
"""
from __future__ import annotations
import hashlib, json, platform, sys
from pathlib import Path


def sha(path: Path) -> str: return hashlib.sha256(path.read_bytes()).hexdigest()

def main() -> int:
    root=Path(__file__).resolve().parents[1]
    tools=[
      ("recipes/generate_assets.py","deterministic fixture generation"),
      ("recipes/verify_gguf.py","independent structural self-check"),
      ("recipes/build_manifest.py","fixture/applicability manifest materialization"),
      ("recipes/verify_corpus.py","package self-check and clean reproduction"),
      ("recipes/verify_comparators.py","comparator command-line self-check"),
      ("recipes/write_execution_status.py","non-execution status materialization"),
      ("recipes/package.py","deterministic inventory/archive construction"),
      ("comparators/exact_json.py","canonical JSON comparator self-check"),
      ("comparators/numeric.py","numerical comparator self-check"),
      ("comparators/rejection.py","rejection classifier self-check"),
      ("comparators/sse.py","streaming reference validation"),
      ("comparators/tokenizer_reference.py","tokenizer reference validation"),
      ("comparators/json_schema_subset.py","schema reference validation"),
    ]
    status={
      "schema_version":1,
      "prepared_on":"2026-07-18",
      "candidate_binaries_executed":False,
      "candidate_libraries_loaded":False,
      "candidate_network_services_contacted":False,
      "candidate_helper_scripts_executed":False,
      "claim_labels":["UNEXECUTED-EVIDENCE","QUALIFICATION-REQUIRED"],
      "executed_self_authored_tools":[{"path":rp,"sha256":sha(root/rp),"purpose":purpose} for rp,purpose in tools],
      "unexecuted_candidate_evidence":[
        "adapters/run-candidate.example.sh",
        "adapters/api-capture.example.py",
        "recipes/portable-recurrent-model.md",
        "recipes/portable-mtp-sidecar.md"
      ],
      "python":{"version":sys.version.split()[0],"implementation":platform.python_implementation(),"platform":platform.platform()},
      "qualification_state":{"tiny_llama_model":"not candidate-loaded","rocmfpx_type106_probe":"not candidate-loaded","recurrent_model":"not present","mtp_artifact":"not present"}
    }
    out=root/'qualification/EXECUTION-STATUS.json'; out.write_text(json.dumps(status,ensure_ascii=False,sort_keys=True,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({"candidate_binaries_executed":False,"output":str(out)},sort_keys=True))
    return 0
if __name__=='__main__': raise SystemExit(main())
