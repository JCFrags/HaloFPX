import hashlib
import hmac
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "authority", ROOT / "scripts" / "halofpx_replay_authority.py")
AUTHORITY = importlib.util.module_from_spec(SPEC)
assert SPEC.loader
SPEC.loader.exec_module(AUTHORITY)


class ReplayAuthorityTest(unittest.TestCase):
    def setUp(self):
        self.key = bytes(range(32))
        self.parts = [
            "phase=restore", "version=1", "graph_reused=0",
            "scheduler_reset=1", "graph_nodes=42", "output_count=1",
            "output_row=0", "output_swaps=0", "logits_backend=ROCm0",
            "flash_attention=1", "backend_count=2", "backend=0,RPC0",
            "backend=1,ROCm0", "kv_type_k=q8_0", "kv_type_v=q8_0",
            "kv_v_trans=1", "kv_n_stream=1", "kv_prepare_slots=0:17",
            "kv_apply_slots=0:17", "kv_heads_before=17",
            "kv_heads_after=18", "kv_n=256", "kv_positions=1128",
            "kv_sequence_ids=0",
            "kv_tensor=0,k,q8_0,64,64,1,1,34,2176,139264,139264,0,4096,RPC0",
            "attention_view=0,k,0,64,1,256,1,34,2176,2176,557056",
        ]

    def record(self, parts=None):
        canonical = "|".join(parts or self.parts)
        tag = hmac.new(
            self.key, AUTHORITY.DOMAIN + canonical.encode(), hashlib.sha256
        ).hexdigest()
        return AUTHORITY.PREFIX + canonical + "|auth_tag=" + tag

    def test_accepts_complete_authenticated_record(self):
        parsed = AUTHORITY.parse_record(self.record(), self.key)
        self.assertEqual(parsed["kv_prepare_slots"], "0:17")

    def test_each_scalar_sentinel_is_fail_closed(self):
        for name in sorted(AUTHORITY.REQUIRED - {"auth_tag"}):
            with self.subTest(name=name):
                parts = [p for p in self.parts if not p.startswith(name + "=")]
                with self.assertRaises(ValueError):
                    AUTHORITY.parse_record(self.record(parts), self.key)

    def test_prepare_apply_divergence_is_refused(self):
        parts = [
            "kv_apply_slots=0:18" if p.startswith("kv_apply_slots=") else p
            for p in self.parts
        ]
        with self.assertRaisesRegex(ValueError, "slot mismatch"):
            AUTHORITY.parse_record(self.record(parts), self.key)

    def test_tamper_duplicate_and_unknown_are_refused(self):
        with self.assertRaisesRegex(ValueError, "authentication"):
            AUTHORITY.parse_record(self.record()[:-1] + "0", self.key)
        for extra in ("|graph_nodes=9", "|mystery=1"):
            canonical = "|".join(self.parts) + extra
            tag = hmac.new(
                self.key, AUTHORITY.DOMAIN + canonical.encode(), hashlib.sha256
            ).hexdigest()
            with self.assertRaises(ValueError):
                AUTHORITY.parse_record(
                    AUTHORITY.PREFIX + canonical + "|auth_tag=" + tag, self.key)

    def test_repeated_authority_geometry_and_order_are_refused(self):
        replacements = {
            "backend=0,RPC0": "backend=2,RPC0",
            "kv_tensor=0,k,q8_0": "kv_tensor=0,x,q8_0",
            "attention_view=0,k,0": "attention_view=0,k,-1",
        }
        for old, new in replacements.items():
            with self.subTest(old=old):
                parts = [p.replace(old, new) for p in self.parts]
                with self.assertRaises(ValueError):
                    AUTHORITY.parse_record(self.record(parts), self.key)
        duplicate = self.parts + [self.parts[-1]]
        with self.assertRaisesRegex(ValueError, "duplicate attention"):
            AUTHORITY.parse_record(self.record(duplicate), self.key)


if __name__ == "__main__":
    unittest.main()
