from __future__ import annotations
from typing import Any, Iterable

def select_cases(
    matrix: dict[str, Any],
    *,
    ids: Iterable[str] = (),
    areas: Iterable[str] = (),
    tiers: Iterable[str] = (),
    fork: str | None = None,
    backend: str | None = None,
    include_failure_injection: bool = True,
) -> list[dict[str, Any]]:
    id_set, area_set, tier_set = set(ids), set(areas), set(tiers)
    out: list[dict[str, Any]] = []
    for case in matrix.get("cases", []):
        if id_set and case["id"] not in id_set:
            continue
        if area_set and case["area"] not in area_set:
            continue
        if tier_set and case["ci_tier"] not in tier_set:
            continue
        if fork:
            state = case["applicability"].get(fork, "unspecified")
            if state == "not-applicable":
                continue
        if backend and backend not in case["backend_scope"] and "gpu" not in case["backend_scope"]:
            continue
        if not include_failure_injection and case.get("failure_injection"):
            continue
        out.append(case)
    return out
