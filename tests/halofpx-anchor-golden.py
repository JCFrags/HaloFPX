"""Independent stdlib-only encoder for the L05e synthetic golden vector."""

import hashlib
import hmac
import json
import pathlib
import sys


def head(major, value):
    if value < 24:
        return bytes([(major << 5) | value])
    widths = ((0xFF, 24, 1), (0xFFFF, 25, 2), (0xFFFFFFFF, 26, 4))
    for maximum, additional, width in widths:
        if value <= maximum:
            return bytes([(major << 5) | additional]) + value.to_bytes(width, "big")
    return bytes([(major << 5) | 27]) + value.to_bytes(8, "big")


uint = lambda value: head(0, value)
map_ = lambda count: head(5, count)
bstr = lambda value: head(2, len(value)) + value
text = lambda value: head(3, len(value)) + value

store = bytes([0x11]) * 16
namespace = bytes([0x22]) * 32
lineage = bytes([0x44]) * 32
selected = bytes([0x77]) * 32
predecessor = bytes([0x88]) * 32
key_id = b"anchor-key-v1"
master = bytes(range(64))

body = (map_(11) + uint(0) + uint(1) + uint(1) + uint(0) +
        uint(2) + bstr(store) + uint(3) + bstr(namespace) +
        uint(4) + uint(3) + uint(5) + bstr(lineage) +
        uint(6) + uint(5) + uint(7) + uint(6) + uint(8) + uint(2) +
        uint(9) + bstr(selected) + uint(10) + bstr(predecessor))
auth_input = (map_(4) + uint(0) + body + uint(1) + text(key_id) +
              uint(2) + uint(1) + uint(3) + uint(9))
derived = hmac.new(master, b"halofpx.anchor-key.v1\0" + len(key_id).to_bytes(2, "big") +
                   key_id + store + namespace + (9).to_bytes(8, "big"), hashlib.sha256).digest()
tag = hmac.new(derived, b"halofpx.anchor-auth.v1\0" + auth_input, hashlib.sha256).digest()
envelope = map_(2) + uint(0) + auth_input + uint(1) + bstr(tag)
digest = hashlib.sha256(b"halofpx.anchor.v1\0" + envelope).digest()

result = {"encoded_bytes": len(envelope), "envelope_hex": envelope.hex(),
          "tag_sha256_hex": tag.hex(), "anchor_digest_sha256_hex": digest.hex()}
if len(sys.argv) == 3 and sys.argv[1] == "--check":
    receipt = json.loads(pathlib.Path(sys.argv[2]).read_text(encoding="utf-8"))
    for key, value in result.items():
        if receipt.get(key) != value:
            raise SystemExit(f"golden mismatch for {key}")
else:
    print(json.dumps(result, sort_keys=True))
