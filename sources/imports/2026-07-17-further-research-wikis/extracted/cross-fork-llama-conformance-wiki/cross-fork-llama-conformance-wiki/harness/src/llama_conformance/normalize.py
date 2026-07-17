from __future__ import annotations
from typing import Any, Iterable

DEFAULT_VOLATILE_FIELDS = {
    "created", "created_at", "request_id", "id", "system_fingerprint",
    "prompt_ms", "predicted_ms", "prompt_per_second", "predicted_per_second",
    "model_path", "slot_id", "generation_time_ms",
}

def normalize_newlines(text: str) -> str:
    return text.replace("\r\n", "\n")

def normalize_json(value: Any, drop_fields: Iterable[str] = DEFAULT_VOLATILE_FIELDS) -> Any:
    drop = set(drop_fields)
    if isinstance(value, dict):
        return {
            key: normalize_json(val, drop)
            for key, val in sorted(value.items())
            if key not in drop
        }
    if isinstance(value, list):
        return [normalize_json(item, drop) for item in value]
    if isinstance(value, str):
        return normalize_newlines(value)
    return value

def assemble_sse(events: list[dict[str, Any]], content_key: str = "content") -> dict[str, Any]:
    content_parts: list[str] = []
    terminal: list[dict[str, Any]] = []
    for event in events:
        if not isinstance(event, dict):
            raise TypeError("SSE event must be an object")
        if event.get("stop") is True or event.get("done") is True:
            terminal.append(event)
        else:
            piece = event.get(content_key, "")
            if not isinstance(piece, str):
                raise TypeError(f"{content_key} must be text")
            content_parts.append(piece)
    if len(terminal) != 1:
        raise ValueError(f"expected exactly one terminal event, found {len(terminal)}")
    return {"content": "".join(content_parts), "terminal": normalize_json(terminal[0])}
