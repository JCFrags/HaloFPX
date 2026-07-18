# OPEN-API-01 input package

## Included protocol fixtures

- Non-streaming tool-call and strict structured-output request/expected records.
- Canonical content streaming.
- Tool-call delta fragmentation across SSE records and byte chunks.
- Structured JSON output split across SSE records.
- UTF-8 code-point splits across transport chunks.
- Malformed JSON SSE record and post-`[DONE]` record handling.
- Invalid message/tool/schema request shapes and an explicitly variable extension case.

## Normalization contract

- Parse incrementally as UTF-8.
- Preserve event order.
- Stop semantic consumption at `[DONE]`.
- Count malformed and post-DONE records according to the fixture.
- Canonicalize embedded tool argument JSON before semantic comparison.
- Ignore only fields explicitly projected by the comparator profile.

## Open live-generation item

The static protocol corpus is ready for parser/serializer testing. Model-backed exact tool-call generation remains `open` until a qualified model/template/parser combination is demonstrated.
