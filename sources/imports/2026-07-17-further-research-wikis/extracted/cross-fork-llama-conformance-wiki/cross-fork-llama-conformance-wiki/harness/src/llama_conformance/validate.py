from __future__ import annotations
import re
from typing import Any
from .errors import InvalidObservationError

CASE_RE = re.compile(r"^[A-Z]+-[0-9]{3}$")
SHA_RE = re.compile(r"^[0-9a-f]{64}$")

def validate_observation(obs: dict[str, Any]) -> None:
    required = {"schema_version","case_id","fork","source","build","environment","fixtures","run","result","artifacts"}
    missing = sorted(required - set(obs))
    if missing:
        raise InvalidObservationError(f"missing observation keys: {missing}")
    if obs.get("schema_version") != "1.0":
        raise InvalidObservationError("unsupported observation schema version")
    if not CASE_RE.fullmatch(str(obs.get("case_id",""))):
        raise InvalidObservationError("invalid case_id")
    source = obs.get("source", {})
    if not source.get("repository") or not source.get("commit"):
        raise InvalidObservationError("source repository and commit are required")
    build = obs.get("build", {})
    if not SHA_RE.fullmatch(str(build.get("binary_sha256",""))):
        raise InvalidObservationError("build.binary_sha256 must be 64 lowercase hex characters")
    for fixture in obs.get("fixtures", []):
        digest = fixture.get("sha256")
        if digest is not None and not SHA_RE.fullmatch(str(digest)):
            raise InvalidObservationError(f"invalid fixture digest for {fixture.get('id')!r}")
    status = obs.get("result", {}).get("status")
    if status not in {"pass","fail","error","skip","cancelled","timeout"}:
        raise InvalidObservationError(f"invalid result status {status!r}")
    if status == "skip" and not obs.get("result", {}).get("skip_reason"):
        raise InvalidObservationError("skip requires a reason")
