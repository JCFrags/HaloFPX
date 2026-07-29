from pathlib import Path

import pytest

from scripts.halofpx_l100_kv_range_audit import (
    audit, classify_tensor, intersect, merge, q8_row_bytes,
    serialized_q8_copy_bytes, subtract, total)


ROOT = Path(__file__).resolve().parents[1]


def test_contiguous_and_padded_ranges() -> None:
    assert merge([(0, 8), (8, 16), (24, 32)]) == [(0, 16), (24, 32)]
    assert subtract([(0, 32)], [(0, 16)]) == [(16, 32)]
    assert intersect([(0, 24)], [(16, 32)]) == [(16, 24)]


def test_quantized_block_rounding_and_truncation() -> None:
    assert q8_row_bytes(32) == 34
    assert q8_row_bytes(1024) == 1088
    with pytest.raises(ValueError):
        q8_row_bytes(31)
    assert serialized_q8_copy_bytes(1128 * 1088) == 38352


def test_strided_view_alias_overlap_and_gaps() -> None:
    rows = [(0, 34), (68, 102)]
    alias = [(0, 34)]
    assert total(rows) == 68
    assert intersect(rows, alias) == alias
    assert subtract([(0, 102)], rows) == [(34, 68)]
    assert subtract([(0, 35)], [(0, 34)]) == [(34, 35)]
    assert subtract([(0, 68)], [(0, 34)]) == [(34, 68)]


def test_one_byte_and_one_quantized_block_gap() -> None:
    assert total(subtract([(0, 69)], [(0, 34), (35, 69)])) == 1
    assert subtract([(0, 102)], [(0, 34), (68, 102)]) == [(34, 68)]


def test_view_offset_and_noncontiguous_rows() -> None:
    base = 256
    stride = 68
    rows = [(base + i * stride, base + i * stride + 34) for i in range(3)]
    assert merge(rows) == [(256, 290), (324, 358), (392, 426)]
    assert total(rows) == 102


def test_actual_classifier_contiguous_padded_truncated_and_view_offset() -> None:
    tensor = {
        "ne": [32, 10, 1, 1], "nb": [34, 68, 680, 680], "allocation_offset": 256,
        "allocation_bytes": 680}
    result = classify_tensor(tensor, occupied_rows=4, padded_rows=6, serialized_bytes=136)
    assert result["provably_read_intervals"] == [
        (256, 290), (324, 358), (392, 426), (460, 494)]
    assert result["source_intervals_covered_by_serialization"] == [(256, 392)]
    assert result["unrepresented_provably_read"] == [(392, 426), (460, 494)]
    assert result["possibly_read_intervals"] == [(528, 562), (596, 630)]
    assert result["row_stride_padding"] == [
        (290, 324), (358, 392), (426, 460), (494, 528), (562, 596), (630, 664)]
    assert result["allocation_only_padding"] == [(664, 936)]


def test_retained_l98_exact_coverage_discriminator() -> None:
    result = audit(ROOT)
    assert result["tensor_count"] == 124
    assert result["rpc_tensor_count"] == 64
    assert result["local_tensor_count"] == 60
    assert result["component_counts"] == {
        "capture": 64, "stage": 64, "apply": 64, "live_recapture": 64}
    assert result["totals"] == {
        "rpc_provably_read_bytes": 78544896,
        "rpc_serialized_bytes": 2454528,
        "rpc_restored_bytes": 2454528,
        "rpc_unrepresented_provably_read_bytes": 76090368,
        "rpc_unrepresented_possibly_read_padding_bytes": 10584064,
        "rpc_serialized_staging_union_bytes": 2454528,
        "rpc_serialized_staging_overlap_bytes": 0,
        "local_provably_read_bytes": 73635840,
        "local_serialized_payload_bytes": 2301120,
        "local_blob_header_and_length_bytes": 568,
        "local_unrepresented_provably_read_bytes": 71334720,
        "all_provably_read_bytes": 152180736,
        "all_serialized_tensor_payload_bytes": 4755648,
        "all_unrepresented_provably_read_bytes": 147425088,
    }
    assert all(
        tensor["capture_component"]["size"] == 38352
        and tensor["capture_component"]["content_sha256"]
            == tensor["stage_component"]["content_sha256"]
            == tensor["commit_component"]["content_sha256"]
            == tensor["live_recapture_component"]["content_sha256"]
        and tensor["unrepresented_provably_read"]
        for tensor in result["tensors"] if "capture_component" in tensor)
    assert all(
        tensor["capture_component"]["ordinal"]
        == tensor["layer"] + (0 if tensor["kind"] == "k" else 32)
        for tensor in result["tensors"] if "capture_component" in tensor)
    assert all(
        tensor["local_blob_component"]["size"] == 38352
        and tensor["unrepresented_provably_read"]
        for tensor in result["tensors"] if "local_blob_component" in tensor)
