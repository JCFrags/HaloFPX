---
section_id: "66"
title: "HaloFPX Extensions and Stable Error Model"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project"]
  software_versions: []
  hardware_revisions: ["two-node Strix Halo target"]
related_sections: ["09", "38", "46", "48", "60", "67", "68", "69", "71"]
---

# Extensions and stable errors

## Namespaced request extensions

**[RECOMMENDATION]** Put product controls under one optional `halofpx` object. Unknown keys are rejected in strict mode and ignored only when the client explicitly requests forward-compatible handling.

```json
{
  "model": "qwen-coder-primary",
  "messages": [{"role": "user", "content": "..."}],
  "halofpx": {
    "user_id": "local-user-17",
    "objective": "interactive_latency",
    "cache": {"read": true, "write": true, "durability": "persistent"},
    "plan_hint": "auto",
    "deterministic_test": {"enabled": false, "seed": 0}
  }
}
```

- `user_id`: routing/accounting identity only after binding to authenticated identity; never trust an arbitrary body value for authorization.
- `objective`: `interactive_latency | throughput | capacity | balanced`.
- `cache`: explicit read/write and `none | session | persistent` durability; policy may reduce but never exceed caller authorization.
- `plan_hint`: `auto` or an allowed stable plan ID; a hint cannot bypass compatibility/admission.
- `deterministic_test`: test-only profile that fixes declared sampling/backend settings. **[INFERENCE]** A seed does not guarantee cross-backend bitwise identity.

## Error envelope

```json
{
  "error": {
    "message": "human-readable summary",
    "type": "halofpx_unavailable_error",
    "param": null,
    "code": "rank_unavailable"
  },
  "request_id": "req_...",
  "retryable": true,
  "retry_after_ms": 750
}
```

| HTTP | Stable code examples | Retry default |
|---:|---|---|
| 400/422 | `invalid_request`, `unsupported_option`, `incompatible_plan` | No |
| 401/403 | `authentication_failed`, `forbidden` | No without credential/policy change |
| 404 | `model_not_found`, `session_not_found` | No |
| 409 | `session_epoch_conflict`, `model_draining` | Only from a clean boundary |
| 413 | `request_too_large` | No; reduce request |
| 429 | `user_limit`, `queue_full`, `rate_limit` | Yes when `Retry-After` supplied |
| 500 | `internal_error` | Only if request is safely replayable |
| 502/503 | `rank_unavailable`, `plan_unavailable`, `model_loading` | Yes from clean boundary |
| 504 | `deadline_exceeded` | Conditional; generation may have started |

## Retry and idempotency

- **[RECOMMENDATION]** GET and token-count requests are replayable. Generation is replayable only before an acceptance/stream-start boundary or with a client idempotency key whose stored outcome is known.
- **[RECOMMENDATION]** Never automatically retry after partial streamed output unless the client opts into a new response and can discard the first.
- **[RECOMMENDATION]** Cancellation is best effort but changes the request to a terminal cancelled state; stale rank output is fenced by session/request epoch.

