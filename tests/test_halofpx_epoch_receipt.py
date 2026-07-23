import importlib.util
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).parents[1] / "scripts" / "halofpx_epoch_receipt.py"
SPEC = importlib.util.spec_from_file_location("halofpx_epoch_receipt", SCRIPT)
assert SPEC and SPEC.loader
receipt = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(receipt)


class EpochReceiptTests(unittest.TestCase):
    def test_exact_receipt_authenticates(self):
        expected = receipt.payload("a" * 64, 101, "b" * 32, 202)
        sealed = receipt.seal(expected, b"key")
        receipt.verify(sealed, expected, b"key")

    def test_object_or_epoch_tamper_refuses(self):
        expected = receipt.payload("a" * 64, 101, "b" * 32, 202)
        sealed = receipt.seal(expected, b"key")
        for changed in (
                receipt.payload("c" * 64, 101, "b" * 32, 202),
                receipt.payload("a" * 64, 303, "b" * 32, 202),
                receipt.payload("a" * 64, 101, "d" * 32, 202)):
            with self.assertRaises(ValueError):
                receipt.verify(sealed, changed, b"key")

    def test_tag_tamper_refuses(self):
        expected = receipt.payload("a" * 64, 101, "b" * 32, 202)
        sealed = receipt.seal(expected, b"key")
        sealed["tag"] = "0" * 64
        with self.assertRaises(ValueError):
            receipt.verify(sealed, expected, b"key")

    def test_atomic_write_refuses_overwrite(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory).resolve() / "receipt.json"
            receipt.atomic_write(path, {"value": 1})
            with self.assertRaises(ValueError):
                receipt.atomic_write(path, {"value": 2})

    def test_missing_receipt_refuses(self):
        with tempfile.TemporaryDirectory() as directory:
            missing = Path(directory) / "missing.json"
            with self.assertRaises(FileNotFoundError):
                missing.read_text(encoding="utf-8")
