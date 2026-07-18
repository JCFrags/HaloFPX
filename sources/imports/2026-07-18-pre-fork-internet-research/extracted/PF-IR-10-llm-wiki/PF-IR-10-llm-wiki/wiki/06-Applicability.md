# Applicability proposal

Allowed values are exactly `required`, `expected-reject`, `not-applicable`, and `open`. The proposal is in `manifests/applicability.csv` and `.json`.

## High-signal rows

| Test | llama.cpp | ROCmFPX | CachyLLama | HaloFPX |
|---|---|---|---|---|
| Generic/vocab GGUF | required | required | required | open |
| Tiny Llama F32 | required | required | required | open |
| Malformed GGUF | expected-reject | expected-reject | expected-reject | open |
| Tokenizer/special/chat | required | required | required | open |
| Static API/SSE | required | required | required | open |
| Whole-context save/restore | required | open | required | open |
| Recurrent rollback | open | open | open | open |
| Model-free n-gram speculation | required | open | open | open |
| MTP/model-assisted speculation | open | open | open | open |
| ROCmFPX type 106 | expected-reject | required | expected-reject | open |
| Exact cross-fork RNG sequence | not-applicable | not-applicable | not-applicable | open |

“Required” is a proposed semantic obligation after qualification; it is not evidence of current execution success.
