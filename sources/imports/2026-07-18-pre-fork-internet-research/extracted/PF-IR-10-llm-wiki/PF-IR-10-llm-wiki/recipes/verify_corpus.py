#!/usr/bin/env python3
"""Self-check PF-IR-10 without invoking any candidate implementation.

SPDX-License-Identifier: CC0-1.0
"""
from __future__ import annotations
import argparse, hashlib, json, os, shutil, subprocess, sys, tempfile
from pathlib import Path
from typing import Any


def sha(data: bytes) -> str: return hashlib.sha256(data).hexdigest()
def git_blob_sha1(data: bytes) -> str:
    return hashlib.sha1(b"blob " + str(len(data)).encode("ascii") + b"\0" + data).hexdigest()
def load_json(path: Path) -> Any: return json.loads(path.read_text(encoding="utf-8"))


def iter_generated(node: Any):
    if isinstance(node, dict):
        if isinstance(node.get("path"), str) and isinstance(node.get("sha256"), str):
            yield node["path"], node["sha256"]
        for v in node.values(): yield from iter_generated(v)
    elif isinstance(node, list):
        for v in node: yield from iter_generated(v)


def check(root: Path, reproduce: bool) -> dict[str, Any]:
    errors: list[dict[str, Any]] = []
    checks: dict[str, Any] = {}
    def fail(code: str, **detail: Any): errors.append({"code":code,**detail})

    generated = load_json(root / "qualification/generated-assets.json")
    generated_paths = dict(iter_generated(generated))
    for rp, expected in generated_paths.items():
        p=root/rp
        if not p.is_file(): fail("generated-missing",path=rp)
        elif sha(p.read_bytes()) != expected: fail("generated-hash",path=rp,expected=expected,actual=sha(p.read_bytes()))
    checks["generated_asset_hashes"]={"count":len(generated_paths),"match":not any(e["code"].startswith("generated-") for e in errors)}

    fixture_lines=(root/"manifests/fixtures.jsonl").read_text(encoding="utf-8").splitlines()
    seen=set(); fixture_count=0
    for n,line in enumerate(fixture_lines,1):
        if not line: continue
        fixture_count+=1; row=json.loads(line); fid=row["fixture_id"]
        if fid in seen: fail("duplicate-fixture-id",fixture_id=fid,line=n)
        seen.add(fid); loc=row["locator"]; p=root/loc["path"]
        if not p.is_file(): fail("fixture-missing",fixture_id=fid,path=loc["path"]); continue
        data=p.read_bytes(); actual=sha(data)
        if actual != row["file_sha256"]: fail("fixture-file-hash",fixture_id=fid,expected=row["file_sha256"],actual=actual)
        if loc["kind"]=="jsonl-line":
            lines=p.read_text(encoding="utf-8").splitlines()
            if not 1<=loc["line"]<=len(lines): fail("fixture-line-range",fixture_id=fid)
            else:
                rec=lines[loc["line"]-1].encode("utf-8")
                if sha(rec)!=row["record_sha256"]: fail("fixture-record-hash",fixture_id=fid)
        elif loc["kind"]=="byte-range":
            a,b=loc["start"],loc["end_exclusive"]
            if not 0<=a<=b<=len(data): fail("fixture-byte-range",fixture_id=fid)
            elif sha(data[a:b])!=row["range_sha256"]: fail("fixture-range-hash",fixture_id=fid)
        elif loc["kind"]!="file": fail("fixture-locator-kind",fixture_id=fid,kind=loc["kind"])
    checks["fixture_manifest"]={"count":fixture_count,"match":not any(e["code"].startswith("fixture-") or e["code"]=="duplicate-fixture-id" for e in errors)}

    app=load_json(root/"manifests/applicability.json"); allowed=set(app["allowed_statuses"])
    for row in app["rows"]:
        for candidate in ("llama.cpp","ROCmFPX","CachyLLama","HaloFPX"):
            if row[candidate] not in allowed: fail("applicability-status",test_id=row["test_id"],candidate=candidate,value=row[candidate])
        if row["HaloFPX"]!="open": fail("halo-not-open",test_id=row["test_id"],value=row["HaloFPX"])
    checks["applicability"]={"rows":len(app["rows"]),"match":not any(e["code"] in {"applicability-status","halo-not-open"} for e in errors)}

    sys.path.insert(0,str((root/"recipes").resolve())); from verify_gguf import classify
    good=["fixtures/gguf/pfir10-tiny-tensor-v3.gguf","fixtures/gguf/pfir10-byte-bpe-chat-v3.gguf","fixtures/gguf/pfir10-tiny-llama-f32-v3.gguf","fixtures/fork-specific/rocmfpx-turbo4-type106-probe.gguf"]
    bad={"bad-magic.gguf":"bad-magic","truncated-header.gguf":"truncated-header","truncated-metadata.gguf":"truncated-metadata","truncated-tensor-data.gguf":"truncated-tensor-data","misaligned-tensor-offset.gguf":"misaligned-tensor-offset","unknown-metadata-type.gguf":"unknown-metadata-type"}
    for rp in good:
        r=classify(root/rp)
        if not r["valid"]: fail("gguf-good-invalid",path=rp,error=r.get("error"))
    for name,code in bad.items():
        r=classify(root/"fixtures/gguf/malformed"/name)
        if r["valid"] or r.get("error",{}).get("code")!=code: fail("gguf-bad-class",path=name,expected=code,actual=r.get("error",{}).get("code"))
    checks["gguf_structural"]={"good":len(good),"malformed":len(bad),"match":not any(e["code"].startswith("gguf-") for e in errors)}

    # JSONL and JSON syntax.
    json_files=0; jsonl_records=0
    for p in sorted((root/"fixtures").rglob("*.json")):
        load_json(p); json_files+=1
    for p in sorted((root/"fixtures").rglob("*.jsonl")):
        for line in p.read_text(encoding="utf-8").splitlines():
            if line: json.loads(line); jsonl_records+=1
    checks["json_syntax"]={"json_files":json_files,"jsonl_records":jsonl_records,"match":True}

    # Tokenizer reference vectors.
    sys.path.insert(0,str((root/"comparators").resolve()))
    from tokenizer_reference import tokenize
    tok_ok=True; tok_cases=0
    for rp in ("fixtures/tokenizer/cases.jsonl","fixtures/tokenizer/special-token-cases.jsonl"):
        for line in (root/rp).read_text(encoding="utf-8").splitlines():
            if not line: continue
            row=json.loads(line); raw=bytes.fromhex(row["input_hex"]); tok_cases+=1
            if "expected_ids_no_special" in row:
                good=tokenize(raw,False,False)==row["expected_ids_no_special"] and raw.hex()==row["expected_roundtrip_hex"]
            else:
                good=(tokenize(raw,False,False)==row["parse_special_false"] and tokenize(raw,True,False)==row["parse_special_true"] and tokenize(raw,True,True)==row["add_bos_true_prefix"])
            if not good: fail("tokenizer-reference",id=row["id"]); tok_ok=False
    checks["tokenizer_reference"]={"cases":tok_cases,"match":tok_ok}

    # JSON-Schema subset and malformed byte boundaries.
    from json_schema_subset import validate
    schema_ok=True; schema_cases=0
    for line in (root/"fixtures/structured/cases.jsonl").read_text(encoding="utf-8").splitlines():
        if not line: continue
        row=json.loads(line); schema_cases+=1; schema=load_json(root/"fixtures/structured"/row["schema"])
        try: value=json.loads(row["json_text"]); actual=not validate(value,schema)
        except json.JSONDecodeError: actual=False
        if actual!=row["valid"]: fail("schema-reference",id=row["id"]); schema_ok=False
    checks["schema_reference"]={"cases":schema_cases,"match":schema_ok}

    boundary_ok=True; boundary_cases=0
    for line in (root/"fixtures/malformed/json-boundaries.jsonl").read_text(encoding="utf-8").splitlines():
        if not line: continue
        row=json.loads(line); boundary_cases+=1
        try: json.loads(bytes.fromhex(row["bytes_hex"]).decode("utf-8")); actual="accept"
        except (UnicodeDecodeError,json.JSONDecodeError): actual="reject-incomplete-json"
        if actual!=row["expected"]: fail("json-boundary-reference",id=row["id"]); boundary_ok=False
    for line in (root/"fixtures/malformed/utf8-boundaries.jsonl").read_text(encoding="utf-8").splitlines():
        if not line: continue
        row=json.loads(line); boundary_cases+=1
        try: bytes.fromhex(row["bytes_hex"]).decode("utf-8"); actual=True
        except UnicodeDecodeError: actual=False
        if actual!=row["valid_utf8"]: fail("utf8-boundary-reference",id=row["id"]); boundary_ok=False
    checks["boundary_reference"]={"cases":boundary_cases,"match":boundary_ok}

    # The bundled grammar is a two-word finite language: AB\n or AC\n.
    grammar_ok=True; grammar_cases=0; language=("AB\n","AC\n")
    for line in (root/"fixtures/structured/grammar-state-cases.jsonl").read_text(encoding="utf-8").splitlines():
        if not line: continue
        row=json.loads(line); grammar_cases+=1; prefix=row["accepted_prefix"]
        possible=[word for word in language if word.startswith(prefix)]
        if row.get("expected")=="reject": good=not possible
        else:
            allowed=sorted({word[len(prefix)] for word in possible if len(word)>len(prefix)})
            good=allowed==sorted(row["allowed_next_utf8"])
        if not good: fail("grammar-state-reference",id=row["id"]); grammar_ok=False
    checks["grammar_state_reference"]={"cases":grammar_cases,"match":grammar_ok}

    # Streaming reference.
    sys.path.insert(0,str((root/"comparators").resolve())); from sse import parse_chunks
    stream_root=root/"fixtures/api/streaming"; sse_ok=True; sse_cases=0
    for line in (stream_root/"expected.jsonl").read_text(encoding="utf-8").splitlines():
        if not line: continue
        row=json.loads(line); sse_cases+=1
        ranges=load_json(stream_root/row["chunks"])["chunks"] if row.get("chunks") else None
        events,stats=parse_chunks((stream_root/row["file"]).read_bytes(),ranges)
        actual={"events":events,**stats}; expected={k:row[k] for k in ("events","done","malformed_records_skipped","post_done_ignored")}
        if actual!=expected: fail("sse-reference",id=row["id"]); sse_ok=False
    checks["sse_reference"]={"cases":sse_cases,"match":sse_ok}

    # Literal source-range and license evidence integrity.
    source_ranges=load_json(root/"evidence/raw/source-ranges/index.json")
    source_range_ok=True
    candidate_commits={c["candidate_id"]:c.get("commit") for c in load_json(root/"manifests/candidates.json")["candidates"]}
    for row in source_ranges["records"]:
        p=root/row["local_path"]
        if not p.is_file():
            fail("source-range-missing",path=row["local_path"]); source_range_ok=False; continue
        data=p.read_bytes()
        line_count=len(data.decode("utf-8").splitlines())
        expected_lines=row["requested_line_end"]-row["requested_line_start"]+1
        if sha(data)!=row["local_sha256"] or len(data)!=row["local_length"]:
            fail("source-range-hash",path=row["local_path"]); source_range_ok=False
        if line_count!=expected_lines:
            fail("source-range-lines",path=row["local_path"],expected=expected_lines,actual=line_count); source_range_ok=False
        if candidate_commits.get(row["candidate_id"])!=row["commit"]:
            fail("source-range-commit",path=row["local_path"]); source_range_ok=False
        if len(row["source_git_blob_sha1"])!=40 or any(c not in "0123456789abcdef" for c in row["source_git_blob_sha1"]):
            fail("source-range-blob-id",path=row["local_path"]); source_range_ok=False
    checks["source_range_evidence"]={"records":len(source_ranges["records"]),"match":source_range_ok}

    license_hashes=load_json(root/"evidence/licenses/hashes.json")
    license_ok=True
    for row in license_hashes["records"]:
        p=root/row["local_path"]
        if not p.is_file():
            fail("license-evidence-missing",path=row["local_path"]); license_ok=False; continue
        data=p.read_bytes()
        if sha(data)!=row["local_sha256"] or len(data)!=row["local_length"] or git_blob_sha1(data)!=row["source_git_blob_sha1"]:
            fail("license-evidence-hash",path=row["local_path"]); license_ok=False
        if candidate_commits.get(row["candidate_id"])!=row["commit"]:
            fail("license-evidence-commit",path=row["local_path"]); license_ok=False
    checks["license_evidence"]={"records":len(license_hashes["records"]),"match":license_ok}

    deps=load_json(root/"manifests/dependencies.lock.json")
    dep_ok=(deps.get("artifact_build",{}).get("third_party_packages")==[] and deps.get("artifact_build",{}).get("external_executables_required")==[])
    if not dep_ok: fail("dependency-lock")
    checks["dependency_lock"]={"third_party_packages":len(deps.get("artifact_build",{}).get("third_party_packages",[])),"match":dep_ok}

    # Excluded hashes must not occur in bundled files.
    excluded=load_json(root/"manifests/excluded-assets.json")
    excluded_hashes={a["sha256"] for a in excluded["assets"] if a.get("sha256")}
    hits=[]
    for p in root.rglob("*"):
        if p.is_file() and p.name!="MANIFEST.sha256":
            h=sha(p.read_bytes())
            if h in excluded_hashes: hits.append(p.relative_to(root).as_posix())
    if hits: fail("excluded-hash-present",paths=hits)
    checks["excluded_payload_absent"]={"match":not hits}

    status=load_json(root/"qualification/EXECUTION-STATUS.json")
    if status.get("candidate_binaries_executed") is not False: fail("candidate-execution-status")
    for p in (root/"adapters").iterdir():
        if p.is_file() and os.access(p,os.X_OK): fail("adapter-executable",path=p.relative_to(root).as_posix())
    checks["execution_boundary"]={"match":not any(e["code"] in {"candidate-execution-status","adapter-executable"} for e in errors)}

    if reproduce:
        with tempfile.TemporaryDirectory(prefix="pfir10-repro-") as td:
            tmp=Path(td)
            subprocess.run([sys.executable,str(root/"recipes/generate_assets.py"),"--root",str(tmp)],check=True)
            mismatches=[]
            for rp,expected in generated_paths.items():
                p=tmp/rp
                if not p.is_file() or sha(p.read_bytes())!=expected: mismatches.append(rp)
            if mismatches: fail("reproduction-mismatch",paths=mismatches)
            checks["clean_reproduction"]={"count":len(generated_paths),"match":not mismatches}

    return {"schema_version":1,"candidate_binaries_executed":False,"match":not errors,"checks":checks,"errors":errors}


def main() -> int:
    ap=argparse.ArgumentParser(); ap.add_argument("--root",type=Path,default=Path(__file__).resolve().parents[1]); ap.add_argument("--reproduce",action="store_true"); ap.add_argument("--json-out",type=Path)
    ns=ap.parse_args(); result=check(ns.root.resolve(),ns.reproduce); text=json.dumps(result,ensure_ascii=False,sort_keys=True,indent=2)+"\n"
    out=ns.json_out or ns.root/"qualification/SELF-CHECK.json"; out.write_text(text,encoding="utf-8",newline="\n")
    print(json.dumps({"match":result["match"],"errors":len(result["errors"]),"output":str(out)},sort_keys=True))
    return 0 if result["match"] else 1
if __name__=="__main__": raise SystemExit(main())
