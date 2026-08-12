#!/usr/bin/env python3
"""Safe source-only sentinels for PF-IR-01. No code execution from target trees."""
from pathlib import Path
import argparse, json, re, subprocess, sys

SENTINELS = [
    ("rpc_cpu_pointer_client", "ggml/src/ggml-rpc/ggml-rpc.cpp", [r"\bdata\s*=\s*0\b"], "corrected RPC client serialization"),
    ("rpc_cpu_pointer_server", "ggml/src/ggml-rpc/ggml-rpc.cpp", [r"buffer\s*==\s*nullptr", r"data\s*!=\s*nullptr"], "corrected RPC server validation"),
    ("rpc_zero_block_type", "ggml/src/ggml-rpc/ggml-rpc.cpp", [r"ggml_blck_size\s*\(", r"==\s*0"], "zero-block-size tensor rejection"),
    ("server_n_discard", "tools/server/server-task.cpp", [r"std::max\s*\(\s*0\s*,\s*params\.n_discard\s*\)"], "negative n_discard clamp"),
    ("vocab_checked_access", "src/llama-vocab.cpp", [r"id_to_token\.at\s*\("], "checked special-token access"),
    ("vocab_int32_guard", "src/llama-vocab.cpp", [r"INT32_MAX"], "signed-size guard"),
    ("gguf_overflow_checks", "ggml/src/gguf.cpp", [r"SIZE_MAX|std::numeric_limits\s*<\s*size_t\s*>", r"ggml_nbytes|mem_size"], "GGUF size guards"),
]

def git_head(root):
    try:
        return subprocess.check_output(["git","-C",str(root),"rev-parse","HEAD"], text=True, stderr=subprocess.DEVNULL).strip()
    except Exception:
        return None

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("repo", type=Path)
    ap.add_argument("--expected-commit")
    ap.add_argument("--standard-release", action="store_true", help="also reject explicit GGML_RPC=ON in release workflow")
    ap.add_argument("--json", action="store_true")
    args=ap.parse_args()
    root=args.repo.resolve()
    results=[]
    head=git_head(root)
    if args.expected_commit:
        results.append({"id":"exact_commit","pass":head==args.expected_commit,"detail":f"HEAD={head!r} expected={args.expected_commit!r}"})
    for sid,rel,patterns,desc in SENTINELS:
        p=root/rel
        if not p.exists():
            results.append({"id":sid,"pass":False,"detail":f"missing {rel}"})
            continue
        text=p.read_text(errors="replace")
        ok=all(re.search(x,text,re.M|re.S) for x in patterns)
        results.append({"id":sid,"pass":bool(ok),"detail":desc})
    if args.standard_release:
        wf=root/".github/workflows/release.yml"
        text=wf.read_text(errors="replace") if wf.exists() else ""
        bad=bool(re.search(r"-DGGML_RPC\s*=\s*ON",text))
        results.append({"id":"standard_release_rpc_denied","pass":not bad,"detail":"release workflow must not enable GGML_RPC"})
    passed=all(x["pass"] for x in results)
    if args.json:
        print(json.dumps({"pass":passed,"results":results},indent=2))
    else:
        for x in results:
            print(("PASS" if x["pass"] else "FAIL"), x["id"], "-", x["detail"])
    return 0 if passed else 1
if __name__=="__main__":
    raise SystemExit(main())
