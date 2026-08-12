from llama_conformance.calibrate import observed_numeric_deltas

def test_calibration_never_sets_normative_limits():
    out = observed_numeric_deltas([1.0,2.0], [[1.0,2.1]])
    assert out["status"] == "PROPOSED_EVIDENCE_ONLY"
    assert all(value is None for value in out["normative_metrics"].values())
