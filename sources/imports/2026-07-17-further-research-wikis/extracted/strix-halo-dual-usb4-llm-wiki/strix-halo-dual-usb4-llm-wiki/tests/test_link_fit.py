from __future__ import annotations

import importlib.util
import math
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("fit_link_model", ROOT / "tools" / "fit_link_model.py")
assert SPEC and SPEC.loader
mod = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = mod
SPEC.loader.exec_module(mod)


def test_exact_affine_fit() -> None:
    bandwidth = 2.5e9
    latency = 20e-6
    xs = [1024.0, 4096.0, 65536.0, 1048576.0]
    ys = [latency + x / bandwidth for x in xs]
    result = mod.fit(xs, ys)
    assert math.isclose(result["one_way_fixed_cost_s"], latency, rel_tol=1e-9)
    assert math.isclose(result["effective_payload_bandwidth_Bps"], bandwidth, rel_tol=1e-9)
    assert math.isclose(result["r_squared"], 1.0, rel_tol=1e-12)
