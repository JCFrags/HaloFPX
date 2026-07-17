import pytest
from llama_conformance.normalize import normalize_json, assemble_sse

def test_recursive_normalize():
    assert normalize_json({"created":1,"x":{"b":2,"a":"q\r\n"}}) == {"x":{"a":"q\n","b":2}}

def test_assemble_sse():
    out = assemble_sse([
        {"content":"a","stop":False},
        {"content":"b","stop":False},
        {"stop":True,"timings":{"x":1}},
    ])
    assert out["content"] == "ab"

def test_assemble_sse_requires_one_terminal():
    with pytest.raises(ValueError):
        assemble_sse([{"content":"a"}])
