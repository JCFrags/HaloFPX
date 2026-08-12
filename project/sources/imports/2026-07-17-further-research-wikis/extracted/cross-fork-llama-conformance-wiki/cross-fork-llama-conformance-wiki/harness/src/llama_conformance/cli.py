from __future__ import annotations
import argparse, json, sys
from pathlib import Path

from .compare import compare_tokens, compare_text, compare_json, compare_numeric, compare_distribution
from .errors import ConformanceError
from .io import load_json, dotted_get
from .select import select_cases
from .validate import validate_observation

def _load_value(path: str, field: str | None):
    obj = load_json(path)
    return dotted_get(obj, field) if field else obj

def cmd_compare(ns: argparse.Namespace) -> int:
    ref = _load_value(ns.reference, ns.reference_field)
    cand = _load_value(ns.candidate, ns.candidate_field)
    profile = load_json(ns.profile) if ns.profile else None
    if ns.kind == "tokens":
        result = compare_tokens(ref, cand)
    elif ns.kind == "text":
        result = compare_text(ref, cand, normalize_eol=not ns.no_normalize_eol)
    elif ns.kind == "json":
        result = compare_json(ref, cand, ns.drop_field)
    elif ns.kind == "numeric":
        if profile is None:
            raise ConformanceError("--profile is required for numeric comparison")
        result = compare_numeric(ref, cand, profile)
    elif ns.kind == "distribution":
        if profile is None:
            raise ConformanceError("--profile is required for distribution comparison")
        result = compare_distribution(ref, cand, profile)
    else:
        raise AssertionError(ns.kind)
    print(json.dumps(result.to_dict(), indent=2))
    return 0 if result.passed else 1

def cmd_select(ns: argparse.Namespace) -> int:
    matrix = load_json(ns.matrix)
    selected = select_cases(
        matrix, ids=ns.id, areas=ns.area, tiers=ns.tier,
        fork=ns.fork, backend=ns.backend,
        include_failure_injection=not ns.exclude_failure_injection,
    )
    if ns.format == "ids":
        for case in selected:
            print(case["id"])
    else:
        print(json.dumps(selected, indent=2))
    return 0

def cmd_validate_observation(ns: argparse.Namespace) -> int:
    obs = load_json(ns.observation)
    validate_observation(obs)
    print(f"valid: {ns.observation}")
    return 0

def build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(prog="llama-conformance")
    sub = ap.add_subparsers(dest="command", required=True)

    cp = sub.add_parser("compare", help="compare two JSON values or observation fields")
    cp.add_argument("--kind", choices=["tokens","text","json","numeric","distribution"], required=True)
    cp.add_argument("--reference", required=True)
    cp.add_argument("--candidate", required=True)
    cp.add_argument("--reference-field")
    cp.add_argument("--candidate-field")
    cp.add_argument("--profile")
    cp.add_argument("--drop-field", action="append", default=[])
    cp.add_argument("--no-normalize-eol", action="store_true")
    cp.set_defaults(func=cmd_compare)

    sp = sub.add_parser("select", help="select cases from the matrix")
    sp.add_argument("--matrix", default="matrix/test-matrix.json")
    sp.add_argument("--id", action="append", default=[])
    sp.add_argument("--area", action="append", default=[])
    sp.add_argument("--tier", action="append", default=[])
    sp.add_argument("--fork", choices=["upstream","rocmfpx","cachyllama","integration"])
    sp.add_argument("--backend")
    sp.add_argument("--exclude-failure-injection", action="store_true")
    sp.add_argument("--format", choices=["ids","json"], default="ids")
    sp.set_defaults(func=cmd_select)

    vp = sub.add_parser("validate-observation")
    vp.add_argument("observation")
    vp.set_defaults(func=cmd_validate_observation)
    return ap

def main(argv: list[str] | None = None) -> int:
    ns = build_parser().parse_args(argv)
    try:
        return ns.func(ns)
    except (ConformanceError, KeyError, ValueError, TypeError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2

if __name__ == "__main__":
    raise SystemExit(main())
