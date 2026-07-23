import hashlib
import importlib.util
import struct
import sys
import tempfile
import threading
import unittest
from pathlib import Path


SCRIPT = Path(__file__).parents[1] / "scripts" / "halofpx_state_component_diagnostics.py"
SPEC = importlib.util.spec_from_file_location("halofpx_component_diagnostics", SCRIPT)
assert SPEC and SPEC.loader
diagnostics = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = diagnostics
SPEC.loader.exec_module(diagnostics)


KEY = bytes(range(32))


def component(
    ordinal: int,
    content: str,
    *,
    group: int = 0,
    begin: int | None = None,
    view_offset: int = 0,
    nb: list[int] | None = None,
) -> dict[str, object]:
    size = 64
    begin = ordinal * 128 if begin is None else begin
    record = {
        "ordinal": ordinal,
        "kind": 1 + ordinal % 2,
        "type": 0,
        "ne": [16, 1, 1, 1],
        "nb": nb or [4, 64, 64, 64],
        "view_offset": view_offset,
        "size": size,
        "label_sha256": hashlib.sha256(f"component-{ordinal}".encode()).hexdigest(),
        "content_sha256": content,
        "buffer_group": group,
        "range_begin": begin,
        "range_end": begin + size,
    }
    record["leaf_sha256"] = hashlib.sha256(diagnostics.component_bytes(record)).hexdigest()
    return record


def phase_lines(phase: str, components: list[dict[str, object]]) -> list[str]:
    lines = []
    for item in components:
        lines.append(
            "[halofpx-state-diag-component] "
            f"phase={phase} ordinal={item['ordinal']} kind={item['kind']} type={item['type']} "
            f"ne={','.join(map(str, item['ne']))} nb={','.join(map(str, item['nb']))} "
            f"view_offset={item['view_offset']} size={item['size']} "
            f"label_sha256={item['label_sha256']} content_sha256={item['content_sha256']} "
            f"buffer_group={item['buffer_group']} range={item['range_begin']}:{item['range_end']} "
            f"leaf_sha256={item['leaf_sha256']}"
        )
    aggregate = hashlib.sha256(
        b"".join(diagnostics.component_bytes(item)[:144] for item in components)
    ).hexdigest()
    merkle = diagnostics.merkle_root(
        [bytes.fromhex(str(item["leaf_sha256"])) for item in components]
    ).hex()
    summary = {
        "phase": phase, "components": len(components),
        "bytes": sum(int(item["size"]) for item in components),
        "aggregate_sha256": aggregate, "merkle_sha256": merkle,
    }
    tag = diagnostics.state_hmac(KEY, diagnostics.summary_bytes(summary)).hex()
    lines.append(
        f"[halofpx-state-diag] phase={phase} components={summary['components']} "
        f"bytes={summary['bytes']} descriptor_content_sha256={aggregate} "
        f"merkle_sha256={merkle} auth_tag={tag}"
    )
    return lines


def log(capture, stage, apply) -> str:
    return "\n".join(
        phase_lines("capture", capture)
        + phase_lines("stage", stage)
        + phase_lines("apply", apply)
    ) + "\n"


def live_recapture_log(capture, stage, apply, recapture) -> str:
    return "\n".join(
        phase_lines("capture", capture)
        + phase_lines("stage", stage)
        + phase_lines("apply", apply)
        + phase_lines("capture", recapture)
    ) + "\n"


def live_receipt(*, changed_family: str | None = None) -> bytes:
    header = diagnostics.LIVE_HEADER.pack(
        b"HFXLIV1\0", 1, 0x0F, 1128, 15048, 2301688, 2454528, 64, 0
    )
    digests = {
        family: hashlib.sha256(family.encode()).digest()
        for family in ("control", "local", "manifest")
    }
    body = bytearray(header)
    for family in ("control", "local", "manifest"):
        for phase in ("original", "restore_input", "live", "pregeneration"):
            value = digests[family]
            if changed_family == family and phase == "live":
                value = hashlib.sha256(b"changed").digest()
            body.extend(value)
    body.extend(hashlib.sha256(b"object").digest())
    body.extend(bytes(32))
    tag = diagnostics.domain_hmac(KEY, diagnostics.LIVE_DOMAIN, bytes(body))
    body[-32:] = tag
    return bytes(body)


