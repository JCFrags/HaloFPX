"""Independent stdlib-only golden vector for the L05k registry successor."""

import hashlib
import hmac
import json
import pathlib
import sys


def head(major, value):
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
    raise ValueError("outside uint64")


def uint(value):
    return head(0, value)


def bstr(value):
    return head(2, len(value)) + value


def tstr(value):
    encoded = value.encode("ascii")
    return head(3, len(encoded)) + encoded


def map_(entries):
    previous = -1
    out = bytearray(head(5, len(entries)))
    for key, value in entries:
        if key <= previous:
            raise ValueError("keys not strictly increasing")
        previous = key
        out += uint(key) + value
    return bytes(out)


def repeated(byte):
    return bytes([byte]) * 32


receipt = map_(
    [
        (0, uint(41)),
        (1, bstr(repeated(0xCC))),
        (2, bstr(repeated(0xDD))),
        (3, bstr(repeated(0xEE))),
        (4, bstr(repeated(0x11))),
        (5, bstr(repeated(0x22))),
    ]
)
body = map_(
    [
        (0, uint(2)),
        (1, uint(0)),
        (2, uint(0)),
        (3, tstr("registry-v1")),
        (4, uint(9)),
        (5, bstr(repeated(0xAA))),
        (6, bstr(repeated(0xBB))),
        (7, uint(41)),
        (8, bstr(bytes.fromhex("a7b731bccfdea83a4595d5257ffa34ef9248bb61499b40a37874895cff6bc1ec"))),
        (9, receipt),
    ]
)

key_id = "registry-auth-v1"
generation = 13
master = repeated(0x44)
tuple_encoding = tstr(key_id) + uint(generation)
derived = hmac.new(
    master,
    b"halofpx.registry-successor-key.v1\0" + tuple_encoding,
    hashlib.sha256,
).digest()
continuity = hmac.new(
    master,
    b"halofpx.registry-key-continuity.v1\0" + tuple_encoding,
    hashlib.sha256,
).digest()
auth_input = map_([(0, body), (1, tstr(key_id)), (2, uint(1)), (3, uint(generation))])
tag = hmac.new(
    derived,
    b"halofpx.registry-successor-auth.v1\0" + auth_input,
    hashlib.sha256,
).digest()
envelope = map_([(0, auth_input), (1, bstr(tag))])
digest = hashlib.sha256(b"halofpx.registry-successor.v1\0" + envelope).digest()

result = {
    "encoded_bytes": len(envelope),
    "envelope_hex": envelope.hex(),
    "tag_hmac_sha256_hex": tag.hex(),
    "envelope_digest_sha256_hex": digest.hex(),
    "key_continuity_hmac_sha256_hex": continuity.hex(),
}

if len(sys.argv) == 3 and sys.argv[1] == "--check":
    receipt_path = pathlib.Path(sys.argv[2])
    expected = json.loads(receipt_path.read_text(encoding="utf-8"))
    for key, value in result.items():
        if expected.get(key) != value:
            raise SystemExit(f"golden mismatch for {key}")
elif len(sys.argv) == 1:
    print(json.dumps(result, sort_keys=True))
else:
    raise SystemExit(f"usage: {pathlib.Path(sys.argv[0]).name} [--check VECTOR.json]")
