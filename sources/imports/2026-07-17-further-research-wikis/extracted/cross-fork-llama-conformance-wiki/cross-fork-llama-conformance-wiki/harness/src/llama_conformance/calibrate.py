from __future__ import annotations
import math
from typing import Iterable, Any

def observed_numeric_deltas(reference: Iterable[float], candidates: Iterable[Iterable[float]]) -> dict[str, Any]:
    ref = [float(x) for x in reference]
    all_abs: list[float] = []
    all_rel: list[float] = []
    per_run: list[dict[str, float]] = []
    for candidate in candidates:
        cand = [float(x) for x in candidate]
        if len(ref) != len(cand):
            raise ValueError("vector length mismatch")
        abs_d = [abs(a-b) for a,b in zip(ref,cand)]
        rel_d = [0.0 if a == b else (abs(a-b)/abs(a) if a != 0 else math.inf) for a,b in zip(ref,cand)]
        all_abs.extend(abs_d)
        all_rel.extend(rel_d)
        per_run.append({
            "max_abs":max(abs_d, default=0.0),
            "max_rel":max(rel_d, default=0.0),
            "mean_abs":sum(abs_d)/len(abs_d) if abs_d else 0.0,
        })
    return {
        "status":"PROPOSED_EVIDENCE_ONLY",
        "observed":{
            "runs":per_run,
            "max_abs_across_runs":max(all_abs, default=0.0),
            "max_rel_across_runs":max(all_rel, default=0.0),
        },
        "normative_metrics":{
            "max_abs":None,
            "max_rel":None,
            "mean_abs":None,
            "cosine_distance":None,
        },
        "warning":"Observed extrema are not approved tolerances and must not be copied automatically into a gate.",
    }
