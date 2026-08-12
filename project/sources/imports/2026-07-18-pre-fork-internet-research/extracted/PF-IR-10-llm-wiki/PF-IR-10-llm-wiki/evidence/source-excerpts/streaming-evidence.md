# Streaming evidence

- **Source:** pinned `tools/ui/tests/unit/sse.test.ts`.
- **Claim:** `VERIFIED-SOURCE`.
- **Observed cases:** multiple JSON data records, `[DONE]`, malformed JSON skipping, chunk-split input, and missing body.
- **Disposition:** PF-IR-10 adds UTF-8 code-point splits, tool-call delta fragmentation, and post-`[DONE]` records as self-generated protocol traces.
