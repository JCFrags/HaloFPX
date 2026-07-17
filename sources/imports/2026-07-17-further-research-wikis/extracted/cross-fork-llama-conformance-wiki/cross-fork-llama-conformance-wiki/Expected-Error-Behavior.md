# Expected error behavior

## Comparison model

Exact diagnostic prose is usually too brittle. Compare:

1. process/HTTP/stream termination behavior;
2. endpoint-required status or event;
3. normalized error class;
4. presence and types of required envelope fields;
5. bounded and redacted diagnostic text;
6. process, slot, cache, or transport condition after failure.

Retain the raw error for debugging.

## Normalized classes

The suite recognizes:

`invalid-format`, `unsupported-version`, `invalid-argument`, `not-supported`, `not-found`, `capacity`, `rate-limit`, `cancelled`, `timeout`, `transport`, `out-of-memory`, `io-error`, `authorization`, and `internal`.

Fork adapters map native errors to these classes. Mapping is versioned and reviewed; it must not turn unrelated errors into equality.

## Error matrix

| Condition | Required outcome |
|---|---|
| Missing model/path | non-success startup/load with a missing-input diagnostic |
| Invalid GGUF | bounded invalid-format/unsupported-version failure; no runnable partial model |
| Unsupported tensor/backend | explicit not-supported or observable configured fallback |
| Invalid CLI value | nonzero exit and option/value diagnostic |
| Unknown endpoint/method | endpoint-consistent routing error |
| Invalid JSON/schema | client error before model execution |
| Context/capacity exceeded | explicit capacity response or configured queue behavior |
| Per-user cap | fork-specific rate-limit response; other users remain independent |
| Host/device OOM | explicit resource failure or observable allowed fallback |
| Disk/cache I/O failure | no false persistence success; partial file not accepted |
| RPC disconnect/mismatch | bounded transport/protocol error |
| Cancellation | no successful completion after cancel; reusable state according to contract |
| Timeout | harness failure with process tree terminated |
| Internal assertion/sanitizer finding | conformance failure, regardless of outward error envelope |

## Cache rejection

A rejected cache may lead to either:

- a cold evaluation with an explicit miss/rejection reason; or
- a request/startup failure if policy forbids fallback.

The chosen behavior must be declared per fork/configuration. A cache hit must never be reported after compatibility rejection.

## Unsupported versus not applicable

- **Not applicable:** capability is intentionally absent and the matrix permits it.
- **Unsupported request:** a client requested an absent feature; the runtime returns a defined error.
- **Skipped test:** the harness did not run the case because capability evidence allows a conditional skip.
- **Error:** the environment could not determine capability or execute the test.

These are distinct report states.

## Security properties

Errors must not expose API keys, authorization headers, full hostile request bodies, arbitrary local paths beyond the configured policy, another user's identifier/path, or unbounded parser content. Redaction is tested separately from semantic error classification.
