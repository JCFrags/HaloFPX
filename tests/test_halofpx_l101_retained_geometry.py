from pathlib import Path

from scripts.halofpx_l100_kv_range_audit import audit, q8_row_bytes


ROOT = Path(__file__).resolve().parents[1]


def test_all_retained_l98_tensors_have_exact_corrected_occupied_geometry() -> None:
    retained = audit(ROOT)
    occupied_rows = retained["geometry"]["kv_prepare_slots"]
    total = 0
    for tensor in retained["tensors"]:
        row_bytes = q8_row_bytes(tensor["ne"][0])
        assert row_bytes == tensor["nb"][1]
        requested = occupied_rows * row_bytes
        assert requested % 34 == 0
        elements = requested // 34 * 32
        assert q8_row_bytes(elements) == requested
        total += requested
    assert len(retained["tensors"]) == 124
    assert total == 152180736
