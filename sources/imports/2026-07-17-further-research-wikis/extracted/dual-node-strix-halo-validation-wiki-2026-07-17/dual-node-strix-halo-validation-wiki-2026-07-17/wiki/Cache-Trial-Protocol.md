# Cache Trial Protocol

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


## Cache-state taxonomy

| State | Name | Required conditions | What it isolates |
|---|---|---|---|
| `C0` | Power-on cold | Fresh boot; service stopped; model not resident; page-cache verification; no prime | Firmware/OS/service/model load plus first inference |
| `C1` | OS-cache warm | Model files verified resident in page cache; service freshly started; no model/KV residency | Storage/page-cache effect on model load |
| `C2` | Model-resident warm | Service ready and model loaded; prompt/KV cache cleared; untimed health canary complete | Steady model-resident prefill/decode |
| `C3` | Exact-prefix cache hit | C2 plus identical tokenized prefix primed into an eligible cache slot | Prompt/KV reuse benefit and correctness |

Do not label a second request “warm” without naming which state it satisfies.

## C0 procedure

1. Record shutdown state and boot ID.
2. Reboot both nodes; do not launch the inference service automatically unless startup time is the subject.
3. Verify no prior process, model mapping, swap-in, or remote cache process remains.
4. Capture initial block counters, memory, USB4 state, clocks, and ambient.
5. Start synchronized collectors, then the worker and coordinator.
6. Record service-ready time and execute one fixed canary.
7. Stop after final telemetry tail and preserve all logs.

`sync; echo 3 > /proc/sys/vm/drop_caches` is permitted only on a dedicated lab host when reboot is infeasible. Record the method and never compare reboot-cold and drop-cache-cold in one baseline family. [[SRC-019]](../references/Sources.md#src-019)

## C1 verification

Warm model files by a declared method, then prove warmness using low physical read bytes on a verification pass. A preload command alone is not proof. Record unique model bytes, page-cache tool/version if used, major/minor faults, and block-read deltas.

## C2 reset

- Keep the model loaded.
- Clear or rotate prompt/KV cache using an engine-supported method.
- Confirm the next request reports zero cached prompt tokens.
- Do not restart the service, because that changes model residency.

## C3 eligibility

A request is cache-eligible only when model, tokenizer, chat template, exact token IDs, cache namespace/slot, and retained prefix are identical. Report the eligible token denominator, not total prompt tokens. Run correctness comparisons with cache enabled and disabled because execution shape can change numerical results. [[SRC-009]](../references/Sources.md#src-009)
