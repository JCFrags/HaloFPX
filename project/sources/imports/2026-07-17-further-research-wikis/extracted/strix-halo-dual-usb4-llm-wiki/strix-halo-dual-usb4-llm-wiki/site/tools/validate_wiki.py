#!/usr/bin/env python3
"""Validate repository completeness, links, schemas, calculations, and site output."""
from __future__ import annotations

import csv
import hashlib
import importlib.util
import json
import re
import sys
from pathlib import Path
from urllib.parse import urlsplit

import jsonschema
import yaml
from bs4 import BeautifulSoup

ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs"

REQUIRED = [
    "README.md", "SUMMARY.md", "MANIFEST.md", "AUDIT.md", "CITATIONS.md",
    "CITATION.cff", "LICENSE.md", "CONTRIBUTING.md", "CODE_OF_CONDUCT.md",
    "SECURITY.md", "CHANGELOG.md", "THIRD_PARTY_NOTICES.md", "OPEN_WIKI.html", "mkdocs.yml", "VERSION",
    "tools/cost_model.py", "tools/build_site.py", "tools/fit_link_model.py",
    "schemas/placement.schema.json", "data/model_configs.csv",
    "data/formula_catalog.csv", "data/worked_examples.json",
    "docs/executive-summary.md", "docs/decision-framework.md",
    "docs/placement/ownership-matrix.md", "docs/feasibility/gates.md",
    "docs/benchmarking/benchmark-plan.md", "docs/sources.md",
    "site/index.html",
]

EXPECTED_MODES = {
    "tensor_parallel_2", "contiguous_layer_split", "pipeline_parallel_2",
    "moe_layer_local_split", "moe_expert_service", "remote_speculation_greedy",
    "remote_speculation_exact_stochastic", "replicated_decode",
    "hybrid_layer_pipeline", "hybrid_prefill_decode_migration",
}

LABELS = (
    "SOURCED FACT", "CALCULATED", "MEASURED INPUT REQUIRED",
    "SCENARIO ASSUMPTION", "DECISION RULE",
)


def fail(errors: list[str], message: str) -> None:
    errors.append(message)


def load_cost_model():
    path = ROOT / "tools" / "cost_model.py"
    spec = importlib.util.spec_from_file_location("wiki_cost_model", path)
    if spec is None or spec.loader is None:
        raise RuntimeError("cannot load cost_model.py")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def check_required(errors: list[str]) -> None:
    for rel in REQUIRED:
        if not (ROOT / rel).exists():
            fail(errors, f"required path missing: {rel}")


def check_nav(errors: list[str]) -> None:
    cfg = yaml.safe_load((ROOT / "mkdocs.yml").read_text(encoding="utf-8"))
    def walk(items):
        for item in items:
            if isinstance(item, str):
                yield item
            elif isinstance(item, dict):
                _, target = next(iter(item.items()))
                if isinstance(target, str):
                    yield target
                else:
                    yield from walk(target)
    for rel in walk(cfg.get("nav", [])):
        if not (DOCS / rel).is_file():
            fail(errors, f"mkdocs nav target missing: docs/{rel}")


def check_markdown_links(errors: list[str]) -> None:
    pattern = re.compile(r"!?\[[^\]]*\]\(([^)]+)\)")
    for source in DOCS.rglob("*.md"):
        text = source.read_text(encoding="utf-8")
        for raw in pattern.findall(text):
            target = raw.strip().split()[0].strip("<>\"'")
            parts = urlsplit(target)
            if parts.scheme or parts.netloc or not parts.path or target.startswith("#"):
                continue
            resolved = (source.parent / parts.path).resolve()
            if not resolved.exists():
                fail(errors, f"broken local link: {source.relative_to(ROOT)} -> {target}")


def check_placements(errors: list[str]) -> None:
    schema = json.loads((ROOT / "schemas" / "placement.schema.json").read_text(encoding="utf-8"))
    modes: set[str] = set()
    files = sorted((ROOT / "placements").glob("*.yaml"))
    if not files:
        fail(errors, "no placement YAML files")
        return
    for path in files:
        try:
            record = yaml.safe_load(path.read_text(encoding="utf-8"))
            jsonschema.validate(record, schema)
        except Exception as exc:
            fail(errors, f"placement invalid {path.name}: {exc}")
            continue
        modes.add(record["mode"])
        rank_ids = {r["rank_id"] for r in record["ranks"]}
        nodes = {r["node"] for r in record["ranks"]}
        if rank_ids != {0, 1} or nodes != {"A", "B"}:
            fail(errors, f"placement {path.name} must cover rank IDs 0/1 and nodes A/B")
        for rank in record["ranks"]:
            ownership = rank["ownership"]
            for key in ("tokenizer", "sampler", "rng", "model", "experts", "kv", "sessions"):
                if not str(ownership[key]).strip():
                    fail(errors, f"placement {path.name} rank {rank['rank_id']} blank ownership: {key}")
    missing = EXPECTED_MODES - modes
    if missing:
        fail(errors, f"placement modes missing: {sorted(missing)}")


