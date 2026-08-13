#!/usr/bin/env python3
"""Shared canonical digest domain for protected Strix production identities."""

from __future__ import annotations

import hashlib
import json
from collections.abc import Mapping
from typing import Any


# This is the exact field domain of
# ``halofpx_strix_maintenance.ProductionIdentity``.  Both the maintenance
# authority and offline HMM admission result use this one definition so a
# digest can be compared directly rather than translated between lookalike
# identity schemas.
PRODUCTION_IDENTITY_FIELDS = frozenset({
    "role",
    "host",
    "unit",
    "pid",
    "invocation_id",
    "nrestarts",
    "process_start_ticks",
    "start_monotonic_us",
    "executable_sha256",
    "argv_sha256",
    "control_group",
    "listener_port",
    "listener_pid",
    "health_sha256",
})
INTEGER_FIELDS = frozenset({
    "pid",
    "nrestarts",
    "process_start_ticks",
    "start_monotonic_us",
    "listener_port",
    "listener_pid",
})
STRING_FIELDS = PRODUCTION_IDENTITY_FIELDS - INTEGER_FIELDS - {"health_sha256"}


class ProductionIdentityDomainError(ValueError):
    """The value is outside the closed protected-production digest domain."""


def canonical_identity_bytes(value: Mapping[str, Any]) -> bytes:
    """Return the exact canonical bytes historically used by maintenance v1."""
    if not isinstance(value, Mapping) or set(value) != PRODUCTION_IDENTITY_FIELDS:
        raise ProductionIdentityDomainError(
            "production identity has the wrong closed field set")
    for name in STRING_FIELDS:
        item = value[name]
        if not isinstance(item, str) or not item or any(
            ord(character) < 0x20 or 0x7f <= ord(character) <= 0x9f
            for character in item
        ):
            raise ProductionIdentityDomainError(
                f"production identity {name} is not a nonempty control-free string")
        try:
            item.encode("utf-8", errors="strict")
        except UnicodeError as exc:
            raise ProductionIdentityDomainError(
                f"production identity {name} contains a non-scalar value") from exc
    for name in INTEGER_FIELDS:
        if isinstance(value[name], bool) or not isinstance(value[name], int):
            raise ProductionIdentityDomainError(
                f"production identity {name} is not an integer")
    health = value["health_sha256"]
    if health is not None:
        if not isinstance(health, str) or not health:
            raise ProductionIdentityDomainError(
                "production identity health_sha256 is not null or a nonempty string")
        try:
            health.encode("utf-8", errors="strict")
        except UnicodeError as exc:
            raise ProductionIdentityDomainError(
                "production identity health_sha256 contains a non-scalar value") from exc
    # Keep this byte grammar identical to halofpx_strix_ab.canonical_bytes,
    # which defined the existing maintenance ProductionIdentity.digest.
    try:
        return (json.dumps(
            dict(value), sort_keys=True, separators=(",", ":"), allow_nan=False,
        ) + "\n").encode("utf-8", errors="strict")
    except (TypeError, ValueError, UnicodeError) as exc:
        raise ProductionIdentityDomainError(
            "production identity cannot be encoded canonically") from exc


def production_identity_digest(value: Mapping[str, Any]) -> str:
    """Hash an exact protected-production identity in the shared v1 domain."""
    return hashlib.sha256(canonical_identity_bytes(value)).hexdigest()
