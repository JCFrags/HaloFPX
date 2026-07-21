#!/usr/bin/env python3
"""Independent stdlib golden generator for the L10b exact-session encoding."""

import hashlib
import hmac
import sys


def head(major: int, value: int) -> bytes:
    if value < 24:
        return bytes([(major << 5) | value])
    if value <= 0xFF:
        return bytes([(major << 5) | 24, value])
    if value <= 0xFFFF:
        return bytes([(major << 5) | 25]) + value.to_bytes(2, "big")
    if value <= 0xFFFFFFFF:
        return bytes([(major << 5) | 26]) + value.to_bytes(4, "big")
    return bytes([(major << 5) | 27]) + value.to_bytes(8, "big")


def uint(value: int) -> bytes:
    return head(0, value)


def digest(value: bytes) -> bytes:
    return head(2, len(value)) + value


def golden() -> str:
    tokens = [1, 23, 24, 255, 256, 70000]
    values = [
        digest(bytes(range(0x20, 0x40))),
        digest(bytes(range(0x40, 0x60))),
        head(2, len(tokens) * 4) + b"".join(token.to_bytes(4, "big") for token in tokens),
        uint(6), uint(4), uint(1),
        digest(bytes(range(0x60, 0x80))),
        digest(bytes(range(0x80, 0xA0))),
        digest(bytes(range(0xA0, 0xC0))),
        uint(7), uint(2), uint(1), uint(1),
    ]
    canonical = head(5, len(values)) + b"".join(
        uint(index) + value for index, value in enumerate(values)
    )
    domain = b"halofpx.exact-session.v1\x00"
    return hmac.new(bytes(range(32)), domain + canonical, hashlib.sha256).hexdigest()


if __name__ == "__main__":
    actual = golden()
    if len(sys.argv) == 3 and sys.argv[1] == "--check":
        if actual != sys.argv[2]:
            raise SystemExit(f"golden mismatch: {actual}")
    elif len(sys.argv) != 1:
        raise SystemExit("usage: halofpx-exact-session-golden.py [--check HEX]")
    print(actual)