def check_structured_data(errors: list[str]) -> None:
    required_csv = {
        "model_configs.csv": {"model_key", "layers", "hidden_size", "kv_heads", "head_dim", "source"},
        "hardware_facts.csv": {"id", "field", "value", "evidence_label", "source"},
        "assumptions.csv": {"id", "variable", "status", "evidence_label", "required_action"},
        "formula_catalog.csv": {"id", "mode", "phase", "quantity", "formula", "evidence_label"},
        "measurement_template.csv": {"run_id", "timestamp", "message_bytes", "direction", "transport", "evidence_label"},
        "worked_examples.csv": {"model_key", "metric", "value", "evidence"},
    }
    for name, columns in required_csv.items():
        path = ROOT / "data" / name
        if not path.exists():
            fail(errors, f"missing data/{name}")
            continue
        with path.open(newline="", encoding="utf-8") as handle:
            reader = csv.DictReader(handle)
            headers = set(reader.fieldnames or [])
            missing = columns - headers
            if missing:
                fail(errors, f"data/{name} missing columns: {sorted(missing)}")


def check_calculations(errors: list[str]) -> None:
    model = load_cost_model()
    expected = model.demo_payload()
    actual = json.loads((ROOT / "data" / "worked_examples.json").read_text(encoding="utf-8"))
    if actual != expected:
        fail(errors, "worked_examples.json differs from tools/cost_model.py --demo")
    # Core arithmetic invariants.
    llama = model.MODELS["llama31-8b"]
    if model.boundary_activation_bytes(llama, 1, 2) != 8192:
        fail(errors, "Llama 3.1 8B boundary-byte invariant failed")
    if model.kv_bytes_per_token(llama, 2) != 131072:
        fail(errors, "Llama 3.1 8B KV-byte invariant failed")
    tp = model.tensor_parallel(llama, 4096, 2)
    if tp["collectives"] != 64 or tp["per_rank_sent_bytes"] != 2 * 1024**3:
        fail(errors, "Llama 3.1 8B TP worked-example invariant failed")


def check_evidence_and_nonclaims(errors: list[str]) -> None:
    corpus = "\n".join(path.read_text(encoding="utf-8") for path in DOCS.rglob("*.md"))
    for label in LABELS:
        if label not in corpus:
            fail(errors, f"evidence label not used in docs: {label}")
    forbidden = [
        r"(?i)measured\s+usb4\s+bandwidth\s*(?:is|=)",
        r"(?i)strix halo\s+(?:achieves|delivers)\s+\d+(?:\.\d+)?\s*tokens?/s",
        r"(?i)dual usb4\s+(?:achieves|delivers)\s+\d+(?:\.\d+)?\s*(?:gb/s|gbps)",
    ]
    for pattern in forbidden:
        if re.search(pattern, corpus):
            fail(errors, f"possible fabricated performance claim matched: {pattern}")


def check_site(errors: list[str]) -> None:
    index = ROOT / "site" / "index.html"
    if not index.exists():
        fail(errors, "prebuilt site missing")
        return
    html_text = index.read_text(encoding="utf-8")
    for marker in ("LLM WIKI", "WIKI_SEARCH_INDEX", "evidence", "Dual-Strix-Halo"):
        if marker not in html_text:
            fail(errors, f"site/index.html missing marker: {marker}")
    source_pages = list(DOCS.rglob("*.md"))
    built_pages = list((ROOT / "site").rglob("*.html"))
    if len(built_pages) < len(source_pages):
        fail(errors, f"site has {len(built_pages)} HTML pages for {len(source_pages)} Markdown pages")
    for page in built_pages:
        soup = BeautifulSoup(page.read_text(encoding="utf-8"), "html.parser")
        for tag, attr in (("a", "href"), ("img", "src"), ("script", "src"), ("link", "href")):
            for node in soup.find_all(tag):
                raw = str(node.get(attr, ""))
                parts = urlsplit(raw)
                if not raw or raw.startswith("#") or parts.scheme or parts.netloc:
                    continue
                target = (page.parent / parts.path).resolve()
                if not target.exists():
                    fail(errors, f"broken built-site link: {page.relative_to(ROOT)} -> {raw}")


def write_hashes() -> None:
    exclusions = {ROOT / "SHA256SUMS"}
    rows = []
    for path in sorted(p for p in ROOT.rglob("*") if p.is_file() and p not in exclusions and "__pycache__" not in p.parts):
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        rows.append(f"{digest}  {path.relative_to(ROOT).as_posix()}")
    (ROOT / "SHA256SUMS").write_text("\n".join(rows) + "\n", encoding="utf-8")


def main() -> int:
    errors: list[str] = []
    for check in (
        check_required, check_nav, check_markdown_links, check_placements,
        check_structured_data, check_calculations, check_evidence_and_nonclaims,
        check_site,
    ):
        check(errors)
    if errors:
        print("VALIDATION FAILED")
        for error in errors:
            print(f"- {error}")
        return 1
    write_hashes()
    report = (
        "VALIDATION PASSED\n"
        "- required wiki/project files present\n"
        "- MkDocs navigation targets present\n"
        "- local Markdown links resolve\n"
        "- all placement YAML files pass JSON Schema and ownership checks\n"
        "- structured-data columns present\n"
        "- worked examples exactly regenerate from cost_model.py\n"
        "- evidence labels and non-fabrication checks pass\n"
        "- prebuilt static site present and page-complete\n"
        "- SHA256SUMS regenerated\n"
    )
    (ROOT / "VALIDATION_REPORT.txt").write_text(report, encoding="utf-8")
    print(report, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
