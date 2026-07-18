"""Independent stdlib-only golden vector for the L05i bootstrap-admin token."""

import hashlib
import hmac
import json
import pathlib
import sys


def cbor_head(major, value):
    if value < 0 or value > 0xFFFFFFFFFFFFFFFF:
        raise ValueError("CBOR argument is outside the uint64 profile")
    if value < 24:
        return bytes([(major << 5) | value])
    for maximum, additional, width in (
        (0xFF, 24, 1),
        (0xFFFF, 25, 2),
        (0xFFFFFFFF, 26, 4),
        (0xFFFFFFFFFFFFFFFF, 27, 8),
    ):
        if value <= maximum:
            return bytes([(major << 5) | additional]) + value.to_bytes(width, "big")
    raise AssertionError("unreachable")


def cbor_uint(value):
    return cbor_head(0, value)


def cbor_bstr(value):
    return cbor_head(2, len(value)) + value


def cbor_tstr(value):
    encoded = value.encode("ascii")
    return cbor_head(3, len(encoded)) + encoded


def cbor_map(entries):
    # Integer keys are already in deterministic encoded-key order for this profile.
    previous = -1
    encoded = bytearray(cbor_head(5, len(entries)))
    for key, value in entries:
        if key <= previous:
            raise ValueError("map keys must be unique and strictly increasing")
        previous = key
        encoded += cbor_uint(key)
        encoded += value
    return bytes(encoded)


def repeated(byte, count):
    return bytes([byte]) * count


body = cbor_map(
    [
        (0, cbor_uint(1)),
        (1, cbor_uint(0)),
        (2, cbor_uint(0)),
        (3, cbor_bstr(repeated(0x01, 16))),
        (4, cbor_bstr(repeated(0x02, 32))),
        (5, cbor_uint(3)),
        (6, cbor_bstr(repeated(0x04, 32))),
        (7, cbor_tstr("manifest")),
        (8, cbor_uint(5)),
        (9, cbor_bstr(repeated(0x06, 32))),
        (10, cbor_uint(7)),
        (11, cbor_tstr("anchor")),
        (12, cbor_uint(8)),
        (13, cbor_uint(1)),
        (14, b"\xf6"),
        (15, cbor_bstr(repeated(0x09, 32))),
        (16, cbor_bstr(repeated(0x0A, 32))),
        (17, cbor_tstr("registry")),
        (18, cbor_uint(11)),
        (19, cbor_bstr(repeated(0x0C, 32))),
        (20, cbor_bstr(repeated(0x0D, 32))),
        (21, cbor_uint(14)),
        (22, cbor_bstr(repeated(0x0F, 32))),
    ]
)

admin_id = "admin"
admin_generation = 4
master_key = repeated(0x41, 32)

# Each domain includes exactly one terminating NUL byte, matching the v1 contract.
key_input = (
    b"halofpx.bootstrap-token-key.v1\0"
    + cbor_tstr(admin_id)
    + cbor_uint(admin_generation)
)
derived_key = hmac.new(master_key, key_input, hashlib.sha256).digest()
auth_input = cbor_map(
    [
        (0, body),
        (1, cbor_tstr(admin_id)),
        (2, cbor_uint(1)),
        (3, cbor_uint(admin_generation)),
    ]
)
tag = hmac.new(
    derived_key,
    b"halofpx.bootstrap-token-auth.v1\0" + auth_input,
    hashlib.sha256,
).digest()

envelope = cbor_map(
    [
        (0, auth_input),
        (1, cbor_bstr(tag)),
    ]
)
envelope_digest = hashlib.sha256(
    b"halofpx.bootstrap-token.v1\0" + envelope
).digest()

result = {
    "encoded_bytes": len(envelope),
    "envelope_hex": envelope.hex(),
    "tag_hmac_sha256_hex": tag.hex(),
    "envelope_digest_sha256_hex": envelope_digest.hex(),
}

if len(sys.argv) == 3 and sys.argv[1] == "--check":
    receipt = json.loads(pathlib.Path(sys.argv[2]).read_text(encoding="utf-8"))
    for key, value in result.items():
        if receipt.get(key) != value:
            raise SystemExit(f"golden mismatch for {key}")
elif len(sys.argv) == 1:
    print(json.dumps(result, sort_keys=True))
else:
    raise SystemExit(f"usage: {pathlib.Path(sys.argv[0]).name} [--check VECTOR.json]")
