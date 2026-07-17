import pytest
from llama_conformance.compare import (
    compare_tokens, compare_text, compare_json, compare_numeric, compare_distribution
)
from llama_conformance.errors import UncalibratedToleranceError

def approved(**metrics):
    base = {
        "max_abs":None,"max_rel":None,"mean_abs":None,"cosine_distance":None,
        "top1_must_match":None,"top_k_overlap_min":None,
        "distribution_statistic_max":None,"p_value_min":None,"quality_delta_max":None,
    }
    base.update(metrics)
    return {"status":"APPROVED","metrics":base}

def test_exact_tokens():
    assert compare_tokens([1,2,3],[1,2,3]).passed
    result = compare_tokens([1,2,3],[1,9,3])
    assert not result.passed
    assert result.metrics["first_mismatch"] == 1

def test_text_normalizes_crlf():
    assert compare_text("a\nb\n","a\r\nb\r\n").passed

def test_json_drops_declared_field():
    assert compare_json({"x":1,"created":1},{"x":1,"created":2},["created"]).passed

def test_uncalibrated_numeric_rejected():
    with pytest.raises(UncalibratedToleranceError):
        compare_numeric([1.0],[1.0],{"status":"UNCALIBRATED","metrics":{"max_abs":None}})

def test_approved_numeric_profile():
    assert compare_numeric([1.0,2.0],[1.0,2.0001],approved(max_abs=0.001)).passed
    assert not compare_numeric([1.0],[1.1],approved(max_abs=0.01)).passed

def test_top1_numeric():
    assert compare_numeric([1.0,3.0],[1.1,2.9],approved(top1_must_match=True)).passed
    assert not compare_numeric([1.0,3.0],[4.0,2.9],approved(top1_must_match=True)).passed

def test_distribution_requires_approved_limit():
    with pytest.raises(UncalibratedToleranceError):
        compare_distribution(["a"],["a"],{"status":"PROPOSED","metrics":{"distribution_statistic_max":1.0}})
    assert compare_distribution(["a","b"],["a","b"],approved(distribution_statistic_max=0.0)).passed
