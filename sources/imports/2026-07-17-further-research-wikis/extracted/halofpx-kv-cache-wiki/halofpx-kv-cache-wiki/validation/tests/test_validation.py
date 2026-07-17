from __future__ import annotations

import json
import shutil
import tempfile
import unittest
from pathlib import Path

from generate_fixtures import generate
from validate_cache import (
    CACHY_INDEX_SIZE, CACHY_RECORD_SIZE, CACHY_SYSTEM_SIZE, HALO_HEADER_SIZE,
    CATALOG_ENTRY_VALID, IMPORT_CANDIDATE_VALID, LEGACY_VALID, MISS, compute_manifest_hmac,
    validate_cachyllama_checkpoint,
    validate_cachyllama_index, validate_cachyllama_system,
    validate_halofpx_manifest, validate_halofpx_object,
)


class ValidationTests(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix="halofpx-validation-"))
        self.fixtures = self.tmp / "fixtures"
        self.values = generate(self.fixtures)

    def tearDown(self):
        shutil.rmtree(self.tmp)

    def test_explicit_struct_sizes(self):
        self.assertEqual(CACHY_RECORD_SIZE, 16480)
        self.assertEqual(CACHY_INDEX_SIZE, 120)
        self.assertEqual(CACHY_SYSTEM_SIZE, 16440)
        self.assertEqual(HALO_HEADER_SIZE, 104)

    def test_valid_legacy_is_not_hit_eligible(self):
        p = self.fixtures / self.values["cachyllama"]["checkpoint"]
        compat = int(self.values["cachyllama"]["compat_u64_hex"], 0)
        r = validate_cachyllama_checkpoint(p, expected_compat=compat)
        self.assertEqual(r.status, LEGACY_VALID)
        self.assertFalse(r.eligible_for_hit)
        self.assertEqual(r.reason, "NO_PAYLOAD_INTEGRITY")

    def test_valid_index_and_system(self):
        compat = int(self.values["cachyllama"]["compat_u64_hex"], 0)
        i = validate_cachyllama_index(self.fixtures / self.values["cachyllama"]["index"], expected_compat=compat)
        s = validate_cachyllama_system(self.fixtures / self.values["cachyllama"]["system"], expected_compat=compat)
        self.assertEqual(i.status, LEGACY_VALID)
        self.assertEqual(s.status, LEGACY_VALID)

    def test_valid_halofpx_manifest_is_import_candidate_not_hit(self):
        p = self.fixtures / self.values["halofpx"]["manifest"]
        root = self.fixtures / self.values["halofpx"]["object_root"]
        key = bytes.fromhex((self.fixtures / self.values["halofpx"]["manifest_hmac_key"]).read_text().strip())
        halo = self.values["halofpx"]
        r = validate_halofpx_manifest(
            p,
            object_root=root,
            manifest_hmac_key=key,
            expected_namespace=halo["namespace_id"],
            expected_compat=halo["compatibility_fingerprint_sha256"],
            expected_cache_key=halo["cache_key_sha256"],
            expected_prompt_root=halo["prompt_root_sha256"],
            expected_engine_family=halo["engine_family"],
        )
        self.assertEqual(r.status, IMPORT_CANDIDATE_VALID)
        self.assertFalse(r.eligible_for_hit)
        self.assertTrue(r.eligible_for_engine_import)

    def test_valid_halofpx_manifest_without_request_binding_is_catalog_only(self):
        p = self.fixtures / self.values["halofpx"]["manifest"]
        root = self.fixtures / self.values["halofpx"]["object_root"]
        key = bytes.fromhex((self.fixtures / self.values["halofpx"]["manifest_hmac_key"]).read_text().strip())
        r = validate_halofpx_manifest(p, object_root=root, manifest_hmac_key=key)
        self.assertEqual(r.status, CATALOG_ENTRY_VALID)
        self.assertFalse(r.eligible_for_hit)
        self.assertFalse(r.eligible_for_engine_import)
        self.assertEqual(
            r.details["missing_current_request_bindings"],
            [
                "cache_key_sha256",
                "compatibility_fingerprint_sha256",
                "engine_family",
                "namespace_id",
                "prompt_root_sha256",
            ],
        )

    def test_halo_payload_bitflip_is_miss(self):
        src = self.fixtures / self.values["halofpx"]["object"]
        data = bytearray(src.read_bytes())
        data[-1] ^= 0x80
        p = self.tmp / src.name
        p.write_bytes(data)
        r = validate_halofpx_object(p)
        self.assertEqual(r.status, MISS)
        self.assertIn(r.reason, {"PAYLOAD_DIGEST_MISMATCH", "OBJECT_FILENAME_DIGEST_MISMATCH", "SEGMENT_DIGEST_MISMATCH"})

    def test_legacy_payload_bitflip_remains_structurally_valid(self):
        src = self.fixtures / self.values["cachyllama"]["checkpoint"]
        data = bytearray(src.read_bytes())
        data[-1] ^= 0x80
        p = self.tmp / "ckpt-1.bin"
        p.write_bytes(data)
        compat = int(self.values["cachyllama"]["compat_u64_hex"], 0)
        r = validate_cachyllama_checkpoint(p, expected_compat=compat)
        self.assertEqual(r.status, LEGACY_VALID)
        self.assertFalse(r.eligible_for_hit)

    def test_manifest_binding_mismatch_is_miss(self):
        src = self.fixtures / self.values["halofpx"]["manifest"]
        m = json.loads(src.read_text())
        m["cache_key_sha256"] = "a" * 64
        key = bytes.fromhex((self.fixtures / self.values["halofpx"]["manifest_hmac_key"]).read_text().strip())
        m["catalog_auth"]["tag_hex"] = compute_manifest_hmac(m, key)
        p = self.tmp / "bad-manifest.json"
        from validate_cache import canonical_json_bytes
        p.write_bytes(canonical_json_bytes(m) + b"\n")
        root = self.fixtures / self.values["halofpx"]["object_root"]
        r = validate_halofpx_manifest(p, object_root=root, manifest_hmac_key=key)
        self.assertEqual(r.status, MISS)

    def test_manifest_without_auth_key_is_miss(self):
        p = self.fixtures / self.values["halofpx"]["manifest"]
        root = self.fixtures / self.values["halofpx"]["object_root"]
        r = validate_halofpx_manifest(p, object_root=root)
        self.assertEqual(r.status, MISS)
        self.assertEqual(r.reason, "MANIFEST_AUTH_KEY_UNAVAILABLE")


if __name__ == "__main__":
    unittest.main()