class ComponentDiagnosticTests(unittest.TestCase):
    def test_frozen_two_line_key_loads_without_exposure(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "channel.key"
            path.write_bytes(("11" * 32 + "\n" + "22" * 32 + "\n").encode("ascii"))
            self.assertEqual(diagnostics.load_control_key(path), bytes([0x11]) * 32)
            path.write_bytes(("11" * 32 + "\n").encode("ascii"))
            with self.assertRaisesRegex(diagnostics.DiagnosticError, "frozen"):
                diagnostics.load_control_key(path)

    def test_exact_components_views_and_noncontiguous_metadata_compare(self):
        content = hashlib.sha256(b"same").hexdigest()
        records = [
            component(0, content, view_offset=32),
            component(1, content, group=1, nb=[4, 96, 96, 96]),
        ]
        report = diagnostics.compare(diagnostics.parse(log(records, records, records), KEY), KEY)
        self.assertEqual(report["mismatches"], [])
        self.assertEqual(len(report["report_auth_tag"]), 64)

    def test_first_content_divergence_localizes_incomplete_async_observation(self):
        before = hashlib.sha256(b"complete").hexdigest()
        after = hashlib.sha256(b"incomplete-write").hexdigest()
        capture = [component(0, before), component(1, before)]
        apply = [component(0, before), component(1, after)]
        report = diagnostics.compare(
            diagnostics.parse(log(capture, capture, apply), KEY), KEY
        )
        self.assertEqual(report["mismatches"], [{
            "boundary": "stage_to_apply", "ordinal": 1, "reason": "content",
            "kind": 2, "type": 0, "size": 64,
            "label_sha256": capture[1]["label_sha256"],
            "left_content_sha256": before, "right_content_sha256": after,
        }])

    def test_synthetic_async_completion_changes_apply_boundary_to_equal(self):
        complete = hashlib.sha256(b"complete").hexdigest()
        pending = hashlib.sha256(b"pending").hexdigest()
        capture = [component(0, complete)]
        live = [component(0, pending)]
        release = threading.Event()

        def complete_write():
            release.wait()
            live[0] = component(0, complete)

        writer = threading.Thread(target=complete_write)
        writer.start()
        early = diagnostics.compare(
            diagnostics.parse(log(capture, capture, live), KEY), KEY
        )
        self.assertEqual(early["mismatches"][0]["boundary"], "stage_to_apply")
        release.set()
        writer.join(timeout=1)
        self.assertFalse(writer.is_alive())
        late = diagnostics.compare(
            diagnostics.parse(log(capture, capture, live), KEY), KEY
        )
        self.assertEqual(late["mismatches"], [])

    def test_overlapping_ranges_refuse(self):
        content = hashlib.sha256(b"same").hexdigest()
        records = [component(0, content, begin=0), component(1, content, begin=32)]
        with self.assertRaisesRegex(diagnostics.DiagnosticError, "overlapping ranges"):
            diagnostics.parse(log(records, records, records), KEY)

    def test_identity_stride_or_view_change_is_not_normalized_away(self):
        content = hashlib.sha256(b"same").hexdigest()
        capture = [component(0, content, view_offset=16)]
        changed = [component(0, content, view_offset=32, nb=[4, 128, 128, 128])]
        report = diagnostics.compare(
            diagnostics.parse(log(capture, changed, changed), KEY), KEY
        )
        self.assertEqual(report["mismatches"][0]["reason"], "component_identity")

    def test_leaf_merkle_and_authentication_tampering_refuse(self):
        content = hashlib.sha256(b"same").hexdigest()
        records = [component(0, content)]
        exact = log(records, records, records)
        for tampered in (
            exact.replace(content, "f" * 64, 1),
            exact.replace("auth_tag=", "auth_tag=" + "0", 1),
        ):
            with self.assertRaises(diagnostics.DiagnosticError):
                diagnostics.parse(tampered, KEY)

    def test_extra_malformed_or_truncated_marker_refuses(self):
        content = hashlib.sha256(b"same").hexdigest()
        records = [component(0, content)]
        exact = log(records, records, records)
        for malformed in (
            exact + "[halofpx-state-diag-component] phase=apply ordinal=0\n",
            exact + "[halofpx-state-diag] phase=apply components=1\n",
            exact.replace(" phase=capture ", " phase=CAPTURE ", 1),
        ):
            with self.assertRaisesRegex(
                diagnostics.DiagnosticError, "malformed or ambiguous"
            ):
                diagnostics.parse(malformed, KEY)

    def test_authenticated_live_recapture_is_independent_fourth_phase(self):
        content = hashlib.sha256(b"same").hexdigest()
        records = [component(0, content), component(1, content)]
        phases = diagnostics.parse(
            live_recapture_log(records, records, records, records),
            KEY,
            require_recapture=True,
        )
        report = diagnostics.compare(phases, KEY)
        self.assertEqual(
            report["schema"],
            "halofpx.state-component-live-recapture-diagnostic.v1",
        )
        self.assertEqual(list(report["phases"]), ["capture", "stage", "apply", "recapture"])
        self.assertEqual(report["mismatches"], [])

    def test_missing_duplicate_or_changed_live_recapture_refuses_or_localizes(self):
        exact = hashlib.sha256(b"same").hexdigest()
        changed = hashlib.sha256(b"changed").hexdigest()
        records = [component(0, exact)]
        with self.assertRaisesRegex(diagnostics.DiagnosticError, "incomplete phases"):
            diagnostics.parse(log(records, records, records), KEY, require_recapture=True)
        with self.assertRaisesRegex(
            diagnostics.DiagnosticError, "duplicate summary|out-of-order"
        ):
            diagnostics.parse(
                live_recapture_log(records, records, records, records)
                + "\n".join(phase_lines("capture", records))
                + "\n",
                KEY,
                require_recapture=True,
            )
        report = diagnostics.compare(
            diagnostics.parse(
                live_recapture_log(records, records, records, [component(0, changed)]),
                KEY,
                require_recapture=True,
            ),
            KEY,
        )
        self.assertEqual(
            report["mismatches"][0],
            {
                "boundary": "apply_to_recapture",
                "ordinal": 0,
                "reason": "content",
                "kind": 1,
                "type": 0,
                "size": 64,
                "label_sha256": records[0]["label_sha256"],
                "left_content_sha256": exact,
                "right_content_sha256": changed,
            },
        )

    def test_live_recapture_phase_reordering_refuses(self):
        content = hashlib.sha256(b"same").hexdigest()
        records = [component(0, content)]
        reordered = "\n".join(
            phase_lines("capture", records)
            + phase_lines("apply", records)
            + phase_lines("stage", records)
            + phase_lines("capture", records)
        ) + "\n"
        with self.assertRaisesRegex(diagnostics.DiagnosticError, "out-of-order"):
            diagnostics.parse(reordered, KEY, require_recapture=True)

    def test_authenticated_coordinator_live_receipt_is_exact_and_complete(self):
        report = diagnostics.parse_live_receipt(live_receipt(), KEY)
        self.assertEqual(report["token_boundary"], 1128)
        self.assertEqual(report["worker_components"], 64)
        self.assertEqual(report["original_control_sha256"], report["live_control_sha256"])

    def test_coordinator_live_receipt_corruption_incomplete_or_phase_change_refuses(self):
        exact = live_receipt()
        cases = [
            exact[:-1],
            exact[:-1] + bytes([exact[-1] ^ 1]),
            live_receipt(changed_family="control"),
            live_receipt(changed_family="local"),
            live_receipt(changed_family="manifest"),
        ]
        for encoded in cases:
            with self.assertRaises(diagnostics.DiagnosticError):
                diagnostics.parse_live_receipt(encoded, KEY)

    def test_coordinator_live_receipt_wrong_phase_mask_or_zero_boundary_refuses(self):
        exact = bytearray(live_receipt())
        struct.pack_into("<I", exact, 12, 0x07)
        exact[-32:] = diagnostics.domain_hmac(
            KEY, diagnostics.LIVE_DOMAIN, bytes(exact[:-32]) + bytes(32)
        )
        with self.assertRaisesRegex(diagnostics.DiagnosticError, "authority"):
            diagnostics.parse_live_receipt(bytes(exact), KEY)


if __name__ == "__main__":
    unittest.main()
