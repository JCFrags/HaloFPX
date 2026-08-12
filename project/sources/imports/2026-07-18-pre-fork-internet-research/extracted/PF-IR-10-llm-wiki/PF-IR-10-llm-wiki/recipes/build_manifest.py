#!/usr/bin/env python3
"""Build PF-IR-10 fixture, candidate, comparator, and applicability manifests.

SPDX-License-Identifier: CC0-1.0
"""
from __future__ import annotations
import csv, hashlib, json
from pathlib import Path
from typing import Any

ACCESS_DATE = "2026-07-18"
CANDIDATES = ["llama.cpp", "ROCmFPX", "CachyLLama", "HaloFPX"]


def sha(data: bytes) -> str: return hashlib.sha256(data).hexdigest()
def rel(root: Path, p: Path) -> str: return p.relative_to(root).as_posix()
def write_json(path: Path, obj: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(obj, ensure_ascii=False, sort_keys=True, indent=2) + "\n", encoding="utf-8", newline="\n")
def write_jsonl(path: Path, rows: list[dict[str, Any]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("".join(json.dumps(r, ensure_ascii=False, sort_keys=True, separators=(",", ":")) + "\n" for r in rows), encoding="utf-8", newline="\n")


def profile_for(path: str, row: dict[str, Any] | None = None) -> str:
    if path.startswith("fixtures/tokenizer/"): return "exact-token-ids"
    if path == "fixtures/chat/cases.jsonl": return "exact-template-utf8"
    if path == "fixtures/chat/malformed-cases.jsonl": return "rejection-class"
    if path.endswith(".jinja"): return "exact-bytes"
    if path == "fixtures/structured/cases.jsonl": return "schema-validity"
    if path == "fixtures/structured/grammar-state-cases.jsonl": return "exact-allowed-token-set"
    if path.endswith(".gbnf") or path.endswith(".schema.json"): return "exact-bytes"
    if path == "fixtures/api/streaming/expected.jsonl": return "sse-normalized"
    if "/streaming/" in path: return "exact-bytes"
    if "/api/expected/" in path: return "canonical-json"
    if "/api/requests/malformed" in path: return "rejection-class"
    if "/api/requests/" in path: return "request-contract"
    if path == "fixtures/malformed/json-boundaries.jsonl": return "json-boundary-validity"
    if path == "fixtures/malformed/utf8-boundaries.jsonl": return "utf8-validity"
    if path.startswith("fixtures/gguf/malformed/") and path.endswith(".gguf"): return "rejection-class"
    if path.endswith(".gguf"): return "gguf-structural"
    if path == "fixtures/sampler/probability-vectors.jsonl": return "numeric-probabilities"
    if path == "fixtures/sampler/rng-properties.jsonl": return "rng-metamorphic"
    if path == "fixtures/sampler/state-properties.jsonl": return "state-metamorphic"
    if path.startswith("fixtures/state/"): return "state-semantic"
    if path.startswith("fixtures/fork-specific/"): return "fork-specific"
    return "exact-bytes"


def category_for(path: str) -> str:
    parts = Path(path).parts
    return parts[1] if len(parts) > 1 else "misc"


def add_file_fixture(root: Path, rows: list[dict[str, Any]], path: Path, fixture_id: str | None = None, *, labels: list[str] | None = None, notes: str | None = None) -> None:
    rp = rel(root, path); data = path.read_bytes()
    labels = labels or ["SELF-GENERATED"]
    row = {
        "fixture_id": fixture_id or rp.replace("/", ":"),
        "category": category_for(rp),
        "locator": {"kind": "file", "path": rp},
        "file_sha256": sha(data),
        "file_length": len(data),
        "license": "CC0-1.0",
        "license_evidence": "evidence/licenses/CC0-1.0.txt",
        "claim_labels": labels,
        "oracle_profile": profile_for(rp),
        "deterministic_recipe": "recipes/generate_assets.py",
    }
    if notes: row["notes"] = notes
    rows.append(row)


def build_fixture_rows(root: Path) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for path in sorted((root / "fixtures").rglob("*")):
        if not path.is_file(): continue
        rp = rel(root, path)
        if path.suffix == ".jsonl":
            file_data = path.read_bytes(); file_hash = sha(file_data)
            for line_no, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
                if not line: continue
                record = json.loads(line); record_bytes = line.encode("utf-8")
                rid = str(record.get("id", f"line-{line_no:04d}"))
                rows.append({
                    "fixture_id": f"{rp}:{rid}",
                    "category": category_for(rp),
                    "locator": {"kind": "jsonl-line", "path": rp, "line": line_no},
                    "file_sha256": file_hash,
                    "record_sha256": sha(record_bytes),
                    "license": "CC0-1.0",
                    "license_evidence": "evidence/licenses/CC0-1.0.txt",
                    "claim_labels": ["SELF-GENERATED"] + (["QUALIFICATION-REQUIRED"] if rp.startswith("fixtures/state/") or rp.startswith("fixtures/fork-specific/") else []),
                    "oracle_profile": profile_for(rp, record),
                    "deterministic_recipe": "recipes/generate_assets.py",
                })
        else:
            labels = ["SELF-GENERATED"]
            notes = None
            if rp in {"fixtures/gguf/pfir10-tiny-llama-f32-v3.gguf", "fixtures/fork-specific/rocmfpx-turbo4-type106-probe.gguf"}:
                labels.append("QUALIFICATION-REQUIRED")
            if rp == "fixtures/fork-specific/rocmfpx-turbo4-type106-probe.gguf":
                notes = "Type-ID dispatch/header probe; packed data is not claimed as numerically valid TurboQuant."
            add_file_fixture(root, rows, path, labels=labels, notes=notes)

    # Significant immutable GGUF byte ranges and metadata records.
    range_defs = [
        ("gguf:tiny-tensor:header", "fixtures/gguf/pfir10-tiny-tensor-v3.gguf", "fixtures/gguf/pfir10-tiny-tensor-v3.locators.json", ["header"]),
        ("gguf:tiny-tensor:probe.weight", "fixtures/gguf/pfir10-tiny-tensor-v3.gguf", "fixtures/gguf/pfir10-tiny-tensor-v3.locators.json", ["tensor_data", "probe.weight"]),
        ("gguf:vocab:tokenizer.ggml.tokens", "fixtures/gguf/pfir10-byte-bpe-chat-v3.gguf", "fixtures/gguf/pfir10-byte-bpe-chat-v3.locators.json", ["metadata", "tokenizer.ggml.tokens"]),
        ("gguf:vocab:chat-template-default", "fixtures/gguf/pfir10-byte-bpe-chat-v3.gguf", "fixtures/gguf/pfir10-byte-bpe-chat-v3.locators.json", ["metadata", "tokenizer.chat_template"]),
        ("gguf:vocab:chat-template-strict", "fixtures/gguf/pfir10-byte-bpe-chat-v3.gguf", "fixtures/gguf/pfir10-byte-bpe-chat-v3.locators.json", ["metadata", "tokenizer.chat_template.strict"]),
        ("gguf:vocab:chat-template-tools", "fixtures/gguf/pfir10-byte-bpe-chat-v3.gguf", "fixtures/gguf/pfir10-byte-bpe-chat-v3.locators.json", ["metadata", "tokenizer.chat_template.tools"]),
        ("gguf:model:token_embd.weight", "fixtures/gguf/pfir10-tiny-llama-f32-v3.gguf", "fixtures/gguf/pfir10-tiny-llama-f32-v3.locators.json", ["tensor_data", "token_embd.weight"]),
        ("gguf:rocmfpx:type106-data", "fixtures/fork-specific/rocmfpx-turbo4-type106-probe.gguf", "fixtures/fork-specific/rocmfpx-turbo4-type106-probe.locators.json", ["tensor_data", "turbo4.probe"]),
    ]
    for fid, data_path, loc_path, keys in range_defs:
        loc = json.loads((root / loc_path).read_text(encoding="utf-8"))
        cur: Any = loc
        for key in keys: cur = cur[key]
        start, end = int(cur["start"]), int(cur["end"])
        data = (root / data_path).read_bytes(); sub = data[start:end]
        rows.append({
            "fixture_id": fid,
            "category": "gguf",
            "locator": {"kind": "byte-range", "path": data_path, "start": start, "end_exclusive": end},
            "file_sha256": sha(data),
            "range_sha256": sha(sub),
            "range_length": len(sub),
            "license": "CC0-1.0",
            "license_evidence": "evidence/licenses/CC0-1.0.txt",
            "claim_labels": ["SELF-GENERATED"] + (["QUALIFICATION-REQUIRED"] if "model:" in fid or "rocmfpx" in fid else []),
            "oracle_profile": "exact-bytes" if "type106" not in fid else "fork-specific",
            "deterministic_recipe": "recipes/generate_assets.py",
        })
    return sorted(rows, key=lambda r: r["fixture_id"])


def candidates_manifest() -> dict[str, Any]:
    return {
      "schema_version": 1,
      "access_date": ACCESS_DATE,
      "approval_status": "proposal-not-approved",
      "claim_labels": ["VERIFIED-SOURCE", "PROPOSAL", "QUALIFICATION-REQUIRED"],
      "candidates": [
        {"candidate_id":"llama.cpp","repository":"ggml-org/llama.cpp","commit":"86a9c79f866799eb0e7e89c03578ccfbcc5d808e","default_branch":"master","identity_status":"verified","license":"MIT","license_evidence":"evidence/licenses/llama.cpp-LICENSE.txt","commit_evidence":"evidence/raw/commits/llama_cpp.json"},
        {"candidate_id":"ROCmFPX","repository":"charlie12345/ROCmFPX","commit":"61f2f2d7bc4955e9bca821095ef69125837133b5","default_branch":"main","identity_status":"verified","license":"MIT","license_evidence":"evidence/licenses/ROCmFPX-LICENSE.txt","third_party_notice":"evidence/licenses/ROCmFPX-THIRD_PARTY_NOTICES.md","commit_evidence":"evidence/raw/commits/ROCmFPX.json"},
        {"candidate_id":"CachyLLama","repository":"fewtarius/CachyLLama","commit":"6be745998f568e379ea197fcf827baec73ff9940","default_branch":"master","identity_status":"verified","license":"MIT","license_evidence":"evidence/licenses/CachyLLama-LICENSE.txt","commit_evidence":"evidence/raw/commits/CachyLLama.json"},
        {"candidate_id":"HaloFPX","repository":None,"commit":None,"default_branch":None,"identity_status":"open","license":None,"license_evidence":None,"commit_evidence":"evidence/raw/commits/HaloFPX.json","claim_labels":["UNVERIFIED-IDENTITY","OPEN-QUESTION"],"rule":"All applicability remains open; do not infer a substitute repository."},
      ]
    }


def comparator_profiles() -> dict[str, Any]:
    return {"schema_version":1,"profiles":{
      "exact-bytes":{"class":"exact-output","rule":"SHA-256 and byte length equal"},
      "exact-token-ids":{"class":"exact-output","rule":"integer token vector equal; detokenized bytes equal input bytes"},
      "exact-template-utf8":{"class":"exact-output","rule":"UTF-8 byte sequence equal"},
      "canonical-json":{"class":"exact-output","rule":"project dynamic fields, parse JSON, compare semantic tree; tool arguments parse then canonicalize"},
      "request-contract":{"class":"exact-output","rule":"request file is immutable input; response evaluated by another declared profile"},
      "sse-normalized":{"class":"exact-output","rule":"incremental UTF-8 decode; data records in order; malformed-count and post-DONE-count exact"},
      "schema-validity":{"class":"exact-output","rule":"valid/invalid boolean equal under declared JSON-Schema subset"},
      "exact-allowed-token-set":{"class":"exact-output","rule":"allowed next byte/token set equal; ordering ignored"},
      "json-boundary-validity":{"class":"expected-rejection","rule":"all incomplete cuts reject; complete record accepts"},
      "utf8-validity":{"class":"expected-rejection","rule":"validity boolean equal"},
      "rejection-class":{"class":"expected-rejection","rule":"non-success plus coarse error class; diagnostic wording is not an oracle"},
      "gguf-structural":{"class":"exact-output","rule":"container fields, metadata, type IDs, dimensions, alignment, and tensor byte ranges equal"},
      "numeric-probabilities":{"class":"numerical","rule":"token order exact when present; probabilities abs(error)<=1e-5; rtol=0"},
      "numeric_f32_restore":{"class":"numerical","rule":"same continuation token IDs; logits abs(error)<=1e-5; rtol=0"},
      "rng-metamorphic":{"class":"metamorphic","rule":"same-candidate clone/replay equality; no cross-fork exact RNG sequence oracle"},
      "state-metamorphic":{"class":"metamorphic","rule":"clone/reset/restore semantics; opaque serialization bytes excluded"},
      "state-semantic":{"class":"metamorphic","rule":"continuation and logits equivalence after save/restore"},
      "fork-specific":{"class":"fork-specific","rule":"evaluate only for declared candidate; other candidates reject or are not applicable"}
    }}


def applicability_rows() -> list[dict[str, Any]]:
    # required | expected-reject | not-applicable | open only.
    base = [
      ("GGUF-01","generic GGUF v3 header/tensor parser","required","required","required","open","gguf-structural"),
      ("GGUF-02","vocab-only GGUF load","required","required","required","open","gguf-structural"),
      ("GGUF-03","tiny self-generated Llama F32 model load/inference","required","required","required","open","numeric_f32_restore"),
      ("GGUF-NEG-01","bad magic/truncation/alignment/type boundary corpus","expected-reject","expected-reject","expected-reject","open","rejection-class"),
      ("TOK-01","byte BPE edge cases including invalid UTF-8","required","required","required","open","exact-token-ids"),
      ("TOK-02","special-token parse/no-parse and BOS behavior","required","required","required","open","exact-token-ids"),
      ("CHAT-01","default chat template metadata/rendering","required","required","required","open","exact-template-utf8"),
      ("CHAT-02","named strict/tools chat templates","required","required","required","open","exact-template-utf8"),
      ("CHAT-NEG-01","malformed templates and unsupported role","expected-reject","expected-reject","expected-reject","open","rejection-class"),
      ("STRUCT-01","JSON Schema and GBNF structured output","required","required","required","open","schema-validity"),
      ("GRAMMAR-STATE-01","grammar accepted-prefix/clone state","required","required","required","open","exact-allowed-token-set"),
      ("SAMPLER-01","temperature/top-k/top-p/min-p/penalty vectors","required","required","required","open","numeric-probabilities"),
      ("RNG-01","same-candidate seeded clone/replay property","required","required","required","open","rng-metamorphic"),
      ("RNG-XFORK-EXACT","exact RNG token sequence across forks","not-applicable","not-applicable","not-applicable","open","rng-metamorphic"),
      ("API-01","static tool-call and structured-output response normalization","required","required","required","open","canonical-json"),
      ("API-02","incremental SSE content/tool/structured traces","required","required","required","open","sse-normalized"),
      ("API-NEG-01","invalid request/schema/message shapes","expected-reject","expected-reject","expected-reject","open","rejection-class"),
      ("API-EXT-01","stream_options with stream=false","open","open","open","open","rejection-class"),
      ("API-LIVE-TOOL-01","live required tool call from generated model","open","open","open","open","canonical-json"),
      ("STATE-01","whole-context save/restore semantic continuation","required","open","required","open","state-semantic"),
      ("STATE-02","fragmented/sequence state isolation and restore","required","required","required","open","state-semantic"),
      ("STATE-BYTES-01","opaque state-file byte equality across forks","not-applicable","not-applicable","not-applicable","open","state-metamorphic"),
      ("RECURRENT-01","recurrent rollback and dirty-context restore","open","open","open","open","numeric_f32_restore"),
      ("SPEC-NGRAM-01","model-free ngram-simple speculative equivalence","required","open","open","open","state-semantic"),
      ("SPEC-MTP-01","MTP/model-assisted speculative equivalence","open","open","open","open","state-semantic"),
      ("ROCM-TYPE-106","ROCmFPX TURBO4 type-ID dispatch probe","expected-reject","required","expected-reject","open","fork-specific"),
      ("ROCM-TURBO-CPU","ROCmFPX TurboQuant CPU vector recipe","not-applicable","required","not-applicable","open","fork-specific"),
    ]
    rows=[]
    for tid,scope,llama,rocm,cachy,halo,prof in base:
      rows.append({"test_id":tid,"scope":scope,"oracle_profile":prof,"llama.cpp":llama,"ROCmFPX":rocm,"CachyLLama":cachy,"HaloFPX":halo,"claim_labels":["PROPOSAL","QUALIFICATION-REQUIRED"],"approval_status":"not-approved"})
    return rows


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    m = root / "manifests"; m.mkdir(exist_ok=True)
    fixture_rows = build_fixture_rows(root)
    write_jsonl(m / "fixtures.jsonl", fixture_rows)
    write_json(m / "candidates.json", candidates_manifest())
    write_json(m / "comparator-profiles.json", comparator_profiles())
    app = applicability_rows(); write_json(m / "applicability.json", {"schema_version":1,"allowed_statuses":["required","expected-reject","not-applicable","open"],"rows":app})
    with (m / "applicability.csv").open("w", encoding="utf-8", newline="") as f:
        fields=["test_id","scope","oracle_profile",*CANDIDATES,"approval_status","claim_labels"]
        w=csv.DictWriter(f,fieldnames=fields); w.writeheader()
        for row in app:
            out={k:row.get(k) for k in fields}; out["claim_labels"]=";".join(row["claim_labels"]); w.writerow(out)
    write_json(m / "excluded-assets.json", {
      "schema_version":1,"access_date":ACCESS_DATE,"claim_labels":["EXCLUDED-LICENSE-UNCLEAR","EXCLUDED-PROVENANCE-GAP"],
      "assets":[
        {"id":"ggml-org-vocabs-binaries","source":"https://huggingface.co/ggml-org/vocabs","status":"excluded","reason_label":"EXCLUDED-LICENSE-UNCLEAR","reason":"No sufficient per-repository/per-artifact license evidence was observed."},
        {"id":"stories15M-q4_0.gguf","sha256":"66967fbece6dbe97886593fdbb73589584927e29119ec31f08090732d1861739","status":"excluded","reason_label":"EXCLUDED-PROVENANCE-GAP","reason":"Checksum is pinned upstream, but the exact converted artifact license/conversion chain was not sufficiently preserved."},
        {"id":"stories15M-q4_0-big-endian.gguf","sha256":"9aec857937849d976f30397e97eb1cabb53eb9dcb1ce4611ba8247fb5f44c65d","status":"excluded","reason_label":"EXCLUDED-PROVENANCE-GAP"},
        {"id":"publisher-MTP-EAGLE-DFlash-draft-weights","status":"excluded","reason_label":"EXCLUDED-LICENSE-UNCLEAR","reason":"No minimal exact artifact with complete redistribution and conversion evidence chain was accepted."}
      ]})
    all_fixture_files=sorted({r["locator"]["path"] for r in fixture_rows})
    write_json(m / "accepted-assets.proposed.json", {
      "schema_version":1,"approval_status":"proposal-not-approved","claim_labels":["PROPOSAL","SELF-GENERATED","QUALIFICATION-REQUIRED"],
      "legal_payload":{"license":"CC0-1.0","files":all_fixture_files},
      "execution_tiers":{
        "static-or-structural-after-review":[p for p in all_fixture_files if "tiny-llama-f32" not in p and "type106" not in p and not p.startswith("fixtures/state/")],
        "requires-isolated-candidate-qualification":[p for p in all_fixture_files if "tiny-llama-f32" in p or "type106" in p or p.startswith("fixtures/state/")]
      },
      "mandatory_gate":"Human approval of exact files and hashes after local source-derived applicability review."
    })
    write_json(m / "coverage.json", {
      "schema_version":1,
      "requested_domains":["tiny GGUF tensors/models","tokenizer edge cases","default/named chat templates","special-token behavior","tool calls/structured output","streaming API traces","malformed-boundary inputs","state save/restore","recurrent","MTP/speculative","sampler/RNG/grammar state"],
      "fixture_record_count":len(fixture_rows),
      "disposition":{"recurrent":"open recipe; no accepted recurrent GGUF","MTP":"open recipe; no accepted model-assisted artifact","speculative":"model-free ngram scenario present","candidate_execution":"none"}
    })
    return 0
if __name__ == "__main__": raise SystemExit(main())
