from llama_conformance.select import select_cases

MATRIX = {"cases":[
    {"id":"A-001","area":"A","ci_tier":"pr","backend_scope":["cpu"],"failure_injection":False,
     "applicability":{"upstream":"required"}},
    {"id":"B-001","area":"B","ci_tier":"nightly","backend_scope":["gpu"],"failure_injection":True,
     "applicability":{"upstream":"not-applicable"}},
]}

def test_select_area():
    assert [x["id"] for x in select_cases(MATRIX, areas=["A"])] == ["A-001"]

def test_excludes_not_applicable():
    assert select_cases(MATRIX, fork="upstream") == [MATRIX["cases"][0]]

def test_excludes_failure():
    assert select_cases(MATRIX, include_failure_injection=False) == [MATRIX["cases"][0]]
