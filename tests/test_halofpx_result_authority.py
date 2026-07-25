import hashlib
import hmac
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "result_authority", ROOT / "scripts" / "halofpx_result_authority.py")
AUTHORITY = importlib.util.module_from_spec(SPEC)
assert SPEC.loader
SPEC.loader.exec_module(AUTHORITY)


class ResultAuthorityTest(unittest.TestCase):
    def setUp(self):
        self.key = bytes(range(32))
        self.canonical = (
            "mode=restore label=restore coordinator_pid=42 "
            "prompt_tokens=1129 saved_boundary=1128 n_batch=512 "
            "prompt_chunks=0 max_prompt_chunk=0 tokens=4245,"
        )

    def record(self, canonical=None):
        value = canonical or self.canonical
        tag = hmac.new(
            self.key, AUTHORITY.DOMAIN + value.encode(), hashlib.sha256
        ).hexdigest()
        return value + AUTHORITY.TAG_MARKER + tag + "\n"

    def test_accepts_complete_durable_live_context_result(self):
        parsed = AUTHORITY.verify(self.record(), self.key)
        self.assertEqual(parsed["n_batch"], "512")
        self.assertEqual(len(parsed["result_auth_tag"]), 64)

    def test_tamper_duplicate_partial_and_malformed_are_refused(self):
        cases = (
            self.record()[:-2] + "0\n",
            self.record(self.canonical + " n_batch=0"),
            self.canonical,
            self.record().replace("n_batch=512", "n_batch=3386108400"),
        )
        for record in cases:
            with self.subTest(record=record), self.assertRaises(ValueError):
                parsed = AUTHORITY.verify(record, self.key)
                if parsed.get("n_batch") != "512":
                    raise ValueError("malformed live n_batch")

    def test_source_captures_and_durably_emits_before_single_final_free(self):
        source = (
            ROOT / "tests" / "test-halofpx-distributed-state-canary.cpp"
        ).read_text(encoding="utf-8")
        start = source.index("const size_t result_n_batch = llama_n_batch(run_ctx);")
        durable = source.index("write_text_fsync(", start)
        emitted = source.index("std::fflush(stdout) == 0", durable)
        freed = source.index("llama_free(disposable_ctx);", emitted)
        self.assertLess(start, durable)
        self.assertLess(durable, emitted)
        self.assertLess(emitted, freed)
        final_region = source[start:source.index("if (owns_ctx)", freed)]
        self.assertEqual(final_region.count("llama_free(disposable_ctx);"), 1)
        self.assertNotIn("llama_n_batch(run_ctx)", final_region[freed - start:])


if __name__ == "__main__":
    unittest.main()
