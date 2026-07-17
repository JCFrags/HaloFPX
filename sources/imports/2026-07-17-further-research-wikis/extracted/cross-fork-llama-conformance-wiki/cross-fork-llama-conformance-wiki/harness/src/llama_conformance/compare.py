from __future__ import annotations
from dataclasses import dataclass, asdict
import math
from collections import Counter
from typing import Any, Iterable

from .errors import UncalibratedToleranceError
from .normalize import normalize_json, normalize_newlines

@dataclass(frozen=True)
class ComparisonResult:
    passed: bool
    kind: str
    metrics: dict[str, Any]
    failures: tuple[str, ...]

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)

def _result(kind: str, metrics: dict[str, Any], failures: list[str]) -> ComparisonResult:
    return ComparisonResult(not failures, kind, metrics, tuple(failures))

def compare_tokens(reference: Iterable[int], candidate: Iterable[int]) -> ComparisonResult:
    ref = list(reference)
    cand = list(candidate)
    failures: list[str] = []
    first_mismatch = None
    if len(ref) != len(cand):
        failures.append(f"length mismatch: reference={len(ref)} candidate={len(cand)}")
    for i, (a, b) in enumerate(zip(ref, cand)):
        if a != b:
            first_mismatch = i
            failures.append(f"token mismatch at index {i}: reference={a} candidate={b}")
            break
    return _result("tokens-exact", {
        "reference_length": len(ref),
        "candidate_length": len(cand),
        "first_mismatch": first_mismatch,
    }, failures)

def compare_text(reference: str, candidate: str, normalize_eol: bool = True) -> ComparisonResult:
    ref = normalize_newlines(reference) if normalize_eol else reference
    cand = normalize_newlines(candidate) if normalize_eol else candidate
    failures = [] if ref == cand else ["text differs"]
    first_mismatch = next((i for i, (a, b) in enumerate(zip(ref, cand)) if a != b), None)
    if first_mismatch is None and len(ref) != len(cand):
        first_mismatch = min(len(ref), len(cand))
    return _result("text-exact", {
        "reference_bytes": len(ref.encode("utf-8")),
        "candidate_bytes": len(cand.encode("utf-8")),
        "first_mismatch": first_mismatch,
    }, failures)

def compare_json(reference: Any, candidate: Any, drop_fields: Iterable[str] = ()) -> ComparisonResult:
    ref = normalize_json(reference, drop_fields)
    cand = normalize_json(candidate, drop_fields)
    return _result("json-normalized", {}, [] if ref == cand else ["normalized JSON differs"])

def _approved_metrics(profile: dict[str, Any]) -> dict[str, Any]:
    if profile.get("status") != "APPROVED":
        raise UncalibratedToleranceError(
            f"profile status is {profile.get('status')!r}; numeric gates require APPROVED"
        )
    metrics = profile.get("metrics")
    if not isinstance(metrics, dict):
        raise UncalibratedToleranceError("profile has no metrics object")
    if not any(value is not None for value in metrics.values()):
        raise UncalibratedToleranceError("approved profile contains no populated metric")
    return metrics

def _finite_vector(values: Iterable[float], label: str) -> list[float]:
    out = [float(x) for x in values]
    bad = [i for i, x in enumerate(out) if not math.isfinite(x)]
    if bad:
        raise ValueError(f"{label} contains non-finite values at indices {bad[:8]}")
    return out

def compare_numeric(
    reference: Iterable[float],
    candidate: Iterable[float],
    profile: dict[str, Any],
    *,
    reference_token_ids: Iterable[int] | None = None,
    candidate_token_ids: Iterable[int] | None = None,
) -> ComparisonResult:
    limits = _approved_metrics(profile)
    ref = _finite_vector(reference, "reference")
    cand = _finite_vector(candidate, "candidate")
    failures: list[str] = []
    if len(ref) != len(cand):
        return _result("numeric", {"reference_length":len(ref),"candidate_length":len(cand)}, ["length mismatch"])

    abs_deltas = [abs(a-b) for a,b in zip(ref,cand)]
    rel_deltas = [
        0.0 if a == b else (abs(a-b)/abs(a) if a != 0 else math.inf)
        for a,b in zip(ref,cand)
    ]
    dot = sum(a*b for a,b in zip(ref,cand))
    norm_a = math.sqrt(sum(a*a for a in ref))
    norm_b = math.sqrt(sum(b*b for b in cand))
    cosine_distance = 0.0 if ref == cand else (
        1.0 - dot/(norm_a*norm_b) if norm_a and norm_b else math.inf
    )
    observed = {
        "max_abs": max(abs_deltas, default=0.0),
        "max_rel": max(rel_deltas, default=0.0),
        "mean_abs": sum(abs_deltas)/len(abs_deltas) if abs_deltas else 0.0,
        "cosine_distance": cosine_distance,
    }
    for name in ("max_abs","max_rel","mean_abs","cosine_distance"):
        limit = limits.get(name)
        if limit is not None and observed[name] > limit:
            failures.append(f"{name}={observed[name]!r} exceeds approved limit {limit!r}")

    top1_required = limits.get("top1_must_match")
    if top1_required:
        if not ref:
            failures.append("top1 comparison requested for empty vectors")
        else:
            ref_top = max(range(len(ref)), key=ref.__getitem__)
            cand_top = max(range(len(cand)), key=cand.__getitem__)
            observed["reference_top1_index"] = ref_top
            observed["candidate_top1_index"] = cand_top
            if ref_top != cand_top:
                failures.append(f"top1 index differs: reference={ref_top} candidate={cand_top}")

    overlap_min = limits.get("top_k_overlap_min")
    if overlap_min is not None:
        if reference_token_ids is None or candidate_token_ids is None:
            failures.append("top_k_overlap_min requires token ID arrays")
        else:
            rids, cids = list(reference_token_ids), list(candidate_token_ids)
            if not rids and not cids:
                overlap = 1.0
            else:
                denom = max(len(set(rids)), len(set(cids)), 1)
                overlap = len(set(rids) & set(cids))/denom
            observed["top_k_overlap"] = overlap
            if overlap < overlap_min:
                failures.append(f"top_k_overlap={overlap!r} below approved minimum {overlap_min!r}")

    return _result("numeric", observed, failures)

def compare_distribution(reference_samples: Iterable[Any], candidate_samples: Iterable[Any], profile: dict[str, Any]) -> ComparisonResult:
    limits = _approved_metrics(profile)
    ref = list(reference_samples)
    cand = list(candidate_samples)
    if not ref or not cand:
        return _result("distribution", {"reference_n":len(ref),"candidate_n":len(cand)}, ["both sample sets must be nonempty"])
    rc, cc = Counter(ref), Counter(cand)
    keys = set(rc) | set(cc)
    tvd = 0.5 * sum(abs(rc[k]/len(ref) - cc[k]/len(cand)) for k in keys)
    failures: list[str] = []
    limit = limits.get("distribution_statistic_max")
    if limit is None:
        raise UncalibratedToleranceError("approved profile does not set distribution_statistic_max")
    if tvd > limit:
        failures.append(f"total_variation_distance={tvd!r} exceeds approved limit {limit!r}")
    return _result("distribution", {
        "reference_n":len(ref),
        "candidate_n":len(cand),
        "categories":len(keys),
        "total_variation_distance":tvd,
    }, failures)
