import os
from contextlib import contextmanager

import pytest

from utils import *


MODEL_PATH_ENV = "NGRAM_SIMPLE_MODEL_PATH"
STRICT_PARITY_ENV = "NGRAM_SIMPLE_STRICT_PARITY"
STRICT_PARITY_UBATCH_ENV = "NGRAM_SIMPLE_STRICT_PARITY_UBATCH"

STABLE_PROMPT = "Once upon a time"
NEAR_TIE_PROMPT = "I believe the meaning of life is"


def create_server(
        *,
        speculative: bool,
        n_slots: int = 1,
        n_ctx: int = 512,
        n_ubatch: int = 256) -> ServerProcess:
    server = ServerPreset.stories15m_moe()
    model_path = os.environ.get(MODEL_PATH_ENV)
    if model_path:
        server.model_file = model_path
        server.model_hf_repo = None
        server.model_hf_file = None
        server.offline = True

    server.n_threads = 4
    server.n_batch = 256
    server.n_ubatch = n_ubatch
    server.n_ctx = n_ctx
    server.n_slots = n_slots
    server.fa = "off"
    server.cache_ram = 0
    server.temperature = 0.0
    server.seed = 42

    if speculative:
        server.spec_type = "ngram-simple"
        server.spec_draft_n_min = 1
        server.spec_draft_n_max = 16
        server.spec_ngram_simple_size_n = 3
        server.spec_ngram_simple_size_m = 16
        server.spec_ngram_simple_min_hits = 1

    return server


@contextmanager
def running_server(server: ServerProcess):
    server.start()
    try:
        yield server
    finally:
        server.stop()


@pytest.fixture(autouse=True)
def stop_test_servers():
    yield
    # Keep focused runs using --confcutdir safe as well as the full suite.
    for instance in set(server_instances):
        instance.stop()


def completion_request(prompt: str, n_predict: int = 96) -> dict:
    return {
        "prompt": prompt,
        "temperature": 0.0,
        "samplers": ["temperature"],
        "n_predict": n_predict,
        "return_tokens": True,
        "cache_prompt": False,
        "seed": 42,
    }


def completion(server: ServerProcess, request: dict) -> dict:
    response = server.make_request("POST", "/completion", data=request)
    assert response.status_code == 200
    assert response.body["tokens_predicted"] == request["n_predict"]
    assert len(response.body["tokens"]) == request["n_predict"]
    assert all(type(token) is int for token in response.body["tokens"])
    return response.body


def startup_defaults(server: ServerProcess) -> dict:
    response = server.make_request("GET", "/props")
    assert response.status_code == 200
    return response.body["default_generation_settings"]["params"]


def assert_startup_speculation(
        params: dict,
        *,
        spec_type: str,
        n_min: int,
        n_max: int) -> None:
    assert params["speculative.types"] == spec_type
    assert type(params["speculative.types"]) is str
    assert params["speculative.n_min"] == n_min
    assert type(params["speculative.n_min"]) is int
    assert params["speculative.n_max"] == n_max
    assert type(params["speculative.n_max"]) is int


def assert_no_speculative_counters(result: dict) -> None:
    assert "draft_n" not in result["timings"]
    assert "draft_n_accepted" not in result["timings"]


def assert_authoritative_speculative_counters(result: dict) -> None:
    timings = result["timings"]
    assert type(timings["draft_n"]) is int
    assert type(timings["draft_n_accepted"]) is int
    assert timings["draft_n"] > 0
    assert 0 <= timings["draft_n_accepted"] < timings["draft_n"]


def first_token_difference(left: list[int], right: list[int]) -> int | None:
    for index, (left_token, right_token) in enumerate(zip(left, right)):
        if left_token != right_token:
            return index
    if len(left) != len(right):
        return min(len(left), len(right))
    return None


def assert_exact_completion(target: dict, speculative: dict) -> None:
    difference = first_token_difference(target["tokens"], speculative["tokens"])
    assert difference is None, (
        "strict greedy token parity failed; "
        f"first_difference={difference}, "
        f"target_token={target['tokens'][difference] if difference is not None and difference < len(target['tokens']) else None}, "
        f"speculative_token={speculative['tokens'][difference] if difference is not None and difference < len(speculative['tokens']) else None}"
    )
    assert target["content"] == speculative["content"]


def test_ngram_simple_default_off_props_parity_and_rejection():
    request = completion_request(STABLE_PROMPT)

    with running_server(create_server(speculative=False)) as target_server:
        assert_startup_speculation(
            startup_defaults(target_server), spec_type="none", n_min=0, n_max=16)
        target = completion(target_server, request)
        assert_no_speculative_counters(target)

    with running_server(create_server(speculative=True)) as speculative_server:
        assert_startup_speculation(
            startup_defaults(speculative_server), spec_type="ngram-simple", n_min=1, n_max=16)
        speculative = completion(speculative_server, request)
        assert_authoritative_speculative_counters(speculative)

    assert_exact_completion(target, speculative)


def test_ngram_simple_context_shift_exact_parity():
    request = completion_request("Hello " * 248, n_predict=64)

    target_server = create_server(speculative=False, n_ctx=256)
    target_server.enable_ctx_shift = True
    with running_server(target_server):
        target = completion(target_server, request)

    speculative_server = create_server(speculative=True, n_ctx=256)
    speculative_server.enable_ctx_shift = True
    with running_server(speculative_server):
        speculative = completion(speculative_server, request)

    assert target["truncated"] is True
    assert speculative["truncated"] is True
    assert_authoritative_speculative_counters(speculative)
    assert_exact_completion(target, speculative)


def test_ngram_simple_sequential_and_two_slot_exact_request_identity():
    request = completion_request(STABLE_PROMPT)

    with running_server(create_server(speculative=False, n_slots=2)) as target_server:
        target = completion(target_server, request)

    with running_server(create_server(speculative=True, n_slots=2)) as speculative_server:
        sequential = [
            completion(speculative_server, request),
            completion(speculative_server, request),
        ]
        tasks = [
            (completion, (speculative_server, request)),
            (completion, (speculative_server, request)),
        ]
        concurrent = parallel_function_calls(tasks)

    for result in sequential + concurrent:
        assert result is not None
        assert_authoritative_speculative_counters(result)
        assert_exact_completion(target, result)


@pytest.mark.skipif(
    os.environ.get(STRICT_PARITY_ENV) != "1",
    reason=f"set {STRICT_PARITY_ENV}=1 to run the fail-closed strict parity qualification",
)
def test_ngram_simple_strict_greedy_parity_qualification():
    """Compare modes dynamically; do not bake a backend-specific expected token."""
    n_ubatch = int(os.environ.get(STRICT_PARITY_UBATCH_ENV, "256"))
    request = completion_request(NEAR_TIE_PROMPT)

    with running_server(create_server(speculative=False, n_ubatch=n_ubatch)) as target_server:
        target = completion(target_server, request)

    with running_server(create_server(speculative=True, n_ubatch=n_ubatch)) as speculative_server:
        speculative = completion(speculative_server, request)

    assert_authoritative_speculative_counters(speculative)
    assert_exact_completion(target, speculative)
