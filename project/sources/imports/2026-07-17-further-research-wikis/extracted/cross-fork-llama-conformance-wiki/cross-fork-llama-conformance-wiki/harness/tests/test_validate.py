import pytest
from llama_conformance.validate import validate_observation
from llama_conformance.errors import InvalidObservationError

def valid():
    return {
        "schema_version":"1.0","case_id":"DET-001","fork":"upstream",
        "source":{"repository":"x/y","commit":"abcdef0"},
        "build":{"binary_sha256":"0"*64},
        "environment":{},"fixtures":[],"run":{},
        "result":{"status":"pass"},"artifacts":[],
    }

def test_valid_minimal_shape():
    validate_observation(valid())

def test_skip_requires_reason():
    obs = valid()
    obs["result"]["status"] = "skip"
    with pytest.raises(InvalidObservationError):
        validate_observation(obs)
